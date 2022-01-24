// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <MockModulesManager.h>

using ::testing::StrictMock;

namespace Tests
{
    std::shared_ptr<StrictMock<MockManagementModule>> MockModulesManager::CreateModule(const std::string& componentName)
    {
        std::shared_ptr<StrictMock<MockManagementModule>> module = std::make_shared<StrictMock<MockManagementModule>>(componentName);
        this->modMap[componentName] = {module, std::chrono::system_clock::now(), false};
        return module;
    }

    std::shared_ptr<ManagementModule> MockModulesManager::GetModule(const std::string& componentName)
    {
        return this->modMap[componentName].module;
    }

    std::vector<MockModulesManager::ModuleMetadata> MockModulesManager::GetModulesToUnload()
    {
        return this->modulesToUnload;
    }
} // namespace Tests