#include "Common.h"

bool RecipeModuleSessionLoader::Load(const std::vector<std::string> &modulePaths)
{
    for (const auto &modulePath : modulePaths)
    {
        std::shared_ptr<ManagementModule> mm = std::make_shared<ManagementModule>(modulePath);
        if (0 == mm->Load())
        {
            ManagementModule::Info info = mm->GetInfo();

            for (const auto &component : info.components)
            {
                auto search = g_componentModuleSessionMap.find(component);
                if (search == g_componentModuleSessionMap.end())
                {
                    auto session = std::make_shared<MmiSession>(mm, DefaultClientName);
                    if (modulePath == _mainModulePath)
                    {
                        _mainSession = session;
                    }
                    g_componentModuleSessionMap[component] = std::make_pair(mm, session);
                }
            }
        }
        else
        {
            TestLogError("Failed to load module '%s'", modulePath.c_str());
        }
        mm->Unload();
    }

    return true;
}

void RecipeModuleSessionLoader::Unload()
{
    for (auto &moduleSession : g_componentModuleSessionMap)
    {
        if (moduleSession.second.second->IsOpen())
        {
            TestLogInfo("[RecipeModuleSessionLoader] Closing session for '%s'", moduleSession.second.second->GetInfo().name.c_str());
            moduleSession.second.second->Close();
        }
    }
}

RecipeModuleSessionLoader::~RecipeModuleSessionLoader()
{
    Unload();
}

std::shared_ptr<MmiSession> RecipeModuleSessionLoader::GetSession(const std::string &componentName)
{
    auto search = g_componentModuleSessionMap.find(componentName);
    if (search != g_componentModuleSessionMap.end())
    {
        // Open session if not open yet
        if (!search->second.second->IsOpen())
        {
            TestLogInfo("[RecipeModuleSessionLoader] Opening session for '%s'", search->second.first->GetInfo().name.c_str());
            search->second.second->Open();
        }

        return search->second.second;
    }
    
    // Return main recipe module session if the component name is not found
    return _mainSession;
}