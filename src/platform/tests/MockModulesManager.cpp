// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <PlatformCommon.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <MockManagementModule.h>
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
            m_moduleComponentName[component] = info.name;
        }
    }

    void MockModulesManager::AddReportedObject(std::string componentName, std::string objectName)
    {
        if (m_reportedComponents.find(componentName) == m_reportedComponents.end())
        {
            m_reportedComponents[componentName] = std::vector<std::string>();
        }

        m_reportedComponents[componentName].push_back(objectName);
    }
} // namespace Tests