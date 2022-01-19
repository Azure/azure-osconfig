// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MOCKMANAGEMENTMODULE_H
#define MOCKMANAGEMENTMODULE_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ManagementModule.h>

namespace Tests
{
    class MockManagementModule : public ManagementModule
    {
    public:
        MockManagementModule(const std::string& clientName, unsigned int maxPayloadSizeBytes = 0);

        MOCK_METHOD(void, LoadModule, ());
        MOCK_METHOD(void, UnloadModule, ());
        MOCK_METHOD(int, CallMmiSet, (const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes));
        MOCK_METHOD(int, CallMmiGet, (const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes));

        void MmiSet(Mmi_Set mmiSet);
        void MmiGet(Mmi_Get mmiGet);
    };
} // namespace Tests

#endif // MOCKMANAGEMENTMODULE_H