// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/ProxyUtils.h"

char* GetHttpProxyData()
{
    const char* proxyVariables[] = {
        "http_proxy",
        "https_proxy",
        "HTTP_PROXY",
        "HTTPS_PROXY"
    };
    int proxyVariablesSize = ARRAY_SIZE(proxyVariables);

    char* proxyData = NULL;
    char* environmentVariable = NULL;
    int i = 0;

    for (i = 0; i < proxyVariablesSize; i++)
    {
        environmentVariable = getenv(proxyVariables[i]);
        if (NULL != environmentVariable)
        {
            // The environment variable string must be treated as read-only, make a copy for our use:
            proxyData = strdup(environmentVariable);
            if (NULL == proxyData)
            {
                LogErrorWithTelemetry(GetLog(), "Cannot make a copy of proxy data (%s): %d", environmentVariable, errno);
            }
            else
            {
                OsConfigLogInfo(GetLog(), "Proxy data from %s: %s", proxyVariables[i], proxyData);
            }
            break;
        }
    }

    return proxyData;
}

bool ParseHttpProxyData(char* proxyData, char** proxyHostAddress, int* proxyPort, char** proxyUsername, char** proxyPassword)
{
    // We accept the proxy data string to be in one of two following formats:
    // "http://server:port"
    // "http://username:password@server:port"

    const char httpPrefix[] = "http://";

    char* credentialsSeparator = NULL;
    char* firstColumn = NULL;
    char* lastColumn = NULL;

    char* hostAddress = NULL;
    char* port = NULL;
    char* username = NULL;
    char* password = NULL;

    int hostAddressLength = 0;
    int portLength = 0;
    int portNumber = 0;
    int usernameLength = 0;
    int passwordLength = 0;

    bool result = false;

    if ((NULL == proxyData) || (NULL == proxyHostAddress) || (NULL == proxyPort))
    {
        LogErrorWithTelemetry(GetLog(), "ParseHttpProxyData called with invalid arguments");
        return NULL;
    }

    *proxyHostAddress = NULL;
    *proxyPort = 0;
    
    if (proxyUsername)
    {
        *proxyUsername = NULL;
    }

    if (proxyPassword)
    {
        *proxyPassword = NULL;
    }

    if (strlen(proxyData) <= strlen(httpPrefix))
    {
        LogErrorWithTelemetry(GetLog(), "Unsupported proxy data (%s), too short", proxyData);
    }
    else if (0 != strncmp(proxyData, httpPrefix, strlen(httpPrefix)))
    {
        LogErrorWithTelemetry(GetLog(), "Unsupported proxy data (%s), no %s prefix", proxyData, httpPrefix);
    }
    else
    {
        proxyData += strlen(httpPrefix);

        firstColumn = strchr(proxyData, ':');
        lastColumn = strrchr(proxyData, ':');
        credentialsSeparator = strchr(proxyData, '@');

        // If found, bump over the first character that is the separator itself

        if (firstColumn && (strlen(firstColumn) > 0))
        {
            firstColumn += 1;
        }

        if (lastColumn && (strlen(lastColumn) > 0))
        {
            lastColumn += 1;
        }

        if (credentialsSeparator && (strlen(credentialsSeparator) > 0))
        {
            credentialsSeparator += 1;
        }

        if ((proxyData >= firstColumn) ||
            (firstColumn > lastColumn) ||
            (credentialsSeparator && (firstColumn >= credentialsSeparator)) ||
            (credentialsSeparator && (credentialsSeparator >= lastColumn)) ||
            (credentialsSeparator && (firstColumn == lastColumn)) ||
            (credentialsSeparator && (0 == strlen(credentialsSeparator))) ||
            ((credentialsSeparator ? strlen("A:A@A:A") : strlen("A:A")) >= strlen(proxyData)) ||
            (1 >= strlen(lastColumn)) ||
            (1 >= strlen(firstColumn)))
        {
            LogErrorWithTelemetry(GetLog(), "Unsupported proxy data (%s) format", proxyData);
        }
        else
        {
            {
                if (credentialsSeparator)
                {
                    // username:password@server:port
                    usernameLength = (int)(firstColumn - proxyData - 1);
                    if (usernameLength > 0)
                    {
                        if (NULL != (username = (char*)malloc(usernameLength + 1)))
                        {
                            strncpy(username, proxyData, usernameLength);
                            username[usernameLength] = 0;
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.username: %d", errno);
                        }
                    }

                    passwordLength = (int)(credentialsSeparator - firstColumn - 1);
                    if (passwordLength > 0)
                    {
                        if (NULL != (password = (char*)malloc(passwordLength + 1)))
                        {
                            strncpy(password, firstColumn, passwordLength);
                            password[passwordLength] = 0;
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.password: %d", errno);
                        }
                    }

                    hostAddressLength = (int)(lastColumn - credentialsSeparator - 1);
                    if (hostAddressLength > 0)
                    {
                        if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                        {
                            strncpy(hostAddress, credentialsSeparator, hostAddressLength);
                            hostAddress[hostAddressLength] = 0;
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                        }
                    }

                    portLength = (int)strlen(lastColumn);
                    if (portLength > 0)
                    {
                        if (NULL != (port = (char*)malloc(portLength + 1)))
                        {
                            strncpy(port, lastColumn, hostAddressLength);
                            portNumber = strtol(port, NULL, 10);
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                        }
                    }
                }
                else
                {
                    // server:port
                    hostAddressLength = (int)(firstColumn - proxyData - 1);
                    if (hostAddressLength > 0)
                    {
                        if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                        {
                            strncpy(hostAddress, proxyData, hostAddressLength);
                            hostAddress[hostAddressLength] = 0;
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                        }
                    }

                    portLength = (int)strlen(firstColumn);
                    if (portLength > 0)
                    {
                        if (NULL != (port = (char*)malloc(portLength + 1)))
                        {
                            strncpy(port, firstColumn, hostAddressLength);
                            portNumber = strtol(port, NULL, 10);
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                        }
                    }
                }

                *proxyHostAddress = hostAddress;
                *proxyPort = portNumber;
                
                if ((NULL != proxyUsername) && (NULL != proxyPassword))
                {
                    *proxyUsername = username;
                    *proxyPassword = password;
                }

                OsConfigLogInfo(GetLog(), "Proxy host|address: %s (%d)", *proxyHostAddress, hostAddressLength);
                OsConfigLogInfo(GetLog(), "Proxy port: %d (%s, %d)", *proxyPort, port, portLength);
                OsConfigLogInfo(GetLog(), "Proxy username: %s (%d)", *proxyUsername, usernameLength);
                OsConfigLogInfo(GetLog(), "Proxy password: %s (%d)", *proxyPassword, passwordLength);

                // Port is unused past this, can be freed; the rest must remain allocated
                FREE_MEMORY(port);

                result = true;
            }
        }
    }

    return result;
}