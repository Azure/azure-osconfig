// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef RECIPEMODULESESSIONLOADER_H
#define RECIPEMODULESESSIONLOADER_H

typedef std::pair<std::shared_ptr<ManagementModule>, std::shared_ptr<MmiSession>> ModuleSession;

class RecipeModuleSessionLoader
{
public:
    RecipeModuleSessionLoader(const std::string &mainModulePath) : _mainModulePath(mainModulePath) {};
    ~RecipeModuleSessionLoader();
    bool Load(const std::vector<std::string> &modulePaths);
    void Unload();
    std::shared_ptr<MmiSession> GetSession(const std::string &componentName);

private:
    std::map<std::string, ModuleSession> g_componentModuleSessionMap;
    const std::string _mainModulePath;
    std::shared_ptr<MmiSession> _mainSession;
};

#endif // RECIPEMODULESESSIONLOADER_H