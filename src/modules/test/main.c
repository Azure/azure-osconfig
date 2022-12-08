// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Common.h>
#include <Module.h>

#define DEFAULT_BIN_PATH "/usr/lib/osconfig"
#define OSCONFIG_CONFIG_FILE "/etc/osconfig/osconfig.json"

#define AZURE_OSCONFIG "Azure OSConfig"

typedef enum STEP_TYPE
{
    MODULE,
    RUN
} STEP_TYPE;

typedef enum TEST_TYPE
{
    DESIRED,
    REPORTED
} TEST_TYPE;

typedef struct EXPECTATION
{
    int status;
    JSON_Value* value;
} EXPECTATION;

typedef struct TEST
{
    TEST_TYPE type;
    char* component;
    char* object;
    JSON_Value* value;
    EXPECTATION expect;
    const MANAGEMENT_MODULE* module;
} TEST;

typedef struct SCRIPT {
    char** commands;
    int num;
} SCRIPT;

typedef struct STEP
{
    STEP_TYPE type;
    int delay;
    union
    {
        TEST test;
        SCRIPT script;
    } data;
} STEP;

static bool g_verbose = false;

void FreeStep(STEP* step)
{
    if (step == NULL)
    {
        return;
    }

    if (step->type == MODULE)
    {
        free(step->data.test.component);
        free(step->data.test.object);
        json_value_free(step->data.test.value);
        json_value_free(step->data.test.expect.value);
    }
    else
    {
        for (int i = 0; i < step->data.script.num; i++)
        {
            free(step->data.script.commands[i]);
        }
        free(step->data.script.commands);
    }
}

JSON_Value* ParseValue(const JSON_Object* object)
{
    JSON_Value* value = NULL;
    const char* json = NULL;

    if (NULL == (value = json_object_get_value(object, "payload")))
    {
        if (NULL != (json = json_object_get_string(object, "json")))
        {
            value = json_parse_string(json);
        }
    }

    return json_value_deep_copy(value);
}

int ParseTest(const JSON_Object* object, TEST* test)
{
    int status = 0;
    const char* type = NULL;
    JSON_Object* expect_object = NULL;

    if (NULL == (type = json_object_get_string(object, "type")))
    {
        LOG_ERROR("Missing test type");
        status = EINVAL;
    }

    if (strcmp(type, "reported") == 0)
    {
        test->type = REPORTED;
    }
    else if (strcmp(type, "desired") == 0)
    {
        test->type = DESIRED;
        test->value = ParseValue(object);
    }
    else
    {
        status = EINVAL;
    }

    if (NULL == (test->component = strdup(json_object_get_string(object, "component"))))
    {
        LOG_ERROR("'component' is required for '%s' test", type);
        status = EINVAL;
    }

    if (NULL == (test->object = strdup(json_object_get_string(object, "object"))))
    {
        LOG_ERROR("'object' is required for '%s' test", type);
        status = EINVAL;
    }

    if (NULL != (expect_object = json_object_get_object(object, "expect")))
    {
        test->expect.status = json_object_get_number(expect_object, "status");
        test->expect.value = ParseValue(expect_object);
    }

    return status;
}

int ParseScript(const JSON_Value* object, SCRIPT* script)
{
    int status = 0;
    const char* command = NULL;
    JSON_Array* array = NULL;

    if (script == NULL)
    {
        LOG_ERROR("Invalid (null) script");
        return EINVAL;
    }

    if (json_value_get_type(object) == JSONArray)
    {
        array = json_value_get_array(object);
        script->num = json_array_get_count(array);
        if (NULL == (script->commands = calloc(script->num, sizeof(char*))))
        {
            LOG_ERROR("Failed to allocate script commands");
            status = ENOMEM;
        }
        else
        {
            for (int i = 0; i < script->num; i++)
            {
                if (NULL == (command = json_array_get_string(array, i)))
                {
                    LOG_ERROR("Failed to get command %d", i);
                    status = EINVAL;
                    break;
                }
                else {
                    if (NULL == (script->commands[i] = strdup(command)))
                    {
                        LOG_ERROR("Failed to allocate command %d", i);
                        status = ENOMEM;
                        break;
                    }
                }
            }
        }
    }
    else if (NULL != (command = json_value_get_string(object)))
    {
        script->num = 1;
        if (NULL == (script->commands = calloc(script->num, sizeof(char*))))
        {
            LOG_ERROR("Failed to allocate script commands");
            status = ENOMEM;
        }
        else
        {
            if (NULL == (script->commands[0] = strdup(command)))
            {
                LOG_ERROR("Failed to allocate command");
                status = ENOMEM;
            }
        }
    }

    return status;
}

