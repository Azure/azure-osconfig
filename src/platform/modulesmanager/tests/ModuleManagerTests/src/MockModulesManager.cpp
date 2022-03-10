// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <MockModulesManager.h>

using ::testing::StrictMock;

namespace Tests
{
    void MockModulesManager::Load(std::shared_ptr<ManagementModule> module)
    {
        ManagementModule::Info info = module->GetInfo();
        m_modules[info.name] = module;
        for (auto& component : info.components)
        {
            m_componentToModule[component] = info.name;
        }
    }

    void MockModulesManager::AddReportedObject(std::string componentName, std::string objectName)
    {
        if (m_reported.find(componentName) == m_reported.end())
        {
            m_reported[componentName] = std::unordered_set<std::string>();
        }

        m_reported[componentName].insert(objectName);
    }
} // namespace Tests