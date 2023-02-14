// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>

#include "AnsibleUtils.h"

#define PYTHON_ENVIRONMENT "/etc/osconfig/python"
#define PYTHON_EXECUTABLE "python3"
#define PYTHON_PIP_DEPENDENCY "pip"
#define PYTHON_VENV_DEPENDENCY "venv"
#define PYTHON_PACKAGE "ansible-core"
#define ANSIBLE_EXECUTABLE "ansible"
#define ANSIBLE_GALAXY_EXECUTABLE "ansible-galaxy"
#define ANSIBLE_DEFAULT_COLLECTION "ansible.builtin"

static const char* g_checkPythonCommand = "which " PYTHON_EXECUTABLE;
static const char* g_checkPythonPipCommand = PYTHON_EXECUTABLE " -m " PYTHON_PIP_DEPENDENCY " --version";
static const char* g_checkPythonVenvCommand = PYTHON_EXECUTABLE " -m " PYTHON_VENV_DEPENDENCY " -h";
static const char* g_checkPythonEnviromentCommand = PYTHON_EXECUTABLE " -m " PYTHON_VENV_DEPENDENCY " " PYTHON_ENVIRONMENT;
static const char* g_checkPythonPackageCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; " PYTHON_EXECUTABLE " -m " PYTHON_PIP_DEPENDENCY " install " PYTHON_PACKAGE "'";
static const char* g_checkAnsibleCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; which " ANSIBLE_EXECUTABLE "'";
static const char* g_checkAnsibleGalaxyCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; which " ANSIBLE_GALAXY_EXECUTABLE "'";

static const char* g_getPythonVersionCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; " PYTHON_EXECUTABLE " --version' | grep 'Python ' | cut -d ' ' -f 2 | tr -d '\n'";
static const char* g_getPythonLocationCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; which " PYTHON_EXECUTABLE "' | tr -d '\n'";
static const char* g_getAnsibleVersionCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; " ANSIBLE_EXECUTABLE " --version' | grep '" ANSIBLE_EXECUTABLE " \\[core ' | cut -d ' ' -f 3 | tr -d ']\n'";
static const char* g_getAnsibleLocationCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; which " ANSIBLE_EXECUTABLE "' | tr -d '\n'";
static const char* g_getAnsibleGalaxyLocationCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; which " ANSIBLE_GALAXY_EXECUTABLE "' | tr -d '\n'";

static const char* g_runAnsibleModuleCommand = "sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; " ANSIBLE_EXECUTABLE " localhost -m %s.%s -a \"%s\" -o 2> /dev/null' | grep -o '{.*'";
static const char* g_runAnsibleGalaxyCommand = " sh -c '. " PYTHON_ENVIRONMENT "/bin/activate; " ANSIBLE_GALAXY_EXECUTABLE " collection install %s'";

int AnsibleCheckDependencies(void* log)
{
    int status = 0;
    char* pythonVersion = NULL;
    char* pythonLocation = NULL;
    char* ansibleVersion = NULL;
    char* ansibleLocation = NULL;
    char* ansibleGalaxyLocation = NULL;

    if (0 != ExecuteCommand(NULL, g_checkPythonCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Python executable '%s'", PYTHON_EXECUTABLE);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkPythonPipCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Python dependency '%s'", PYTHON_PIP_DEPENDENCY);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkPythonVenvCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Python dependency '%s'", PYTHON_VENV_DEPENDENCY);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkPythonEnviromentCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Python environment '%s'", PYTHON_ENVIRONMENT);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkPythonPackageCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Python package '%s'", PYTHON_PACKAGE);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkAnsibleCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Ansible executable '%s'", ANSIBLE_EXECUTABLE);
        }
        status = EINVAL;
    }
    else if (0 != ExecuteCommand(NULL, g_checkAnsibleGalaxyCommand, false, false, 0, 0, NULL, NULL, log))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find Ansible executable '%s'", ANSIBLE_GALAXY_EXECUTABLE);
        }
        status = EINVAL;
    }
    else if ((0 != ExecuteCommand(NULL, g_getPythonVersionCommand, false, false, 0, 0, &pythonVersion, NULL, log)) ||
        (0 != ExecuteCommand(NULL, g_getPythonLocationCommand, false, false, 0, 0, &pythonLocation, NULL, log)) ||
        (0 != ExecuteCommand(NULL, g_getAnsibleVersionCommand, false, false, 0, 0, &ansibleVersion, NULL, log)) ||
        (0 != ExecuteCommand(NULL, g_getAnsibleLocationCommand, false, false, 0, 0, &ansibleLocation, NULL, log)) || 
        (0 != ExecuteCommand(NULL, g_getAnsibleGalaxyLocationCommand, false, false, 0, 0, &ansibleGalaxyLocation, NULL, log)))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckDependencies() failed to find dependency information");
        }
        status = EINVAL;
    }
    else
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(log, "AnsibleCheckDependencies() found Python executable ('%s', '%s')", pythonVersion, pythonLocation);
            OsConfigLogInfo(log, "AnsibleCheckDependencies() found Ansible executables ('%s', '%s', '%s')", ansibleVersion, ansibleLocation, ansibleGalaxyLocation);
        }
    }

    FREE_MEMORY(pythonVersion);
    FREE_MEMORY(pythonLocation);
    FREE_MEMORY(ansibleVersion);
    FREE_MEMORY(ansibleLocation);
    FREE_MEMORY(ansibleGalaxyLocation);

    return status;
}