int ParseStep(const JSON_Object* object, STEP* step)
{
    int status = 0;
    JSON_Value* value = NULL;

    step->delay = json_object_get_number(object, "delay");

    if (NULL != (value = json_object_get_value(object, "run")))
    {
        step->type = RUN;
        if (0 != (status = ParseScript(value, &step->data.script)))
        {
            LOG_ERROR("Failed to parse script");
        }
    }
    else if (NULL != (value = json_object_get_value(object, "type")))
    {
        step->type = MODULE;
        if (0 != (status = ParseTest(object, &step->data.test)))
        {
            LOG_ERROR("Failed to parse test");
        }
    }
    else
    {
        LOG_ERROR("Step must contain either 'run' or 'type' field");
        status = EINVAL;
    }

    return status;
}

int ParseDefinition(const char* path, SCRIPT* setup, SCRIPT* teardown, char*** moduleNames, int* numModules, STEP** steps, int* numSteps)
{
    int status = 0;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Array* moduleArray = NULL;
    JSON_Value* setupValue = NULL;
    JSON_Value* teardownValue = NULL;
    JSON_Array* stepArray = NULL;
    JSON_Object* stepObject = NULL;

    if (NULL == (rootValue = json_parse_file(path)))
    {
        LOG_ERROR("Failed to parse test definition file: %s", path);
        status = EINVAL;
    }
    else if (NULL == (rootObject = json_value_get_object(rootValue)))
    {
        LOG_ERROR("Root element is not an object: %s", path);
        status = EINVAL;
    }
    else if (NULL == (stepArray = json_object_get_array(rootObject, "steps")))
    {
        LOG_ERROR("No 'steps' array found: %s", path);
        status = EINVAL;
    }
    else if (NULL == (moduleArray = json_object_get_array(rootObject, "modules")))
    {
        LOG_ERROR("No 'modules' array found: %s", path);
        status = EINVAL;
    }
    else if ((NULL != (setupValue = json_object_get_value(rootObject, "setup"))) && (0 != ParseScript(setupValue, setup)))
    {
        LOG_ERROR("Failed to parse 'setup' script: %s", path);
        status = EINVAL;
    }
    else if ((NULL != (teardownValue = json_object_get_value(rootObject, "teardown"))) && (0 != ParseScript(teardownValue, teardown)))
    {
        LOG_ERROR("Failed to parse 'teardown' script: %s", path);
        status = EINVAL;
    }
    else
    {
        *numSteps = (int)json_array_get_count(stepArray);

        if (NULL == (*steps = calloc(*numSteps, sizeof(STEP))))
        {
            status = ENOMEM;
        }
        else
        {
            for (int i = 0; i < *numSteps; i++)
            {
                stepObject = json_array_get_object(stepArray, i);

                if (0 != (status = ParseStep(stepObject, &(*steps)[i])))
                {
                    LOG_ERROR("Failed to parse step at index: %d", i);
                    status = EINVAL;
                    break;
                }
            }

            *numModules = (int)json_array_get_count(moduleArray);

            if (NULL == (*moduleNames = calloc(*numModules, sizeof(char*))))
            {
                LOG_ERROR("Failed to allocate module names array");
                status = ENOMEM;
            }
            else
            {
                for (int i = 0; i < *numModules; i++)
                {
                    if (NULL == ((*moduleNames)[i] = strdup(json_array_get_string(moduleArray, i))))
                    {
                        LOG_ERROR("Failed to allocate module name");
                        status = ENOMEM;
                        break;
                    }
                }
            }

            if (status != 0)
            {
                for (int i = 0; i < *numSteps; i++)
                {
                    FreeStep(&(*steps)[i]);
                }

                FREE_MEMORY(*steps);
                *numSteps = 0;

                for (int i = 0; i < *numModules; i++)
                {
                    FREE_MEMORY(moduleNames[i]);
                }

                FREE_MEMORY(*moduleNames);
            }
        }
    }

    json_value_free(rootValue);

    return status;
}

int RunScript(const SCRIPT* script)
{
    int status = 0;
    char* textResult = NULL;

    if (script != NULL)
    {
        for (int i = 0; i < script->num; i++)
        {
            printf("$ %s\n", script->commands[i]);

            if (0 != (status = ExecuteCommand(NULL, script->commands[i], false, false, 0, 0, &textResult, NULL, NULL)))
            {
                printf("failed with status: %d: %s\n", status, textResult);
                break;
            }
            else
            {
                if (textResult != NULL)
                {
                    printf("%s\n", textResult);
                }
            }

            FREE_MEMORY(textResult);
        }
    }

    return status;
}

MANAGEMENT_MODULE* FindModule(const char* component, const MANAGEMENT_MODULE* modules, int numModules)
{
    MANAGEMENT_MODULE* module = NULL;
    char** components = NULL;

    for (int i = 0; i < numModules; i++)
    {
        components = modules[i].info->components;
        for (int j = 0; j < (int)modules[i].info->componentCount; j++)
        {
            if (0 == strcmp(component, components[j]))
            {
                module = (MANAGEMENT_MODULE*)&modules[i];
                break;
            }
        }
    }

    return module;
}

