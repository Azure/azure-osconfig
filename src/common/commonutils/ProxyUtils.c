// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static void RemoveProxyStringEscaping(char* value)
{
    int i = 0;
    int j = 0;

    if (NULL == value)
    {
        return;
    }

    int length = strlen(value);

    for (i = 0; i < length - 1; i++)
    {
        if (('\\' == value[i]) && ('@' == value[i + 1]))
        {
            for (j = i; j < length - 1; j++)
            {
                value[j] = value[j + 1];
            }
            length -= 1;
            value[length] = 0;
        }
    }
}

bool ParseHttpProxyData(const char* proxyData, char** proxyHostAddress, int* proxyPort, char** proxyUsername, char** proxyPassword, void* log)
{
    // We accept the proxy data string to be in one of two following formats:
    //
    // "http://server:port"
    // "http://username:password@server:port"
    //
    // ..where the prefix must be either lowercase "http" or uppercase "HTTP"
    // ..and username and password can contain '@' characters escaped as "\\@"
    //
    // For example:
    //
    // "http://username\\@mail.foo:p\\@ssw\\@rd@server:port" where username is "username@mail.foo" and password is "p@ssw@rd"

    const char httpPrefix[] = "http://";
    const char httpUppercasePrefix[] = "HTTP://";

    int proxyDataLength = 0;
    int prefixLength = 0;
    bool isBadAlphaNum = false;
    int credentialsSeparatorCounter = 0;
    int columnCounter = 0;

    char* credentialsSeparator = NULL;
    char* firstColumn = NULL;
    char* lastColumn = NULL;

    char* afterPrefix = NULL;
    char* hostAddress = NULL;
    char* port = NULL;
    char* username = NULL;
    char* password = NULL;

    int hostAddressLength = 0;
    int portLength = 0;
    int portNumber = 0;
    int usernameLength = 0;
    int passwordLength = 0;

    int i = 0;

    bool result = false;

    if ((NULL == proxyData) || (NULL == proxyHostAddress) || (NULL == proxyPort))
    {
        OsConfigLogError(log, "ParseHttpProxyData called with invalid arguments");
        return result;
    }

    // Initialize output arguments

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

    // Check for required prefix and invalid characters and if any found then immediately fail

    proxyDataLength = strlen(proxyData);
    prefixLength = strlen(httpPrefix);
    if (proxyDataLength <= prefixLength)
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), too short", proxyData);
        return NULL;
    }

    if (strncmp(proxyData, httpPrefix, prefixLength) && strncmp(proxyData, httpUppercasePrefix, strlen(httpUppercasePrefix)))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), no %s prefix", proxyData, httpPrefix);
        return NULL;
    }

    for (i = 0; i < proxyDataLength; i++)
    {
        if (('.' == proxyData[i]) || ('/' == proxyData[i]) || ('\\' == proxyData[i]) || ('_' == proxyData[i]) ||
            ('-' == proxyData[i]) || ('$' == proxyData[i]) || ('!' == proxyData[i]) || (isalnum(proxyData[i])))
        {
            continue;
        }
        else if ('@' == proxyData[i])
        {
            // not valid as first character
            if (0 == i)
            {
                OsConfigLogError(log, "Unsupported proxy data (%s), invalid '@' prefix", proxyData);
                isBadAlphaNum = true;
                break;
            }
            // '\@' can be used to insert '@' characters in username or password
            else if ((i > 0) && ('\\' != proxyData[i - 1]))
            {
                if (NULL == credentialsSeparator)
                {
                    credentialsSeparator = (char*)&(proxyData[i]);
                }
                credentialsSeparatorCounter += 1;
                if (credentialsSeparatorCounter > 1)
                {
                    OsConfigLogError(log, "Unsupported proxy data (%s), too many '@' characters", proxyData);
                    isBadAlphaNum = true;
                    break;
                }
            }
        }
        else if (':' == proxyData[i])
        {
            columnCounter += 1;
            if (columnCounter > 3)
            {
                OsConfigLogError(log, "Unsupported proxy data (%s), too many ':' characters", proxyData);
                isBadAlphaNum = true;
                break;
            }
        }
        else
        {
            OsConfigLogError(log, "Unsupported proxy data (%s), unsupported character '%c' at position %d", proxyData, proxyData[i], i);
            isBadAlphaNum = true;
            break;
        }
    }

    if ((0 == columnCounter) && (false == isBadAlphaNum))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s), missing ':'", proxyData);
        isBadAlphaNum = true;
    }

    if (isBadAlphaNum)
    {
        return NULL;
    }

    afterPrefix = (char*)(proxyData + prefixLength);
    firstColumn = strchr(afterPrefix, ':');
    lastColumn = strrchr(afterPrefix, ':');

    // If the '@' credentials separator is not already found, try the first one if any
    if (NULL == credentialsSeparator)
    {
        credentialsSeparator = strchr(afterPrefix, '@');
    }

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
        (afterPrefix >= firstColumn) ||
        (firstColumn > lastColumn) ||
        (credentialsSeparator && (firstColumn >= credentialsSeparator)) ||
        (credentialsSeparator && (credentialsSeparator >= lastColumn)) ||
        (credentialsSeparator && (firstColumn == lastColumn)) ||
        (credentialsSeparator && (0 == strlen(credentialsSeparator))) ||
        ((credentialsSeparator ? strlen("A:A@A:A") : strlen("A:A")) > strlen(afterPrefix)) ||
        (1 > strlen(lastColumn)) ||
        (1 > strlen(firstColumn)))
    {
        OsConfigLogError(log, "Unsupported proxy data (%s) format", afterPrefix);
    }
    else
    {
        {
            if (credentialsSeparator)
            {
                // username:password@server:port
                usernameLength = (int)(firstColumn - afterPrefix - 1);
                if (usernameLength > 0)
                {
                    if (NULL != (username = (char*)malloc(usernameLength + 1)))
                    {
                        memcpy(username, afterPrefix, usernameLength);
                        username[usernameLength] = 0;

                        RemoveProxyStringEscaping(username);
                        usernameLength = strlen(username);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.username: %d", errno);
                    }
                }

                passwordLength = (int)(credentialsSeparator - firstColumn - 1);
                if (passwordLength > 0)
                {
                    if (NULL != (password = (char*)malloc(passwordLength + 1)))
                    {
                        memcpy(password, firstColumn, passwordLength);
                        password[passwordLength] = 0;

                        RemoveProxyStringEscaping(password);
                        passwordLength = strlen(password);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.password: %d", errno);
                    }
                }

                hostAddressLength = (int)(lastColumn - credentialsSeparator - 1);
                if (hostAddressLength > 0)
                {
                    if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                    {
                        memcpy(hostAddress, credentialsSeparator, hostAddressLength);
                        hostAddress[hostAddressLength] = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                    }
                }

                portLength = (int)strlen(lastColumn);
                if (portLength > 0)
                {
                    if (NULL != (port = (char*)malloc(portLength + 1)))
                    {
                        memcpy(port, lastColumn, portLength);
                        port[portLength] = 0;
                        portNumber = strtol(port, NULL, 10);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                    }
                }
            }
            else
            {
                // server:port
                hostAddressLength = (int)(firstColumn - afterPrefix - 1);
                if (hostAddressLength > 0)
                {
                    if (NULL != (hostAddress = (char*)malloc(hostAddressLength + 1)))
                    {
                        memcpy(hostAddress, afterPrefix, hostAddressLength);
                        hostAddress[hostAddressLength] = 0;
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.host_address: %d", errno);
                    }
                }

                portLength = (int)strlen(firstColumn);
                if (portLength > 0)
                {
                    if (NULL != (port = (char*)malloc(portLength + 1)))
                    {
                        memcpy(port, firstColumn, portLength);
                        port[portLength] = 0;
                        portNumber = strtol(port, NULL, 10);
                    }
                    else
                    {
                        OsConfigLogError(log, "Cannot allocate memory for HTTP_PROXY_OPTIONS.port string copy: %d", errno);
                    }
                }
            }

            *proxyHostAddress = hostAddress;
            *proxyPort = portNumber;

            if (proxyUsername && proxyPassword)
            {
                *proxyUsername = username;
                *proxyPassword = password;

            }

            OsConfigLogInfo(log, "HTTP proxy host|address: %s (%d)", *proxyHostAddress, hostAddressLength);
            OsConfigLogInfo(log, "HTTP proxy port: %d", *proxyPort);

            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(log, "HTTP proxy username: %s (%d)", *proxyUsername, usernameLength);
                OsConfigLogInfo(log, "HTTP proxy password: %s (%d)", *proxyPassword, passwordLength);
            }

            FREE_MEMORY(port);

            result = true;
        }
    }

    return result;
}
