// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Common.h>
#include <Module.h>

#define DEFAULT_BIN_PATH "/usr/lib/osconfig"
#define OSCONFIG_CONFIG_FILE "/etc/osconfig/osconfig.json"

#define AZURE_OSCONFIG "Azure OSConfig"

#define RECIPE_ACTION "Action"
#define RECIPE_LOAD_MODULE "LoadModule"
#define RECIPE_UNLOAD_MODULE "UnloadModule"
#define RECIPE_MODULE "Module"

#define RECIPE_TYPE "Type"
#define RECIPE_REPORTED "Reported"
#define RECIPE_DESIRED "Desired"
#define RECIPE_COMPONENT "Component"
#define RECIPE_OBJECT "Object"
#define RECIPE_PAYLOAD "Payload"
#define RECIPE_JSON "Json"
#define RECIPE_STATUS "Status"
#define RECIPE_DELAY "Delay"

#define RECIPE_RUN_COMMAND "RunCommand"

typedef enum STEP_TYPE
{
    MODULE = 0,
    COMMAND,
    TEST
} STEP_TYPE;

typedef enum PAYLOAD_TYPE
{
    DESIRED = 0,
    REPORTED
} PAYLOAD_TYPE;

typedef struct TEST_STEP
{
    PAYLOAD_TYPE type;
    char* component;
    char* object;
    JSON_Value* payload;
    int status;
} TEST_STEP;

typedef enum MODULE_ACTION
{
    LOAD = 0,
    UNLOAD
} MODULE_ACTION;

typedef struct MODULE_STEP
{
    MODULE_ACTION action;
    char* name;
} MODULE_STEP;

typedef struct COMMAND_STEP
{
    int status;
    char* arguments;
} COMMAND_STEP;

typedef struct STEP
{
    STEP_TYPE type;
    int delay;
    union
    {
        MODULE_STEP module;
        COMMAND_STEP command;
        TEST_STEP test;
    } data;
} STEP;

static bool g_verbose = false;

void FreeStep(STEP* step)
{
    if (step)
    {
        return;
    }

    switch (step->type)
    {
        case MODULE:
            free(step->data.module.name);
            break;
        case COMMAND:
            free(step->data.command.arguments);
            break;
        case TEST:
            free(step->data.test.component);
            free(step->data.test.object);
            json_value_free(step->data.test.payload);
        default:
            break;
    }
}

int ParseTestStep(const JSON_Object* object, TEST_STEP* test)
{
    int status = 0;
    const char* type = NULL;
    JSON_Value* payload = NULL;
    const char* json = NULL;

    if (NULL == (type = json_object_get_string(object, RECIPE_TYPE)))
    {
        LOG_ERROR("Missing '%s' from test step", RECIPE_TYPE);
        status = EINVAL;
    }
    else
    {
        if (strcmp(type, RECIPE_REPORTED) == 0)
        {
            test->type = REPORTED;
        }
        else if (strcmp(type, RECIPE_DESIRED) == 0)
        {
            test->type = DESIRED;
        }
        else
        {
            status = EINVAL;
        }

        if (NULL == (test->component = strdup(json_object_get_string(object, RECIPE_COMPONENT))))
        {
            LOG_ERROR("'%s' is required for '%s' test step", RECIPE_COMPONENT, type);
            status = EINVAL;
        }

        if (NULL == (test->object = strdup(json_object_get_string(object, RECIPE_OBJECT))))
        {
            LOG_ERROR("'%s' is required for '%s' test", RECIPE_OBJECT, type);
            status = EINVAL;
        }

        if (NULL == (payload = json_object_get_value(object, RECIPE_PAYLOAD)))
        {
            if (NULL != (json = json_object_get_string(object, RECIPE_JSON)))
            {
                payload = json_parse_string(json);
            }
        }

        test->payload = payload ? json_value_deep_copy(payload) : NULL;
        test->status = json_object_get_number(object, RECIPE_STATUS);
    }

    return status;
}

