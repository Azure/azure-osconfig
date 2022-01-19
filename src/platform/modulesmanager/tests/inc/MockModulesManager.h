// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MOCKMODULESMANAGER_H
#define MOCKMODULESMANAGER_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ModulesManager.h>
#include <MockManagementModule.h>

using ::testing::StrictMock;

namespace Tests
{
    class MockModulesManager : public ModulesManager
    {
    public:
        MockModulesManager(std::string clientName, unsigned int maxPayloadSizeBytes = 0) : ModulesManager(clientName, maxPayloadSizeBytes) {}
        ~MockModulesManager() {}

        // Helper methods providing access to protected members of ModulesManager
        std::shared_ptr<StrictMock<MockManagementModule>> CreateModule(const std::string& componentName);
        std::shared_ptr<ManagementModule> GetModule(const std::string& componentName);
        std::vector<MockModulesManager::ModuleMetadata> GetModulesToUnload();
    };
} // namespace Tests

#endif // MOCKMODULESMANAGER_H