int AnsibleCheckCollection(const char* collectionName, void* log)
{
    int status = 0;
    char* commandBuffer = NULL;
    int commandBufferSizeBytes = 0;

    if (NULL == collectionName)
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleCheckCollection(%s) called with invalid arguments", collectionName);
        }
        status = EINVAL;
    }
    else if (0 == strcmp(collectionName, ANSIBLE_DEFAULT_COLLECTION))
    {
        // NOOP.
    }
    else 
    {
        commandBufferSizeBytes = snprintf(NULL, 0, g_runAnsibleGalaxyCommand, collectionName);
        commandBuffer = malloc(commandBufferSizeBytes + 1);
        
        if (NULL != commandBuffer)
        {
            memset(commandBuffer, 0, commandBufferSizeBytes + 1);
            snprintf(commandBuffer, commandBufferSizeBytes + 1, g_runAnsibleGalaxyCommand, collectionName);

            if ((0 != ExecuteCommand(NULL, commandBuffer, false, false, 0, 0, NULL, NULL, log)))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(log, "AnsibleCheckCollection(%s) failed to execute command '%s'", collectionName, commandBuffer);
                }
                status = EINVAL;
            }
        }
        else 
        { 
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "AnsibleCheckCollection(%s) failed to allocate %d bytes", collectionName, commandBufferSizeBytes + 1);
            }
            status = EINVAL;
        }
    }

    FREE_MEMORY(commandBuffer);

    return status;
}

int AnsibleExecuteModule(const char* collectionName, const char* moduleName, const char* moduleArguments, char** result, void* log)
{
    int status = 0;
    char* commandBuffer = NULL;
    int commandBufferSizeBytes = 0;
    
    if ((NULL == collectionName) || (NULL == moduleName))
    {
        if (IsFullLoggingEnabled())
        {
            OsConfigLogError(log, "AnsibleExecuteModule(%s, %s, %s, %p) called with invalid arguments", collectionName, moduleName, moduleArguments, result);
        }
        status = EINVAL;
    }
    else 
    {
        commandBufferSizeBytes = snprintf(NULL, 0, g_runAnsibleModuleCommand, collectionName, moduleName, ((NULL == moduleArguments) ? "" : moduleArguments));
        commandBuffer = malloc(commandBufferSizeBytes + 1);

        if (NULL != commandBuffer)
        {
            memset(commandBuffer, 0, commandBufferSizeBytes + 1);
            snprintf(commandBuffer, commandBufferSizeBytes + 1, g_runAnsibleModuleCommand, collectionName, moduleName, ((NULL == moduleArguments) ? "" : moduleArguments));

            if ((0 != ExecuteCommand(NULL, commandBuffer, false, false, 0, 0, result, NULL, log)))
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(log, "AnsibleExecuteModule(%s, %s, %s, %p) failed to execute command '%s'", collectionName, moduleName, moduleArguments, result, commandBuffer);
                }
                status = EINVAL;
            }
        }
        else 
        { 
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(log, "AnsibleExecuteModule(%s, %s, %s, %p) failed to allocate %d bytes", collectionName, moduleName, moduleArguments, result, commandBufferSizeBytes + 1);
            }
            status = EINVAL;
        }
    }

    FREE_MEMORY(commandBuffer);

    return status;
}