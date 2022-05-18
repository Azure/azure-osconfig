// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <PlatformCommon.h>
#include <ManagementModule.h>
#include <MockManagementModule.h>

namespace Tests
{
    class MockHandle {};

    MockManagementModule::MockManagementModule() :
        ManagementModule()
    {
        this->MmiGetInfo([](const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes) -> int
            {
                (void)clientName;

                static constexpr const char mockInfo[] = R"""({
                    "Name": "Mock Management Module",
                    "Description": "This is a mocked module",
                    "Manufacturer": "Microsoft",
                    "VersionMajor": 1,
                    "VersionMinor": 0,
                    "VersionInfo": "",
                    "Components": ["TestModule_Component_1"],
                    "Lifetime": 2,
                    "UserAccount": 0})""";

                std::size_t len = strlen(mockInfo);
                *payloadSizeBytes = len;
                *payload = new char[len];
                std::memcpy(*payload, mockInfo, len);

                return MMI_OK;
            });

        this->MmiOpen([](const char* clientName, const unsigned int maxPayloadSizeBytes) -> MMI_HANDLE
            {
                (void)clientName;
                (void)maxPayloadSizeBytes;
                return reinterpret_cast<MMI_HANDLE>(new MockHandle());
            });

        this->MmiClose([](MMI_HANDLE handle)
            {
                if (nullptr != handle)
                {
                    delete reinterpret_cast<MockHandle*>(handle);
                }
            });

        this->MmiSet([](MMI_HANDLE handle, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
            {
                (void)handle;
                (void)componentName;
                (void)objectName;
                (void)payload;
                (void)payloadSizeBytes;

                return 0;
            });

        this->MmiGet([](MMI_HANDLE handle, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes) -> int
            {
                (void)handle;
                (void)componentName;
                (void)objectName;
                (void)payload;
                (void)payloadSizeBytes;

                return 0;
            });

        this->MmiFree([](MMI_JSON_STRING payload)
            {
                delete[] payload;
            });

        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        CallMmiGetInfo("Azure OsConfig", &payload, &payloadSizeBytes);

        rapidjson::Document document;
        document.Parse(payload, payloadSizeBytes).HasParseError();
        Info::Deserialize(document, m_info);
    }

    MockManagementModule::MockManagementModule(std::string name, std::vector<std::string> components) :
        MockManagementModule()
    {
        m_info.name = name;
        m_info.components = components;
    }

    void MockManagementModule::MmiGetInfo(Mmi_GetInfo mmiGetInfo)
    {
        this->m_mmiGetInfo = mmiGetInfo;
    }

    void MockManagementModule::MmiOpen(Mmi_Open mmiOpen)
    {
        this->m_mmiOpen = mmiOpen;
    }

    void MockManagementModule::MmiClose(Mmi_Close mmiClose)
    {
        this->m_mmiClose = mmiClose;
    }

    void MockManagementModule::MmiSet(Mmi_Set mmiSet)
    {
        this->m_mmiSet = mmiSet;
    }

    void MockManagementModule::MmiGet(Mmi_Get mmiGet)
    {
        this->m_mmiGet = mmiGet;
    }

    void MockManagementModule::MmiFree(Mmi_Free mmiFree)
    {
        this->m_mmiFree = mmiFree;
    }
} // namespace Tests