int ParseCommandStep(const JSON_Object* object, COMMAND_STEP* command)
{
    int status = 0;
    JSON_Value* value = NULL;
    const char* arguments = NULL;

    if (command == NULL)
    {
        LOG_ERROR("Invalid (null) command");
        return EINVAL;
    }

    if (NULL == (value = json_object_get_value(object, RECIPE_RUN_COMMAND)))
    {
        LOG_ERROR("Missing '%s' from command step", RECIPE_RUN_COMMAND);
        status = EINVAL;
    }
    else if (JSONString != json_value_get_type(value))
    {
        LOG_ERROR("'%s' must be a string", RECIPE_RUN_COMMAND);
        status = EINVAL;
    }
    else if (NULL == (arguments = json_value_get_string(value)))
    {
        LOG_ERROR("Failed to serialize '%s' from command step", RECIPE_RUN_COMMAND);
        status = EINVAL;
    }
    else if (NULL == (command->arguments = strdup(arguments)))
    {
        LOG_ERROR("Failed to copy '%s' from command step", RECIPE_RUN_COMMAND);
        status = ENOMEM;
    }

    command->status = json_object_get_number(object, "Status");

    return status;
}

int ParseModuleStep(const JSON_Object* object, MODULE_STEP* module)
{
    int status = 0;
    const char* action = NULL;

    if (module == NULL)
    {
        LOG_ERROR("Invalid (null) module");
        status = EINVAL;
    }
    else
    {
        if (NULL == (action = json_object_get_string(object, RECIPE_ACTION)))
        {
            LOG_ERROR("Missing '%s' from module step", RECIPE_ACTION);
            status = EINVAL;
        }
        else
        {
            if (strcmp(action, RECIPE_LOAD_MODULE) == 0)
            {
                module->action = LOAD;
            }
            else if (strcmp(action, RECIPE_UNLOAD_MODULE) == 0)
            {
                module->action = UNLOAD;
            }
            else
            {
                LOG_ERROR("Invalid action '%s'", action);
                status = EINVAL;
            }
        }

        if ((status == 0) && (module->action == LOAD))
        {
            if (NULL == (module->name = strdup(json_object_get_string(object, RECIPE_MODULE))))
            {
                LOG_ERROR("'%s' is required to load a module", RECIPE_MODULE);
                status = EINVAL;
            }
        }
    }

    return status;
}

int ParseStep(const JSON_Object* object, STEP* step)
{
    int status = 0;
    JSON_Value* value = NULL;

    step->delay = json_object_get_number(object, RECIPE_DELAY);

    if (NULL != (value = json_object_get_value(object, RECIPE_RUN_COMMAND)))
    {
        step->type = COMMAND;
        if (0 != (status = ParseCommandStep(object, &step->data.command)))
        {
            LOG_ERROR("Failed to parse command step: %s", json_serialize_to_string(value));
        }
    }
    else if (NULL != (value = json_object_get_value(object, RECIPE_TYPE)))
    {
        step->type = TEST;
        if (0 != (status = ParseTestStep(object, &step->data.test)))
        {
            LOG_ERROR("Failed to parse test step: %s", json_serialize_to_string(value));
        }
    }
    else if (NULL != (value = json_object_get_value(object, RECIPE_ACTION)))
    {
        step->type = MODULE;
        if (0 != (status = ParseModuleStep(object, &step->data.module)))
        {
            LOG_ERROR("Failed to parse module step: %s", json_serialize_to_string(value));
        }
    }
    else
    {
        status = EINVAL;
    }

    return status;
}

