// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MOCKMODULESMANAGER_H
#define MOCKMODULESMANAGER_H

using ::testing::StrictMock;

namespace Tests
{
    class MockModulesManager : public ModulesManager
    {
    public:
        MockModulesManager() : ModulesManager() {}
        ~MockModulesManager() {}

        // Helper method to "load" mock modules into the ModulesManager
        void Load(std::shared_ptr<ManagementModule> module);

        // Helper method to add reported objects to the ModulesManager
        void AddReportedObject(std::string componentName, std::string objectName);
    };
} // namespace Tests

#endif // MOCKMODULESMANAGER_H