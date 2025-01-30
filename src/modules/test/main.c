// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Common.h>
#include <Module.h>
#include <math.h>

#define DEFAULT_BIN_PATH "/usr/lib/osconfig"
#define OSCONFIG_CONFIG_FILE "/etc/osconfig/osconfig.json"

#define AZURE_OSCONFIG "Azure OSConfig"

#define RECIPE_ACTION "Action"
#define RECIPE_LOAD_MODULE "LoadModule"
#define RECIPE_UNLOAD_MODULE "UnloadModule"
#define RECIPE_MODULE "Module"

#define RECIPE_TYPE "ObjectType"
#define RECIPE_REPORTED "Reported"
#define RECIPE_DESIRED "Desired"
#define RECIPE_COMPONENT "ComponentName"
#define RECIPE_OBJECT "ObjectName"
#define RECIPE_PAYLOAD "Payload"
#define RECIPE_PAYLOAD_SIZE_BYTES "PayloadSizeBytes"
#define RECIPE_JSON "Json"
#define RECIPE_STATUS "ExpectedResult"
#define RECIPE_WAIT_SECONDS "WaitSeconds"
#define SECURITY_BASELINE "SecurityBaseline"

#define RECIPE_RUN_COMMAND "RunCommand"

#define LINE_SEPARATOR "--------------------------------------------------------------------------------"
#define LINE_SEPARATOR_THICK "================================================================================"

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
    char* payload;
    int payloadSize;
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

typedef struct FAILURE
{
    int index;
    char* name;
} FAILURE;

static bool g_verbose = false;

void FreeStep(STEP* step)
{
    if (NULL == step)
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
            free(step->data.test.payload);
            break;
        default:
            LOG_ERROR("Unknown step type: %d", step->type);
    }
}

bool ParseTestStep(const JSON_Object* object, TEST_STEP* test)
{
    bool parseError = false;
    const char* type = NULL;
    const char* componentName = NULL;
    const char* objectName = NULL;
    JSON_Value* payload = NULL;
    const char* json = NULL;

    if (NULL == (type = json_object_get_string(object, RECIPE_TYPE)))
    {
        LOG_ERROR("Missing '%s' from test step", RECIPE_TYPE);
        parseError = true;
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
            parseError = true;
        }

        if (NULL == (componentName = json_object_get_string(object, RECIPE_COMPONENT)))
        {
            LOG_ERROR("'%s' is required for '%s' test step", RECIPE_COMPONENT, type);
            parseError = true;
        }
        else if (NULL == (test->component = strdup(componentName)))
        {
            LOG_ERROR("Failed to copy '%s' for '%s' test step", RECIPE_COMPONENT, type);
            parseError = true;
        }

        if (NULL == (objectName = json_object_get_string(object, RECIPE_OBJECT)))
        {
            LOG_ERROR("'%s' is required for '%s' test step", RECIPE_OBJECT, type);
            parseError = true;
        }
        else if (NULL == (test->object = strdup(objectName)))
        {
            LOG_ERROR("Failed to copy '%s' for '%s' test step", RECIPE_OBJECT, type);
            parseError = true;
        }

        if (NULL != (payload = json_object_get_value(object, RECIPE_PAYLOAD)))
        {
            test->payload = json_serialize_to_string(payload);
        }
        else if (NULL != (json = json_object_get_string(object, RECIPE_JSON)))
        {
            test->payload = strdup(json);
        }

        if (NULL != json_object_get_value(object, RECIPE_PAYLOAD_SIZE_BYTES))
        {
            test->payloadSize = json_object_get_number(object, RECIPE_PAYLOAD_SIZE_BYTES);
        }
        else
        {
            test->payloadSize = test->payload ? (strlen(test->payload)) : 0;
        }

        test->status = json_object_get_number(object, RECIPE_STATUS);
    }

    return !parseError;
}

