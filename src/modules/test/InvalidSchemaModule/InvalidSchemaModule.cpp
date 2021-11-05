// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <cstring>
#include <errno.h>
#include <memory>
#include <CommonUtils.h>
#include <Mmi.h>
#include <map>

class TestsModuleInternal{};
// Mapping clientName <-> TestsModuleInternal
std::map<std::string, std::weak_ptr<TestsModuleInternal>> testModuleMap;
// Mapping MMI_HANDLE <->  TestsModuleInternal
std::map<MMI_HANDLE, std::shared_ptr<TestsModuleInternal>> mmiMap;

// Missing "name", "description" from schema
// "name": "InvalidSchemaMMI",
int MmiGetInfo(
    const char* clientName,
    MMI_JSON_STRING*payload,
    int* payloadSizeBytes)
{
    if (nullptr == clientName || nullptr == payload)
    {
        return EINVAL;
    }

    constexpr const char ret[] = R""""({
        "manufacture": "Microsoft",
        "Description": false,
        "VersionMajor": -1,
        "VersionMinor": "String",
        "VersionInfo": 123,
        "Components": ["InvalidSchemaMMI", "InvalidSchemaMMI"],
        "UserAccount": -1,
        "Lifetime": 4})"""";

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
    UNUSED(componentName);
    UNUSED(objectName);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return MMI_OK;
}

void MmiFree(MMI_JSON_STRING payload)
{
    delete[] payload;
}