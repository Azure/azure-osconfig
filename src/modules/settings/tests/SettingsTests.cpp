// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <Settings.h>
#include <Mmi.h>
#include <fstream>
#include <iostream>
#include <cstring>

using namespace std;

const int maxPayloadSizeBytes = 4000;
const char* clientName = "ClientName";

TEST (SettingsTests, DH)
{
    static const char g_testFile[] = "test.toml";
    const char g_testTomlData[] = "Permission = \"None\"";

    ofstream ofs_toml(g_testFile);
    if (nullptr != g_testTomlData)
    {
        ofs_toml << g_testTomlData;
    }
    ofs_toml.close();

    Settings settingsTest;

    bool configurationChanged = false;
    ASSERT_EQ(MMI_OK, settingsTest.SetDeviceHealthTelemetryConfiguration("2", g_testFile, configurationChanged));

    configurationChanged = false;
    settingsTest.SetDeviceHealthTelemetryConfiguration("2", g_testFile, configurationChanged);
    ASSERT_EQ(false, configurationChanged);

    ASSERT_NE(MMI_OK, settingsTest.SetDeviceHealthTelemetryConfiguration("7", g_testFile, configurationChanged));
    ASSERT_NE(MMI_OK, settingsTest.SetDeviceHealthTelemetryConfiguration("", g_testFile, configurationChanged));

    if (!(remove(g_testFile) == 0))
    {
        std::cout << "Err: can't remove test toml file.";
    }
}

TEST(SettingsTests, DO)
{
    static const char g_testFile[] = "test.json";
    const char g_testJsonData[] = "{\"DOPercentageDownloadThrottle\":0, \"DOCacheHostSource\": 1, \"DOCacheHost\":\"test\", \"DOCacheHostFallback\":2020}";
    ofstream ofs_json(g_testFile);

    if (nullptr != g_testJsonData)
    {
        ofs_json << g_testJsonData;
    }
    ofs_json.close();

    bool configurationChanged = false;
    Settings settingsTest;
    Settings::DeliveryOptimization DO{30, 2, "testing", 2020};
    ASSERT_EQ(MMI_OK, settingsTest.SetDeliveryOptimizationPolicies(DO, g_testFile, configurationChanged));

    configurationChanged = false;
    settingsTest.SetDeliveryOptimizationPolicies(DO, g_testFile, configurationChanged);
    ASSERT_EQ(false, configurationChanged);

    Settings::DeliveryOptimization DO_empty{};
    ASSERT_EQ(MMI_OK, settingsTest.SetDeliveryOptimizationPolicies(DO_empty, g_testFile, configurationChanged));

    if (!(remove(g_testFile) == 0))
    {
        std::cout << "Err: can't remove test json file.";
    }
}

TEST(SettingsTests, MmiGetInfo)
{
    MMI_JSON_STRING payload = nullptr;
    int payloadSizeBytes = 0;

    int status = MmiGetInfo(nullptr, &payload, &payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);
    EXPECT_EQ(payload, nullptr);
    EXPECT_EQ(payloadSizeBytes, 0);

    status = MmiGetInfo(clientName, nullptr, &payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);
    EXPECT_EQ(payloadSizeBytes, 0);

    status = MmiGetInfo(clientName, &payload, nullptr);
    EXPECT_EQ(status, EINVAL);
    EXPECT_EQ(payload, nullptr);

    status = MmiGetInfo(clientName, &payload, &payloadSizeBytes);
    EXPECT_EQ(status, MMI_OK);

    EXPECT_NE(payload, nullptr);
    delete payload;
}

TEST(SettingsTests, MmiOpen)
{
    MMI_HANDLE handle = MmiOpen(nullptr, maxPayloadSizeBytes);
    EXPECT_EQ(handle, nullptr);

    handle = MmiOpen(clientName, maxPayloadSizeBytes);
    EXPECT_NE(handle, nullptr);

    MmiClose(handle);
}

TEST(SettingsTests, MmiSet)
{
    char arr[4] = {'t', 'e', 's', 't'};
    const MMI_JSON_STRING payload = &arr[0];
    const int payloadSizeBytes = 0;
    const int payloadSizeBytesExceedsMax = 4001;

    MMI_HANDLE handle = MmiOpen(clientName, maxPayloadSizeBytes);
    EXPECT_NE(handle, nullptr);

    int status = MmiSet(nullptr, g_componentName.c_str(), g_deviceHealthTelemetry.c_str(), payload, payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);

    const char* componentNameUnknown = "ComponentNameUnknown";
    status = MmiSet(handle, componentNameUnknown, g_deviceHealthTelemetry.c_str(), payload, payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);

    const char* objectNameUnknown = "ObjectNameUnknown";
    status = MmiSet(handle, g_componentName.c_str(), objectNameUnknown, payload, payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);

    status = MmiSet(handle, g_componentName.c_str(), g_deviceHealthTelemetry.c_str(), nullptr, payloadSizeBytes);
    EXPECT_EQ(status, EINVAL);

    status = MmiSet(handle, g_componentName.c_str(), g_deviceHealthTelemetry.c_str(), payload, payloadSizeBytesExceedsMax);
    EXPECT_EQ(status, E2BIG);

    MmiClose(handle);
}