bool ParseCommandStep(const JSON_Object* object, COMMAND_STEP* command)
{
    bool parseError = false;
    JSON_Value* value = NULL;
    const char* arguments = NULL;

    if (NULL == (value = json_object_get_value(object, RECIPE_RUN_COMMAND)))
    {
        LOG_ERROR("Missing '%s' from command step", RECIPE_RUN_COMMAND);
        parseError = true;
    }
    else if (JSONString != json_value_get_type(value))
    {
        LOG_ERROR("'%s' must be a string", RECIPE_RUN_COMMAND);
        parseError = true;
    }
    else if (NULL == (arguments = json_value_get_string(value)))
    {
        LOG_ERROR("Failed to serialize '%s' from command step", RECIPE_RUN_COMMAND);
        parseError = true;
    }
    else if (NULL == (command->arguments = strdup(arguments)))
    {
        LOG_ERROR("Failed to copy '%s' from command step", RECIPE_RUN_COMMAND);
        parseError = true;
    }

    command->status = json_object_get_number(object, "ExpectedResult");

    return !parseError;
}

bool ParseModuleStep(const JSON_Object* object, MODULE_STEP* module)
{
    bool parseError = false;
    const char* action = NULL;
    const char* name = NULL;

    if (NULL == (action = json_object_get_string(object, RECIPE_ACTION)))
    {
        LOG_ERROR("Missing '%s' from module step", RECIPE_ACTION);
        parseError = true;
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
            parseError = true;
        }
    }

    if ((!parseError) && (module->action == LOAD))
    {
        if (NULL == (name = json_object_get_string(object, RECIPE_MODULE)))
        {
            LOG_ERROR("Missing '%s' from module step", RECIPE_MODULE);
            parseError = true;
        }
        else if (NULL == (module->name = strdup(name)))
        {
            LOG_ERROR("Failed to copy '%s' from module step", RECIPE_MODULE);
            parseError = true;
        }
    }

    return !parseError;
}

