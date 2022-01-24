// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <string>

#include <MockManagementModule.h>

namespace Tests
{
    class MockHandle {};

    MockManagementModule::MockManagementModule(const std::string& clientName, unsigned int maxPayloadSizeBytes) : ManagementModule(clientName, "", maxPayloadSizeBytes)
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

                std::size_t len = strlen(mockInfo) - 1;
                *payloadSizeBytes = len;
                *payload = new char[len];
                std::memcpy(*payload, mockInfo, len);

                return MMI_OK;
            });

        this->MmiOpen([](const char* clientName, const unsigned int maxPayloadSizeBytes) -> MMI_HANDLE
            {
                (void)maxPayloadSizeBytes;

                MMI_HANDLE handle = nullptr;

                if (0 == strcmp("client_name", clientName))
                {
                    handle = reinterpret_cast<MMI_HANDLE>(new MockHandle());
                }

                return handle;
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
    }

    void MockManagementModule::MmiGetInfo(Mmi_GetInfo mmiGetInfo)
    {
        this->mmiGetInfo = mmiGetInfo;
    }

    void MockManagementModule::MmiOpen(Mmi_Open mmiOpen)
    {
        this->mmiOpen = mmiOpen;
    }

    void MockManagementModule::MmiClose(Mmi_Close mmiClose)
    {
        this->mmiClose = mmiClose;
    }

    void MockManagementModule::MmiSet(Mmi_Set mmiSet)
    {
        this->mmiSet = mmiSet;
    }

    void MockManagementModule::MmiGet(Mmi_Get mmiGet)
    {
        this->mmiGet = mmiGet;
    }

    void MockManagementModule::MmiFree(Mmi_Free mmiFree)
    {
        this->mmiFree = mmiFree;
    }
} // namespace Tests