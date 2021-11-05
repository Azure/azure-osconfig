// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <string>
#include <vector>
#include <memory>
#include <CommonUtils.h>
#include <Mmi.h>
#include <map>

const std::string moduleName = "MultiComponentTheLargestVersionModule";
// "TestComponent2" are defined in MultiComponentModule, newer version, and newest version (this module).
// When comparing two versions, select the larger one.
const std::string componentName2 = "TestComponent2";
const std::string componentName3 = "TestComponent3";

class TestsModuleInternal{};
// Mapping clientName <-> TestsModuleInternal
std::map<std::string, std::weak_ptr<TestsModuleInternal>> testModuleMap;
// Mapping MMI_HANDLE <->  TestsModuleInternal
std::map<MMI_HANDLE, std::shared_ptr<TestsModuleInternal>> mmiMap;

int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientName);
    if (nullptr == payload)
    {
        return EINVAL;
    }

    constexpr const char ret[] = R""""({
        "Name": "MultiComponentTheLargestVersionModule",
        "Description": "A normally behaving test module with a valid MmiGetInfo schema which also implements multiple components",
        "Manufacturer": "Microsoft",
        "VersionMajor": 1,
        "VersionMinor": 0,
        "VersionInfo": "Initial Version",
         "Components": ["TestComponent2", "TestComponent3"],
         "Lifetime": 0,
         "UserAccount": 0})"""";

    std::size_t len = sizeof(ret) - 1;

    *payloadSizeBytes = len;
    *payload = new char[len];
    std::memcpy(*payload, ret, len);

    return MMI_OK;
}

MMI_HANDLE MmiOpen(
    const char* clientName,
    const unsigned int maxPayloadSize)
{
    MMI_HANDLE handle = nullptr;

    UNUSED(maxPayloadSize);

    if (nullptr == clientName)
    {
        return handle;
    }

    if (testModuleMap.end() == testModuleMap.find(clientName))
    {
        auto testModule = std::shared_ptr<TestsModuleInternal>(new TestsModuleInternal());
        MMI_HANDLE mmiH = reinterpret_cast<MMI_HANDLE>(testModule.get());
        testModuleMap[clientName] = testModule;
        mmiMap[mmiH] = testModule;
        handle = mmiH;
    }
    else
    {
        handle = reinterpret_cast<MMI_HANDLE>(testModuleMap[clientName].lock().get());
    }
    return handle;
}

void MmiClose(MMI_HANDLE clientSession)
{
    if (mmiMap.end() != mmiMap.find(clientSession))
    {
        mmiMap[clientSession].reset();
    }
}

int MmiSet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    const MMI_JSON_STRING payload,
    const int payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MMI_OK;
}

int MmiGet(
    MMI_HANDLE clientSession,
    const char* componentName,
    const char* objectName,
    MMI_JSON_STRING* payload,
    int* payloadSizeBytes)
{
    UNUSED(clientSession);
    UNUSED(objectName);

    constexpr const char fmt[] = R""""( { "returnValue": "%s-%s" } )"""";

    if (0 == strcmp(componentName, componentName2.c_str()))
    {
        int sz = std::snprintf(nullptr, 0, fmt, componentName2.c_str(), moduleName.c_str());
        *payload = new char[sz + 1];
        std::snprintf(*payload, sz, fmt, componentName2.c_str(), moduleName.c_str());
        *payloadSizeBytes = sz;
    }
    else if (0 == strcmp(componentName, componentName3.c_str()))
    {
        int sz = std::snprintf(nullptr, 0, fmt, componentName3.c_str(), moduleName.c_str());
        *payload = new char[sz + 1];
        std::snprintf(*payload, sz, fmt, componentName3.c_str(), moduleName.c_str());
        *payloadSizeBytes = sz;
    }
    else
    {
        return EINVAL;
    }

    return MMI_OK;
}

void MmiFree(MMI_JSON_STRING payload)
{
    delete[] payload;
}