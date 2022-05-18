// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MOCKMANAGEMENTMODULE_H
#define MOCKMANAGEMENTMODULE_H

namespace Tests
{
    class MockManagementModule : public ManagementModule
    {
    public:
        MMI_HANDLE mmiHandle;

        MockManagementModule();
        MockManagementModule(std::string name, std::vector<std::string> components);

        MOCK_METHOD(int, CallMmiSet, (MMI_HANDLE handle, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes), (override));
        MOCK_METHOD(int, CallMmiGet, (MMI_HANDLE handle, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes), (override));

        void MmiGetInfo(Mmi_GetInfo mmiGetInfo);
        void MmiOpen(Mmi_Open mmiOpen);
        void MmiClose(Mmi_Close mmiClose);
        void MmiSet(Mmi_Set mmiSet);
        void MmiGet(Mmi_Get mmiGet);
        void MmiFree(Mmi_Free mmiFree);
    };
} // namespace Tests

#endif // MOCKMANAGEMENTMODULE_H