int RunModuleTest(const TEST* test, const MANAGEMENT_MODULE* modules, int numModules)
{
    int result = 0;
    JSON_Value* value = NULL;
    char* payloadString = NULL;
    MMI_JSON_STRING payload = NULL;
    int size = 0;
    int mmiStatus = 0;

    printf("[%s] %s.%s\n", (test->type == REPORTED) ? "reported" : ((test->type == DESIRED) ? "desired" : "--"), test->component, test->object);

    MANAGEMENT_MODULE* module = FindModule(test->component, modules, numModules);

    if (module == NULL)
    {
        LOG_ERROR("Failed to find module for component: '%s'", test->component);

        if (test->expect.status != EINVAL)
        {
            result = EINVAL;
        }
    }
    else
    {
        switch (test->type)
        {
        case REPORTED:
            mmiStatus = module->get(module->session, test->component, test->object, &payload, &size);

            if (MMI_OK == mmiStatus)
                {
                    if (NULL == (payloadString = calloc(size + 1, sizeof(char))))
                    {
                        result = ENOMEM;
                    }
                    else
                    {
                        memcpy(payloadString, payload, size);
                        if (NULL == (value = json_parse_string(payloadString)))
                        {
                            printf("[FAILURE] Failed to parse JSON payload: %s\n", payloadString);
                            result = EINVAL;
                        }
                    }
                }

                if (test->expect.value != NULL)
                {
                    if (value != NULL)
                    {
                        if (!json_value_equals(test->expect.value, value))
                        {
                            printf("[FAILURE]\n\texpected:\n\t\t%s\n\tactual:\n\t\t%s\n", json_serialize_to_string(test->expect.value), json_serialize_to_string(value));
                            result = -1;
                        }
                    }
                    else
                    {
                        printf("[FAILURE]\n\texpected:\n\t\t%s\n\tactual:\n\t\tnull\n", json_serialize_to_string(test->expect.value));
                        result = -1;
                    }
                }

                if (test->expect.status != mmiStatus)
                {
                    printf("[FAILURE] expected status '%d', actual '%d'\n", test->expect.status, mmiStatus);
                    result = -1;
                }
                break;

            case DESIRED:
                if (test->value != NULL)
                {
                    payload = json_serialize_to_string(test->value);
                    size = (int)strlen(payload);
                }

                mmiStatus = module->set(module->session, test->component, test->object, payload, size);

                if (test->expect.status != mmiStatus)
                {
                    printf("[FAILURE] expected status '%d', actual'%d'\n", test->expect.status, mmiStatus);
                    result = -1;
                }

                break;

            default:
                LOG_ERROR("Unknown test type '%d', skipping test", test->type);
                break;
        }
    }

    return result;
}

long long CurrentMilliseconds()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long milliseconds = time.tv_sec * 1000LL + time.tv_usec / 1000;
    return milliseconds;
}

int RunTest(const char* client, const char* path, const char* bin)
{
    int status = 0;
    int failures = 0;
    int total = 0;
    int numModules = 0;
    SCRIPT setup = { 0 };
    SCRIPT teardown = { 0 };
    char** moduleNames = NULL;
    MANAGEMENT_MODULE* modules = NULL;
    STEP* steps = NULL;
    TEST* test = NULL;
    long long start = 0;
    long long end = 0;

    printf("\n");
    printf("test definition: %s\n", path);
    printf("\n");

    if ((0 == (status = ParseDefinition(path, &setup, &teardown, &moduleNames, &numModules, &steps, &total))))
    {
        printf("client: '%s'\n", client);
        printf("bin: %s\n", bin);
        printf("\n");
        printf("modules:\n");

        for (int i = 0; i < numModules; i++)
        {
            printf("  - %s\n", moduleNames[i]);
        }

        printf("\n");

        if (setup.num > 0)
        {
            printf("setup:\n");
            status = RunScript(&setup);
        }

        // Load the modules
        if (status == 0)
        {
            if (NULL == (modules = calloc(numModules, sizeof(MANAGEMENT_MODULE))))
            {
                LOG_ERROR("Failed to allocate memory for modules");
                status = ENOMEM;
            }
            else
            {
                for (int i = 0; i < numModules; i++)
                {
                    if (0 != (status = LoadModule(client, bin, moduleNames[i], &modules[i])))
                    {
                        LOG_ERROR("Failed to load module: %s", moduleNames[i]);
                        break;
                    }
                }
            }
        }

        start = CurrentMilliseconds();

        if (status == 0)
        {
            printf("\n");
            printf("running %d tests...\n", total);
            printf("\n");

            for (int i = 0; i < total; i++)
            {
                STEP* step = &steps[i];

                if (step->delay > 0)
                {
                    sleep(step->delay);
                }

                printf("test %d/%d\n", i + 1, total);

                switch (step->type)
                {
                case RUN:
                    printf("script:\n");
                    if (0 != (status = RunScript(&step->data.script)))
                    {
                        printf("[FAILURE] script failed\n");
                        failures++;
                    }
                    break;

                case MODULE:
                    test = &step->data.test;
                    failures += (0 == RunModuleTest(test, modules, numModules)) ? 0 : 1;
                    break;

                default:
                    LOG_ERROR("Unknown step type '%d', skipping step", step->type);
                    break;
                }

                printf("\n");
            }

            end = CurrentMilliseconds();

            // Unload the modules
            for (int i = 0; i < numModules; i++)
            {
                UnloadModule(&modules[i]);
            }

            if (teardown.num > 0)
            {
                printf("\n");
                printf("teardown:\n");
                status = RunScript(&teardown);
            }

            printf("\n");
            printf("summary: %s (%d ms)\n", failures == 0 ? "PASSED" : "FAILURE", (int)(end - start));
            printf("  passed: %d\n", total - failures);
            printf("  failed: %d\n", failures);
            printf("  total: %d\n", total);
            printf("\n");
        }
    }

    FREE_MEMORY(moduleNames);
    FREE_MEMORY(steps);

    return status;
}

