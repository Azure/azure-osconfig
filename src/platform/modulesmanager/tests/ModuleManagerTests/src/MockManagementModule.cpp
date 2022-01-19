// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstdio>
#include <cstring>
#include <string>

#include <MockManagementModule.h>

namespace Tests
{
    MockManagementModule::MockManagementModule(const std::string& clientName, unsigned int maxPayloadSizeBytes) : ManagementModule(clientName, "", maxPayloadSizeBytes)
    {
        this->mmiGetInfo = [](const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes) -> int
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
        };
    }

    void MockManagementModule::MmiSet(Mmi_Set mmiSet)
    {
        this->mmiSet = mmiSet;
    }

    void MockManagementModule::MmiGet(Mmi_Get mmiGet)
    {
        this->mmiGet = mmiGet;
    }
} // namespace Tests