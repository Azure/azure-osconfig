// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Event.h>
#include <cerrno>
#include <cstring>
#include <string>

class TelemetryEventTest : public ::testing::Test
{
protected:
    TelemetryProperty properties[5] = {};
    unsigned char payload[TELEMETRY_MAX_EVENT_SIZE] = {};
    unsigned char encoded[TELEMETRY_MAX_EVENT_SIZE] = {};
    size_t size = 0, encodedSize = 0;
    int64_t upload = 0;

    void SetUp() override
    {
        const char* names[] = {"DistroName", "CorrelationId", "Version", "Timestamp", "CrashInfo"};
        size_t i = 0;

        for (i = 0; i < 5; ++i)
        {
            properties[i].name = names[i];
            properties[i].type = TelemetryPropertyString;
            properties[i].value.stringValue = "value";
        }
    }

    int Pack(const char* name = "CrashDetected")
    {
        return TelemetryPackEvent(name, properties, 5, payload, sizeof(payload), &size, nullptr);
    }

    int Encode()
    {
        return TelemetryEncodePayload(payload, size, "o:fixture", "fixture-epoch", 1,
            encoded, &encodedSize, &upload, nullptr);
    }
};

TEST_F(TelemetryEventTest, EncodesNamedSchemaWithoutJsonEscapingOrTypeChanges)
{
    std::string wire = {};

    properties[4].value.stringValue = "quote\" newline\nslash\\";
    ASSERT_EQ(0, Pack());
    ASSERT_EQ(0, Encode());
    EXPECT_GT(upload, 0);
    wire.assign(reinterpret_cast<char*>(encoded), encodedSize);
    EXPECT_NE(std::string::npos, wire.find("CrashDetected"));
    EXPECT_NE(std::string::npos, wire.find(properties[4].value.stringValue));
    EXPECT_NE(std::string::npos, wire.find(TELEMETRY_CLIENT_VERSION));
}

TEST_F(TelemetryEventTest, RejectsUnknownEventAndWrongOrDuplicateFields)
{
    ASSERT_EQ(0, Pack("Unknown"));
    EXPECT_EQ(EINVAL, Encode());
    properties[4].name = "Unknown";
    ASSERT_EQ(0, Pack());
    EXPECT_EQ(EINVAL, Encode());
    properties[4].name = "DistroName";
    ASSERT_EQ(0, Pack());
    EXPECT_EQ(EINVAL, Encode());
}

TEST_F(TelemetryEventTest, RejectsEveryTruncationAndTrailingBytes)
{
    size_t fullSize = 0;

    ASSERT_EQ(0, Pack());
    fullSize = size;

    for (size = 0; size < fullSize; ++size)
    {
        EXPECT_EQ(EINVAL, Encode()) << size;
        EXPECT_EQ(0U, encodedSize);
    }

    ++size;
    EXPECT_EQ(EINVAL, Encode());
}

TEST_F(TelemetryEventTest, RejectsInvalidTypesOversizedValuesAndCounts)
{
    std::string oversized = {};
    uint32_t count = UINT32_MAX;

    properties[4].type = TelemetryPropertyBoolean;
    EXPECT_EQ(EINVAL, Pack());
    EXPECT_EQ(0U, size);
    properties[4].type = TelemetryPropertyString;
    oversized.assign(TELEMETRY_MAX_EVENT_SIZE, 'x');
    properties[4].value.stringValue = oversized.c_str();
    EXPECT_EQ(EMSGSIZE, Pack());
    EXPECT_EQ(0U, size);
    memcpy(payload, &count, sizeof(count));
    size = sizeof(count) + 1;
    EXPECT_EQ(EINVAL, Encode());
}

TEST_F(TelemetryEventTest, RejectsInvalidUtf8RatherThanChangingTheValue)
{
    properties[4].value.stringValue = "\xFF";
    ASSERT_EQ(0, Pack());
    EXPECT_NE(0, Encode());
    EXPECT_EQ(0U, encodedSize);
}

TEST(TelemetryEpochTest, ProducesDistinctVersionFourIds)
{
    char first[TELEMETRY_EPOCH_SIZE] = {};
    char second[TELEMETRY_EPOCH_SIZE] = {};

    ASSERT_EQ(0, TelemetryCreateEpoch(first, nullptr));
    ASSERT_EQ(0, TelemetryCreateEpoch(second, nullptr));
    EXPECT_EQ(36U, strlen(first));
    EXPECT_EQ('4', first[14]);
    EXPECT_NE(nullptr, strchr("89ab", first[19]));
    EXPECT_STRNE(first, second);
}
