// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>

#include <ScopeGuard.h>
#include <Mmi.h>
#include <Ztsi.h>

static const std::string g_ztsiConfigFile = "/etc/sim-agent-edge/config.json";

bool IsValidClientName(const char* name)
{
    // "Azure OSConfig <model version>;<major>.<minor>.<patch>.<yyyymmdd><build>"
    const std::string productInfoTemplate = "^((Azure OSConfig )([0-9]+);(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.([0-9]{8})).*$";
    const std::string clientNamePrefix = "Azure OSConfig ";
    const std::string modelVersionDelimiter = ";";
    const std::string semanticVersionDelimeter = ".";

    // OSConfig model version 5 published on September 27, 2021
    const int referenceModelVersion = 5;
    const int referenceReleaseDay = 27;
    const int referenceReleaseMonth = 9;
    const int referenceReleaseYear = 2021;

    // String length of date string yyyymmmdd
    const int dateLength = 9;

    bool isValid = true;

    const std::string clientName = name;

    // Regex for validating the client name against the OSConfig product info
    std::regex pattern(productInfoTemplate);

    if (!clientName.empty() && std::regex_match(clientName, pattern))
    {
        std::string versionInfo = clientName.substr(clientNamePrefix.length());
        std::string modelVersion = versionInfo.substr(0, versionInfo.find(modelVersionDelimiter));

        int modelVersionNumber = std::stoi(modelVersion);
        if (modelVersionNumber < referenceModelVersion)
        {
            isValid = false;
        }

        // Get build date from versionInfo
        int position = 0;
        for (int i = 0; i < 3; i++)
        {
            position = versionInfo.find(semanticVersionDelimeter, position + 1);
        }

        std::string buildDate = versionInfo.substr(position + 1, position + dateLength);
        int year = std::stoi(buildDate.substr(0, 4));
        int month = std::stoi(buildDate.substr(4, 2));
        int day = std::stoi(buildDate.substr(6, 2));

        if ((month < 1) || (month > 12) || (day < 1) || (day > 31))
        {
            isValid = false;
        }

        char dateNow[dateLength] = { 0 };
        int monthNow = 0, dayNow = 0, yearNow = 0;
        time_t t = time(0);
        strftime(dateNow, dateLength, "%Y%m%d", localtime(&t));
        sscanf(dateNow, "%4d%2d%2d", &yearNow, &monthNow, &dayNow);

        // Check if the build date is in the future
        if ((yearNow < year) || ((yearNow == year) && ((monthNow < month) || ((monthNow == month) && (dayNow < day)))))
        {
            isValid = false;
        }

        // Check if the build date is from the past - before the reference release date
        if ((year < referenceReleaseYear) || ((year == referenceReleaseYear) && ((month < referenceReleaseMonth) || ((month == referenceReleaseMonth) && (day < referenceReleaseDay)))))
        {
            isValid = false;
        }
    }
    else
    {
        isValid = false;
    }

    return isValid;
}

void __attribute__((constructor)) InitModule()
{
    ZtsiLog::OpenLog();
    OsConfigLogInfo(ZtsiLog::Get(), "Ztsi module loaded");
}

void __attribute__((destructor)) DestroyModule()
{
    OsConfigLogInfo(ZtsiLog::Get(), "Ztsi module unloaded");
    ZtsiLog::CloseLog();
}

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo(%s, %.*s, %d) returned %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGetInfo(%s, -, %d) returned %d", clientName, *payloadSizeBytes, status);
            }
        }
    }};

    status = Ztsi::GetInfo(clientName, payload, payloadSizeBytes);

    return status;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSizeBytes)
{
    int status = MMI_OK;
    MMI_HANDLE handle = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            OsConfigLogInfo(ZtsiLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
        else
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiOpen(%s, %d) returned: %p, status: %d", clientName, maxPayloadSizeBytes, handle, status);
        }
    }};

    if (nullptr == clientName)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiOpen called with null clientName");
        status = EINVAL;
    }
    else if (!IsValidClientName(clientName))
    {
        status = EINVAL;
    }
    else
    {
        try
        {
            Ztsi* ztsi = new (std::nothrow) Ztsi(g_ztsiConfigFile, maxPayloadSizeBytes);
            if (nullptr == ztsi)
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiOpen failed to allocate memory");
                status = ENOMEM;
            }
            else
            {
                handle = reinterpret_cast<MMI_HANDLE>(ztsi);
            }
        }
        catch (std::exception& e)
        {
            OsConfigLogError(ZtsiLog::Get(), "MmiOpen exception thrown: %s", e.what());
            status = EINTR;
        }
    }

    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    Ztsi* ztsi = reinterpret_cast<Ztsi*>(clientSession);
    if (nullptr != ztsi)
    {
        delete ztsi;
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    int status = MMI_OK;
    Ztsi* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (MMI_OK == status)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
        }
        else
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiSet(%p, %s, %s, -, %d) returned %d", clientSession, componentName, objectName, payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiSet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<Ztsi*>(clientSession);
        status = session->Set(componentName, objectName, payload, payloadSizeBytes);
    }

    return status;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    int status = MMI_OK;
    Ztsi* session = nullptr;

    ScopeGuard sg{[&]()
    {
        if (IsFullLoggingEnabled())
        {
            if (MMI_OK == status)
            {
                OsConfigLogInfo(ZtsiLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
            else
            {
                OsConfigLogError(ZtsiLog::Get(), "MmiGet(%p, %s, %s, %.*s, %d) returned %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
            }
        }
    }};

    if (nullptr == clientSession)
    {
        OsConfigLogError(ZtsiLog::Get(), "MmiGet called with null clientSession");
        status = EINVAL;
    }
    else
    {
        session = reinterpret_cast<Ztsi*>(clientSession);
        status = session->Get(componentName, objectName, payload, payloadSizeBytes);
    }


    return status;
}

void MmiFree(MMI_JSON_STRING payload)
{
    if (nullptr != payload)
    {
        delete[] payload;
    }
}