bool ParseStep(const JSON_Object* object, STEP* step)
{
    int status = 0;
    JSON_Value* value = NULL;

    step->delay = json_object_get_number(object, RECIPE_WAIT_SECONDS);

    if (NULL != (value = json_object_get_value(object, RECIPE_RUN_COMMAND)))
    {
        step->type = COMMAND;
        if (!ParseCommandStep(object, &step->data.command))
        {
            LOG_ERROR("Failed to parse command step: %s", json_serialize_to_string(value));
            status = EINVAL;
        }
    }
    else if (NULL != (value = json_object_get_value(object, RECIPE_TYPE)))
    {
        step->type = TEST;
        if (!ParseTestStep(object, &step->data.test))
        {
            LOG_ERROR("Failed to parse test step: %s", json_serialize_to_string(value));
            status = EINVAL;
        }
    }
    else if (NULL != (value = json_object_get_value(object, RECIPE_ACTION)))
    {
        step->type = MODULE;
        if (!ParseModuleStep(object, &step->data.module))
        {
            LOG_ERROR("Failed to parse module step: %s", json_serialize_to_string(value));
            status = EINVAL;
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
        if (command->status != (status = ExecuteCommand(NULL, command->arguments, false, false, 0, 0, &textResult, NULL, NULL)))
        {
            LOG_ERROR("Command exited with status: %d (expected %d): %s", status, command->status, textResult);
            status = (0 != status) ? status : -1;
        }
        else if (textResult != NULL)
        {
            LOG_INFO("%s", textResult);
        }

        FREE_MEMORY(textResult);
    }

    return status;
}

int RunTestStep(const TEST_STEP* test, const MANAGEMENT_MODULE* module)
{
    const char* skippedAudits[] = {
        "auditEnsureKernelSupportForCpuNx",
        "auditEnsureDefaultDenyFirewallPolicyIsSet",
        "auditEnsureAuthenticationRequiredForSingleUserMode",
        "auditEnsureAllBootloadersHavePasswordProtectionEnabled"
        // Add here more audit checks that need to be temporarily disabled during investigation
    };
    int numSkippedAudits = ARRAY_SIZE(skippedAudits);

    const char* skippedRemediations[] = {
        // Add here remediation checks that need to be temporarily disabled during investigation
    };
    int numSkippedRemediations = ARRAY_SIZE(skippedRemediations);

    const char* audit = "audit";
    const char* remediate = "remediate";
    const char* reason = NULL;
    JSON_Value* actualJsonValue = NULL;
    JSON_Value* expectedJsonValue = NULL;
    MMI_JSON_STRING payload = NULL;
    char* payloadString = NULL;
    bool asbAudit = false;
    bool asbRemediation = false;
    int payloadSize = 0;
    int i = 0;
    int mmiStatus = 0;
    int result = 0;

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
                if (NULL == (actualJsonValue = json_parse_string(payloadString)))
                {
                    LOG_ERROR("Failed to parse JSON payload: %s", payloadString);
                    result = EINVAL;
                }
                else if ((0 == strcmp(test->component, SECURITY_BASELINE)) &&
                    (0 == strncmp(test->object, audit, strlen(audit))))
                {
                    asbAudit = true;

                    for (i = 0; i < numSkippedAudits; i++)
                    {
                        if (0 == strcmp(test->object, skippedAudits[i]))
                        {
                            asbAudit = false;
                            break;
                        }
                    }
                }
            }
        }

        if (test->payload || asbAudit)
        {
            if (asbAudit)
            {
                if (NULL == (reason = json_value_get_string(actualJsonValue)))
                {
                    LOG_ERROR("Assertion failed, json_value_get_string('%s') failed", json_serialize_to_string(actualJsonValue));
                    result = -1;
                }
                else if (0 != strncmp(reason, SECURITY_AUDIT_PASS, strlen(SECURITY_AUDIT_PASS)))
                {
                    LOG_ERROR("Assertion failed, expected: '%s...', actual: '%s'", SECURITY_AUDIT_PASS, reason);
                    result = EFAULT;
                }
                else
                {
                    LOG_INFO("Assertion passed with reason: '%s'", reason + strlen(SECURITY_AUDIT_PASS));
                }
            }
            else if (actualJsonValue != NULL)
            {
                if (NULL == (expectedJsonValue = json_parse_string(test->payload)))
                {
                    LOG_ERROR("Failed to parse expected JSON payload: %s", test->payload);
                    result = EFAULT;
                }
                else if (0 == json_value_equals(expectedJsonValue, actualJsonValue))
                {
                    LOG_ERROR("Assertion failed, expected: '%s', actual: '%s'",
                        json_serialize_to_string(expectedJsonValue), json_serialize_to_string(actualJsonValue));
                    result = EFAULT;
                }
            }
            else
            {
                LOG_ERROR("Assertion failed, expected: '%s', actual: (null)", test->payload);
                result = EFAULT;
            }
        }

        if (test->status != mmiStatus)
        {
            LOG_ERROR("Assertion failed, expected result '%d', actual '%d'", test->status, mmiStatus);
            result = EFAULT;
        }

        json_value_free(expectedJsonValue);
        json_value_free(actualJsonValue);
        FREE_MEMORY(payloadString);
        FREE_MEMORY(payload);
    }
    else if (test->type == DESIRED)
    {
        if (test->status != (mmiStatus = module->set(module->session, test->component, test->object, test->payload, test->payloadSize)))
        {
            if ((0 == strcmp(test->component, SECURITY_BASELINE)) && (0 == strncmp(test->object, remediate, strlen(remediate))))
            {
                asbRemediation = true;

                for (i = 0; i < numSkippedRemediations; i++)
                {
                    if (0 == strcmp(test->object, skippedRemediations[i]))
                    {
                        asbRemediation = false;
                        break;
                    }
                }
            }

            if (false == asbRemediation)
            {
                LOG_INFO("Assertion passed, actual result '%d', component '%s' and object '%s'", mmiStatus, test->component, test->object);
                result = 0;
            }
            else
            {
                LOG_ERROR("Assertion failed, expected result '%d', actual '%d'", test->status, mmiStatus);
                result = EFAULT;
            }
        }
    }
    else
    {
        LOG_ERROR("Invalid test step type: %d", test->type);
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
    int failed = 0;
    int skipped = 0;
    int total = 0;
    long long start = 0;
    long long end = 0;
    char* modulePath = NULL;
    MANAGEMENT_MODULE* module = NULL;
    STEP* steps = NULL;
    FAILURE* failures = NULL;
    STEP* step = NULL;
    char* name = NULL;

    LOG_INFO("Test recipe: %s", path);

    if ((0 == (status = ParseRecipe(path, &steps, &total))))
    {
        LOG_INFO("Client: '%s'", client);
        LOG_INFO("Bin: %s", bin);
        LOG_TRACE(LINE_SEPARATOR_THICK);

        failures = calloc(total, sizeof(FAILURE));

        if (NULL == failures)
        {
            LOG_ERROR("Failed to allocate memory for failed steps");
            status = ENOMEM;
        }

        start = CurrentMilliseconds();

        if (status == 0)
        {
            LOG_INFO("Running %d steps...", total);
            LOG_TRACE(LINE_SEPARATOR);

            for (int i = 0; i < total; i++)
            {
                step = &steps[i];

                if (step->delay > 0)
                {
                    sleep(step->delay);
                }

                LOG_INFO("Step %d of %d", i + 1, total);

                switch (step->type)
                {
                    case COMMAND:
                        LOG_INFO("Executing command '%s'", step->data.command.arguments);
                        failed += (0 != RunCommand(&step->data.command)) ? 1 : 0;
                        break;

                    case TEST:
                        LOG_INFO("Running %s test '%s.%s'", step->data.test.type == REPORTED ? "reported" : "desired", step->data.test.component, step->data.test.object);
                        if (module == NULL)
                        {
                            LOG_ERROR("No module loaded, skipping test step: %d", i);
                            skipped++;
                        }
                        else if (0 != RunTestStep(&step->data.test, module))
                        {
                            if (NULL == (name = calloc(strlen(step->data.test.component) + strlen(step->data.test.object) + 2, sizeof(char))))
                            {
                                LOG_ERROR("Failed to allocate memory for test name");
                            }
                            else
                            {
                                sprintf(name, "%s.%s", step->data.test.component, step->data.test.object);
                                failures[failed].name = name;
                                failures[failed].index = i;
                            }

                            failed++;
                        }
                        break;

                    case MODULE:
                        LOG_INFO("%s module...", step->data.module.action == LOAD ? "Loading" : "Unloading");

                        if (module == NULL)
                        {
                            if (step->data.module.action == LOAD)
                            {
                                if (NULL == (modulePath = calloc(strlen(bin) + strlen(step->data.module.name) + 2, sizeof(char))))
                                {
                                    LOG_ERROR("Failed to allocate memory for module path");
                                    failed++;
                                }
                                else
                                {
                                    sprintf(modulePath, "%s/%s", bin, step->data.module.name);

                                    if (NULL == (module = LoadModule(client, modulePath)))
                                    {
                                        LOG_ERROR("Failed to load module '%s'", step->data.module.name);
                                        failed++;
                                    }

                                    FREE_MEMORY(modulePath);
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
                                FREE_MEMORY(module);
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

                if (i < total - 1)
                {
                    LOG_TRACE(LINE_SEPARATOR);
                }
            }

            end = CurrentMilliseconds();

            if (module != NULL)
            {
                LOG_INFO("Warning: module is still loaded, unloading...");
                UnloadModule(module);
                FREE_MEMORY(module);
                module = NULL;
            }

            if (failed > 0)
            {
                LOG_TRACE(LINE_SEPARATOR_THICK);
                LOG_TRACE("Failed tests:");
            }

            for (int i = 0; i < failed; i++)
            {
                LOG_TRACE("  %*d %s", (int)log10(total) + 1, failures[i].index + 1, failures[i].name);
                FREE_MEMORY(failures[i].name);
            }

            LOG_TRACE(LINE_SEPARATOR_THICK);
            LOG_TRACE("summary: %s", failed == 0 ? "PASSED" : "FAILED");
            LOG_TRACE("  passed: %d", total - failed);
            LOG_TRACE("  skipped: %d", skipped);
            LOG_TRACE("  failed: %d", failed);
            LOG_TRACE("  total: %d (%d ms)", total, (int)(end - start));
            LOG_TRACE(LINE_SEPARATOR_THICK);
        }
    }

    if (failed > 0)
    {
        status = 1;
    }

    for (int i = 0; i < total; i++)
    {
        FreeStep(&steps[i]);
    }
    FREE_MEMORY(failures);
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