int ParseRecipe(const char* path, STEP** steps, int* numSteps)
{
    int status = 0;
    JSON_Value* rootValue = NULL;
    JSON_Array* stepArray = NULL;
    JSON_Object* stepObject = NULL;

    if (NULL == (rootValue = json_parse_file(path)))
    {
        LOG_ERROR("Failed to parse test definition file: %s", path);
        status = EINVAL;
    }
    else if (NULL == (stepArray = json_value_get_array(rootValue)))
    {
        LOG_ERROR("Root element is not an array: %s", path);
        status = EINVAL;
    }
    else
    {
        *numSteps = json_array_get_count(stepArray);

        if (NULL == (*steps = calloc(*numSteps, sizeof(STEP))))
        {
            LOG_ERROR("Failed to allocate memory for test steps");
            status = ENOMEM;
        }
        else
        {
            for (int i = 0; i < *numSteps; i++)
            {
                stepObject = json_array_get_object(stepArray, i);

                if (0 != (status = ParseStep(stepObject, &(*steps)[i])))
                {
                    LOG_ERROR("Failed to parse step %d", i);
                    break;
                }
            }
        }
    }

    json_value_free(rootValue);

    return status;
}

int RunCommand(const COMMAND_STEP* command)
{
    int status = 0;
    char* textResult = NULL;

    if (command != NULL)
    {
        printf("$ %s\n", command->arguments);

        if (command->status != ExecuteCommand(NULL, command->arguments, false, false, 0, 0, &textResult, NULL, NULL))
        {
            printf("Command exited with status: %d (expected %d): %s\n", status, command->status, textResult);
            status = -1;
        }
        else if (textResult != NULL)
        {
            printf("%s\n", textResult);
        }

        FREE_MEMORY(textResult);
    }

    return status;
}