int GetClientName(char** client)
{
    int status = 0;
    int version = 0;
    JSON_Value* config = NULL;
    JSON_Object* config_object = NULL;

    if (NULL == (config = json_parse_file(OSCONFIG_CONFIG_FILE)))
    {
        LOG_ERROR("Failed to parse %s\n", OSCONFIG_CONFIG_FILE);
        status = EINVAL;
    }
    else if (NULL == (config_object = json_value_get_object(config)))
    {
        LOG_ERROR("Failed to get config object\n");
        status = EINVAL;
    }
    else if (0 == (version = json_object_get_number(config_object, "ModelVersion")))
    {
        LOG_ERROR("Failed to get model version\n");
        status = EINVAL;
    }
    else
    {
        *client = (char*)calloc(strlen(AZURE_OSCONFIG) + strlen(OSCONFIG_VERSION) + 5, sizeof(char));
        if (NULL == *client)
        {
            LOG_ERROR("Failed to allocate memory for client name\n");
            status = ENOMEM;
        }
        else
        {
            sprintf(*client, "%s %d;%s", AZURE_OSCONFIG, version, OSCONFIG_VERSION);
        }
    }

    if (config != NULL)
    {
        json_value_free(config);
    }

    return status;
}

void usage(const char* executable)
{
    printf("usage: %s <file>... [options]\n", executable);
    printf("\n");
    printf("options:\n");
    printf("  --bin <path>  path to load modules from (default: %s)\n", DEFAULT_BIN_PATH);
    printf("  --verbose     enable verbose logging\n");
    printf("  --help        display this help and exit\n");
}

int main(int argc, char const* argv[])
{
    int result = EXIT_SUCCESS;

    char* client = NULL;
    const char* bin = NULL;
    struct stat path_stat;
    int numFiles = 0;

    if (argc < 2)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            break;
        }

        if (stat(argv[i], &path_stat) != 0)
        {
            printf("file not found: %s\n", argv[i]);
            return EXIT_FAILURE;
        }

        if (!S_ISREG(path_stat.st_mode))
        {
            printf("'%s' is not a file\n", argv[i]);
            return EXIT_FAILURE;
        }

        numFiles++;
    }

    for (int i = numFiles + 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--bin") == 0)
        {
            if (i + 1 < argc)
            {
                bin = argv[++i];
            }
            else
            {
                printf("missing argument for --bin\n");
                result = EXIT_FAILURE;
                break;
            }
        }
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            g_verbose = true;
            SetFullLogging(true);
            SetCommandLogging(true);
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        else
        {
            printf("unknown option: %s\n", argv[i]);
            result = EXIT_FAILURE;
            break;
        }
    }

    if (bin == NULL)
    {
        bin = DEFAULT_BIN_PATH;
    }

    if (result == EXIT_SUCCESS)
    {
        if (0 != (result = GetClientName(&client)))
        {
            printf("failed to get client name\n");
        }
        else
        {
            for (int i = 0; i < numFiles; i++)
            {
                if (0 != (result = RunTest(client, argv[i + 1], bin)))
                {
                    break;
                }

                if (i + 1 < numFiles)
                {
                    printf("\n");
                    printf("============================================================\n");
                    printf("\n");
                }
            }

        }
    }

    FREE_MEMORY(client);

    return result;
}
