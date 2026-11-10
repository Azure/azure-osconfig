// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <EventPayload.h>
#include <cerrno>
#include <cstring>
#include <string>

TEST(TelemetryPayloadTest, PreservesTheReferenceWorkersNativeStringPayload)
{
    TelemetryProperty property = {};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0;
    uint32_t count = 0;
    const char strings[] = "CrashDetected\0CrashInfo\0quote\" newline\nslash\\";

    property.name = "CrashInfo";
    property.type = TelemetryPropertyString;
    property.value.stringValue = "quote\" newline\nslash\\";
    ASSERT_EQ(0, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &size, nullptr));
    ASSERT_EQ(sizeof(count) + sizeof(strings), size);
    memcpy(&count, payload, sizeof(count));
    EXPECT_EQ(1U, count);
    EXPECT_EQ(0, memcmp(payload + sizeof(count), strings, sizeof(strings)));
}

TEST(TelemetryPayloadTest, RejectsTruncatedOutputAndInvalidProperties)
{
    TelemetryProperty property = {};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0;
    size_t fullSize = 0;
    std::string oversized(TELEMETRY_MAX_EVENT_SIZE, 'x');

    property.name = "CrashInfo";
    property.value.stringValue = "value";
    ASSERT_EQ(0, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &fullSize, nullptr));
    for (size_t capacity = sizeof(uint32_t); capacity < fullSize; ++capacity)
    {
        EXPECT_EQ(EMSGSIZE, TelemetryPackEvent("CrashDetected", &property, 1, payload, capacity, &size, nullptr));
        EXPECT_EQ(0U, size);
    }

    property.value.stringValue = oversized.c_str();
    EXPECT_EQ(EMSGSIZE, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &size, nullptr));
    property.value.stringValue = nullptr;
    EXPECT_EQ(EINVAL, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &size, nullptr));
    property.type = TelemetryPropertyBoolean;
    EXPECT_EQ(EINVAL, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &size, nullptr));
    EXPECT_EQ(EINVAL, TelemetryPackEvent("CrashDetected", &property, TELEMETRY_MAX_PROPERTY_COUNT + 1,
        payload, sizeof(payload), &size, nullptr));
    EXPECT_EQ(0U, size);
}
