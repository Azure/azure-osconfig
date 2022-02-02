// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/ProxyUtils.h"

static HTTP_PROXY_OPTIONS* g_proxyOptions = NULL;

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

HTTP_PROXY_OPTIONS* ParseHttpProxyData(char* proxyData)
{
    // We accept the proxy data string to be in one of two following formats:
    // "http://server:port"
    // "http://username:password@server:port"

    const char httpPrefix[] = "http://";

    HTTP_PROXY_OPTIONS* proxyOptions = NULL;
    
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

    if (NULL == proxyData)
    {
        return NULL;
    }

    if (strlen(proxyData) <=  strlen(httpPrefix))
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
            (credentialsSeparator && (0 == strlen(credentialsSeparator)) ||
            ((credentialsSeparator ? strlen("A:A@A:A") : strlen("A:A"))) >= strlen(proxyData)) ||
            (1 >= strlen(lastColumn)) ||
            (1 >= strlen(firstColumn)))
        {
            LogErrorWithTelemetry(GetLog(), "Unsupported proxy data (%s) format", proxyData);
        }
        else
        {
            if (NULL != (proxyOptions = (HTTP_PROXY_OPTIONS*)malloc(sizeof(HTTP_PROXY_OPTIONS))))
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

                proxyOptions->host_address = hostAddress;
                proxyOptions->port = portNumber;
                proxyOptions->username = username;
                proxyOptions->password = password;

                OsConfigLogInfo(GetLog(), "Proxy host|address: %s (%d)", proxyOptions->host_address, hostAddressLength);
                OsConfigLogInfo(GetLog(), "Proxy port: %d (%s, %d)", proxyOptions->port, port, portLength);
                OsConfigLogInfo(GetLog(), "Proxy username: %s (%d)", proxyOptions->username, usernameLength);
                OsConfigLogInfo(GetLog(), "Proxy password: %s (%d)", proxyOptions->password, passwordLength);

                // Port is unused past this, can be freed; the rest must remain allocated
                FREE_MEMORY(port);
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "Cannot allocate memory for HTTP_PROXY_OPTIONS: %d", errno);
            }
        }
    }

    return proxyOptions;
}