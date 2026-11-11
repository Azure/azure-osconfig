// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <EventPayload.h>
#include <parson.h>
#include <cerrno>
#include <cstring>
#include <string>

TEST(TelemetryPayloadTest, EncodesTheGenericJsonEnvelopeWithoutChangingStringValues)
{
    TelemetryProperty property = {};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0;

    property.name = "CrashInfo";
    property.type = TelemetryPropertyString;
    property.value.stringValue = "quote\" newline\nslash\\";
    ASSERT_EQ(0, TelemetryPackEvent("CrashDetected", &property, 1, payload, sizeof(payload), &size, nullptr));
    ASSERT_GT(size, 0U);
    EXPECT_EQ('}', payload[size - 1]);
    std::string text(reinterpret_cast<char*>(payload), size);
    JSON_Value* parsed = json_parse_string(text.c_str());
    ASSERT_NE(nullptr, parsed);
    JSON_Object* event = json_value_get_object(parsed);
    EXPECT_STREQ("CrashDetected", json_object_get_string(event, "EventName"));
    JSON_Array* fields = json_object_get_array(event, "EventFields");
    ASSERT_EQ(1U, json_array_get_count(fields));
    JSON_Object* field = json_array_get_object(fields, 0);
    EXPECT_STREQ("CrashInfo", json_object_get_string(field, "FieldName"));
    EXPECT_STREQ("string", json_object_get_string(field, "FieldType"));
    EXPECT_STREQ(property.value.stringValue, json_object_get_string(field, "FieldValue"));
    json_value_free(parsed);
}

TEST(TelemetryPayloadTest, RejectsDuplicateFieldsAndInvalidNames)
{
    TelemetryProperty properties[2] = {};
    for (auto& property : properties)
    {
        property.name = "Census";
        property.value.stringValue = "example";
    }
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0;
    EXPECT_EQ(EINVAL, TelemetryPackEvent("Test", properties, 2, payload, sizeof(payload), &size, nullptr));
    EXPECT_EQ(EINVAL, TelemetryPackEvent("Bad name", properties, 1, payload, sizeof(payload), &size, nullptr));
    EXPECT_EQ(0U, size);
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