int RunTestStep(const TEST_STEP* test, const MANAGEMENT_MODULE* module)
{
    int result = 0;
    JSON_Value* value = NULL;
    MMI_JSON_STRING payload = NULL;
    char* payloadString = NULL;
    int payloadSize = 0;
    int mmiStatus = 0;

    if (test == NULL)
    {
        LOG_ERROR("Invalid (null) test step");
        result = EINVAL;
    }
    else if (module == NULL)
    {
        LOG_ERROR("Invalid (null) management module");
        result = EINVAL;
    }
    else if (test->type == REPORTED)
    {
        mmiStatus = module->get(module->session, test->component, test->object, &payload, &payloadSize);

        if (MMI_OK == mmiStatus)
        {
            if (NULL == (payloadString = calloc(payloadSize + 1, sizeof(char))))
            {
                result = ENOMEM;
            }
            else
            {
                memcpy(payloadString, payload, payloadSize);
                if (NULL == (value = json_parse_string(payloadString)))
                {
                    printf("[FAILURE] Failed to parse JSON payload: %s\n", payloadString);
                    result = EINVAL;
                }
            }
        }

        if (test->payload != NULL)
        {
            if (value != NULL)
            {
                if (!json_value_equals(test->payload, value))
                {
                    printf("[FAILURE]\n\texpected:\n\t\t%s\n\tactual:\n\t\t%s\n", json_serialize_to_string(test->payload), json_serialize_to_string(value));
                    result = -1;
                }
            }
            else
            {
                printf("[FAILURE]\n\texpected:\n\t\t%s\n\tactual:\n\t\tnull\n", json_serialize_to_string(test->payload));
                result = -1;
            }
        }

        if (test->status != mmiStatus)
        {
            printf("[FAILURE] expected status '%d', actual '%d'\n", test->status, mmiStatus);
            result = -1;
        }
    }
    else if (test->type == DESIRED)
    {
        if (test->payload != NULL)
        {
            payloadString = json_serialize_to_string(test->payload);
            payloadSize = (int)strlen(payloadString);
        }

        mmiStatus = module->set(module->session, test->component, test->object, payloadString, payloadSize);

        if (test->status != mmiStatus)
        {
            printf("[FAILURE] expected status '%d', actual '%d'\n", test->status, mmiStatus);
            result = -1;
        }
    }
    else
    {
        printf("[FAILURE] Invalid test step type: %d\n", test->type);
        result = EINVAL;
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

int InvokeRecipe(const char* client, const char* path, const char* bin)
{
    int status = 0;
    int failures = 0;
    int skipped = 0;
    int total = 0;
    long long start = 0;
    long long end = 0;
    MANAGEMENT_MODULE* module = NULL;
    STEP* steps = NULL;

    printf("\n");
    printf("test recipe: %s\n", path);
    printf("\n");

    if ((0 == (status = ParseRecipe(path, &steps, &total))))
    {
        printf("client: '%s'\n", client);
        printf("bin: %s\n", bin);
        printf("\n");

        start = CurrentMilliseconds();

        if (status == 0)
        {
            printf("running %d steps...\n", total);
            printf("\n");

            for (int i = 0; i < total; i++)
            {
                STEP* step = &steps[i];

                if (step->delay > 0)
                {
                    sleep(step->delay);
                }

                printf("%d/%d ", i + 1, total);

                switch (step->type)
                {
                    case COMMAND:
                        printf("[COMMAND]\n");
                        failures += (0 == RunCommand(&step->data.command)) ? 0 : 1;
                        break;

                    case TEST:
                        printf("[TEST] %s %s.%s\n", step->data.test.type == REPORTED ? "reported" : "desired", step->data.test.component, step->data.test.object);

                        if (module == NULL)
                        {
                            LOG_ERROR("No module loaded, skipping test step: %d", i);
                            skipped++;
                        }
                        else
                        {
                            failures += (0 == RunTestStep(&step->data.test, module)) ? 0 : 1;
                        }
                        break;

                    case MODULE:
                        printf("[MODULE] %s %s\n", step->data.module.action == LOAD ? "load" : "unload", (step->data.module.name == NULL ? "" : step->data.module.name));

                        if (module == NULL)
                        {
                            if (step->data.module.action == LOAD)
                            {
                                if (NULL == (module = LoadModule(client, bin, step->data.module.name)))
                                {
                                    LOG_ERROR("Failed to load module '%s'", step->data.module.name);
                                    failures++;
                                }
                            }
                            else
                            {
                                LOG_ERROR("No module loaded, skipping module load step: %d", i);
                                skipped++;
                            }
                        }
                        else
                        {
                            if (step->data.module.action == UNLOAD)
                            {
                                UnloadModule(module);
                                module = NULL;
                            }
                            else
                            {
                                LOG_ERROR("No module loaded, skipping module load step: %d", i);
                                skipped++;
                            }
                        }
                        break;

                    default:
                        LOG_ERROR("Unknown step type '%d', skipping step", step->type);
                        skipped++;
                        break;
                }

                printf("\n");
            }

            end = CurrentMilliseconds();

            if (module != NULL)
            {
                LOG_INFO("Module is still loaded, unloading...");
                UnloadModule(module);
                module = NULL;
            }

            printf("summary: %s\n", failures == 0 ? "PASSED" : "FAILURE");
            printf("  passed: %d\n", total - failures);
            printf("  skipped: %d\n", skipped);
            printf("  failed: %d\n", failures);
            printf("  total: %d (%d ms)\n", total, (int)(end - start));
            printf("\n");
        }
    }

    FREE_MEMORY(steps);

    return status;
}

int GetClientName(char** client)
{
    int status = 0;
    int version = 0;
    JSON_Value* config = NULL;
    JSON_Object* configObject = NULL;

    if (NULL == (config = json_parse_file(OSCONFIG_CONFIG_FILE)))
    {
        LOG_ERROR("Failed to parse %s\n", OSCONFIG_CONFIG_FILE);
        status = EINVAL;
    }
    else if (NULL == (configObject = json_value_get_object(config)))
    {
        LOG_ERROR("Failed to get config object\n");
        status = EINVAL;
    }
    else if (0 == (version = json_object_get_number(configObject, "ModelVersion")))
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

void Usage(const char* executable)
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
    struct stat pathStat;
    int numFiles = 0;

    if (argc < 2)
    {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            break;
        }

        if (stat(argv[i], &pathStat) != 0)
        {
            printf("file not found: %s\n", argv[i]);
            return EXIT_FAILURE;
        }

        if (!S_ISREG(pathStat.st_mode))
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
            Usage(argv[0]);
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
                if (0 != (result = InvokeRecipe(client, argv[i + 1], bin)))
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
