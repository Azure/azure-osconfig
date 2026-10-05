// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryEncoder.h>

#include <array>
#include <cerrno>
#include <limits>
#include <string>
#include <utility>
#include <vector>

class TelemetryEncoderTest : public ::testing::Test
{
protected:
    TelemetryEvent event = {};
    std::array<unsigned char, TELEMETRY_MAX_EVENT_SIZE + 2> buffer = {};
    size_t encodedSize = 0;

    void SetUp() override
    {
        event.name = "Test";
        event.iKey = "o:t";
        event.time = 1;
        buffer.fill(0xA5);
    }

    static std::vector<unsigned char> RecordPrefix()
    {
        return {
            0x29, 0x03, '3', '.', '0',
            0x49, 0x04, 'T', 'e', 's', 't',
            0x71, 0x02,
            0xA9, 0x03, 'o', ':', 't'
        };
    }

    static void AppendDataHeader(std::vector<unsigned char>& bytes)
    {
        bytes.insert(bytes.end(), {
            0xC9, 0x3C, 0x06, 'c', 'u', 's', 't', 'o', 'm',
            0xCB, 0x46, 0x0A, 0x01
        });
    }

    static std::vector<unsigned char> PropertyRecord(const std::vector<unsigned char>& value)
    {
        auto bytes = RecordPrefix();

        AppendDataHeader(bytes);
        bytes.insert(bytes.end(), {0x2D, 0x09, 0x0A, 0x01, 0x01, 'P'});
        bytes.insert(bytes.end(), value.begin(), value.end());
        bytes.insert(bytes.end(), {0x00, 0x00});

        return bytes;
    }

    void ExpectEncoded(const std::vector<unsigned char>& expected)
    {
        buffer.fill(0xA5);
        encodedSize = 99;
        ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), TELEMETRY_MAX_EVENT_SIZE, &encodedSize, NULL));
        ASSERT_EQ(expected.size(), encodedSize);
        EXPECT_EQ(expected, std::vector<unsigned char>(buffer.begin(), buffer.begin() + encodedSize));
        EXPECT_EQ(0xA5, buffer[encodedSize]);
        EXPECT_EQ(0xA5, buffer[TELEMETRY_MAX_EVENT_SIZE]);
        EXPECT_EQ(0xA5, buffer[TELEMETRY_MAX_EVENT_SIZE + 1]);
    }

    void ExpectFailure(int status, size_t capacity = TELEMETRY_MAX_EVENT_SIZE)
    {
        std::array<unsigned char, TELEMETRY_MAX_EVENT_SIZE + 2> original = {};

        buffer.fill(0xA5);
        original = buffer;
        encodedSize = 99;
        EXPECT_EQ(status, TelemetryEncodeEvent(&event, buffer.data(), capacity, &encodedSize, NULL));
        EXPECT_EQ(0U, encodedSize);
        EXPECT_EQ(original, buffer);
    }

    void UseProperty(const TelemetryProperty& property)
    {
        event.properties = &property;
        event.propertyCount = 1;
    }
};

TEST_F(TelemetryEncoderTest, EncodesOneCommonSchemaRecordWithoutAnOuterEnvelope)
{
    auto expected = RecordPrefix();

    AppendDataHeader(expected);
    expected.insert(expected.end(), {0x00, 0x00});
    ExpectEncoded(expected);
}

TEST_F(TelemetryEncoderTest, EncodesFlagsAndDeviceAndSdkExtensions)
{
    std::vector<unsigned char> expected = {};

    event.flags = 0x0202;
    event.deviceId = "c:d";
    event.sdkVersion = "v";
    event.sdkEpoch = "e";
    event.sequence = 5;

    expected = RecordPrefix();
    expected.insert(expected.end(), {
        0xD1, 0x06, 0x84, 0x08,
        0xCB, 0x17, 0x0A, 0x01, 0x49, 0x03, 'c', ':', 'd', 0x00,
        0xCB, 0x20, 0x0A, 0x01,
        0x29, 0x01, 'v', 0x49, 0x01, 'e', 0x71, 0x0A, 0x00
    });
    AppendDataHeader(expected);
    expected.insert(expected.end(), {0x00, 0x00});
    ExpectEncoded(expected);
}

TEST_F(TelemetryEncoderTest, OmitsEmptyOptionalMetadata)
{
    std::vector<unsigned char> expected = {};

    event.deviceId = "";
    event.sdkVersion = "";
    event.sdkEpoch = "";
    expected = RecordPrefix();
    AppendDataHeader(expected);
    expected.insert(expected.end(), {0x00, 0x00});
    ExpectEncoded(expected);
}

TEST_F(TelemetryEncoderTest, EncodesStringPropertyAndItsDefaultKind)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyString;
    property.value.stringValue = "x";
    UseProperty(property);
    ExpectEncoded(PropertyRecord({0x69, 0x01, 'x', 0x00}));

    property.value.stringValue = "";
    ExpectEncoded(PropertyRecord({0x00}));
}

TEST_F(TelemetryEncoderTest, UsesUtf8ByteLengthsWithoutJsonEscaping)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyString;
    property.value.stringValue = "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
    UseProperty(property);
    ExpectEncoded(PropertyRecord({0x69, 0x09, 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80, 0x00}));

    property.value.stringValue = "\"\\\n";
    ExpectEncoded(PropertyRecord({0x69, 0x03, '"', '\\', '\n', 0x00}));
}

TEST_F(TelemetryEncoderTest, EncodesStringLengthVarintBoundary)
{
    TelemetryProperty property = {};
    std::string text = {};
    std::vector<unsigned char> value = {};

    property.name = "P";
    property.type = TelemetryPropertyString;
    UseProperty(property);

    for (size_t length : {127U, 128U})
    {
        text.assign(length, 'x');
        property.value.stringValue = text.c_str();
        value = {0x69};

        if (127 == length)
        {
            value.push_back(0x7F);
        }
        else
        {
            value.insert(value.end(), {0x80, 0x01});
        }

        value.insert(value.end(), length, 'x');
        value.push_back(0x00);
        ExpectEncoded(PropertyRecord(value));
    }
}

TEST_F(TelemetryEncoderTest, EncodesSignedValuesWithoutLosingIntegerPrecision)
{
    const std::vector<std::pair<int64_t, std::vector<unsigned char>>> cases = {
        {0, {0x30, 0x00, 0x00}},
        {-1, {0x30, 0x00, 0x91, 0x01, 0x00}},
        {1, {0x30, 0x00, 0x91, 0x02, 0x00}},
        {63, {0x30, 0x00, 0x91, 0x7E, 0x00}},
        {64, {0x30, 0x00, 0x91, 0x80, 0x01, 0x00}},
        {-64, {0x30, 0x00, 0x91, 0x7F, 0x00}},
        {-65, {0x30, 0x00, 0x91, 0x81, 0x01, 0x00}},
        {std::numeric_limits<int64_t>::min(),
            {0x30, 0x00, 0x91, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00}},
        {std::numeric_limits<int64_t>::max(),
            {0x30, 0x00, 0x91, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00}}
    };
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyInt64;
    UseProperty(property);

    for (const auto& item : cases)
    {
        SCOPED_TRACE(item.first);
        property.value.int64Value = item.first;
        ExpectEncoded(PropertyRecord(item.second));
    }
}

TEST_F(TelemetryEncoderTest, EncodesBooleanKindEvenWhenFalse)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyBoolean;
    property.value.booleanValue = true;
    UseProperty(property);
    ExpectEncoded(PropertyRecord({0x30, 0x0C, 0x91, 0x02, 0x00}));

    property.value.booleanValue = false;
    ExpectEncoded(PropertyRecord({0x30, 0x0C, 0x00}));
}

TEST_F(TelemetryEncoderTest, EncodesDoubleAsLittleEndianAndOmitsZeroSlot)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyDouble;
    property.value.doubleValue = 1.5;
    UseProperty(property);
    ExpectEncoded(PropertyRecord({0x30, 0x08, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x3F, 0x00}));

    property.value.doubleValue = -1.0;
    ExpectEncoded(PropertyRecord({0x30, 0x08, 0xA8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xBF, 0x00}));

    property.value.doubleValue = 0.0;
    ExpectEncoded(PropertyRecord({0x30, 0x08, 0x00}));
    property.value.doubleValue = -0.0;
    ExpectEncoded(PropertyRecord({0x30, 0x08, 0x00}));
}

TEST_F(TelemetryEncoderTest, SortsPropertiesLikeTheSdkMapWithoutChangingInputs)
{
    TelemetryProperty properties[2] = {};
    std::vector<unsigned char> expected = {};

    properties[0].name = "Z";
    properties[0].type = TelemetryPropertyString;
    properties[0].value.stringValue = "z";
    properties[1].name = "A";
    properties[1].type = TelemetryPropertyInt64;
    properties[1].value.int64Value = -1;
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);

    expected = RecordPrefix();
    AppendDataHeader(expected);
    expected.insert(expected.end(), {
        0x2D, 0x09, 0x0A, 0x02,
        0x01, 'A', 0x30, 0x00, 0x91, 0x01, 0x00,
        0x01, 'Z', 0x69, 0x01, 'z', 0x00, 0x00, 0x00
    });
    ExpectEncoded(expected);
    EXPECT_STREQ("Z", properties[0].name);
    EXPECT_STREQ("A", properties[1].name);
}

TEST_F(TelemetryEncoderTest, RejectsDuplicatePropertiesIncludingAfterInsertionShifts)
{
    TelemetryProperty properties[3] = {};

    properties[0].name = "A";
    properties[1].name = "Z";
    properties[2].name = "A";

    for (auto& property : properties)
    {
        property.type = TelemetryPropertyInt64;
    }

    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectFailure(EINVAL);
}

TEST_F(TelemetryEncoderTest, RejectsNullArgumentsWithoutChangingOutput)
{
    const auto original = buffer;

    encodedSize = 99;
    EXPECT_EQ(EINVAL, TelemetryEncodeEvent(NULL, buffer.data(), buffer.size(), &encodedSize, NULL));
    EXPECT_EQ(0U, encodedSize);
    EXPECT_EQ(original, buffer);

    encodedSize = 99;
    EXPECT_EQ(EINVAL, TelemetryEncodeEvent(&event, NULL, buffer.size(), &encodedSize, NULL));
    EXPECT_EQ(0U, encodedSize);
    EXPECT_EQ(EINVAL, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), NULL, NULL));
    EXPECT_EQ(original, buffer);
}

TEST_F(TelemetryEncoderTest, RejectsInvalidRequiredMetadata)
{
    event.name = NULL;
    ExpectFailure(EINVAL);
    event.name = "";
    ExpectFailure(EINVAL);
    event.name = "ABC";
    ExpectFailure(EINVAL);
    event.name = "Bad Name";
    ExpectFailure(EINVAL);
    event.name = "Test";
    event.iKey = NULL;
    ExpectFailure(EINVAL);
    event.iKey = "o:";
    ExpectFailure(EINVAL);
    event.iKey = "full-token";
    ExpectFailure(EINVAL);
    event.iKey = "o:t";
    event.time = 0;
    ExpectFailure(EINVAL);
    event.time = -1;
    ExpectFailure(EINVAL);
    event.time = 1;
    event.flags = -1;
    ExpectFailure(EINVAL);
    event.flags = 0;
    event.sequence = -1;
    ExpectFailure(EINVAL);
}

TEST_F(TelemetryEncoderTest, RejectsInvalidPropertyNamesAndValues)
{
    TelemetryProperty property = {};

    property.type = TelemetryPropertyString;
    property.value.stringValue = "x";
    UseProperty(property);

    for (const char* name : {static_cast<const char*>(NULL), "", ".P", "P.", "P Q", "P\n", "\xC3\xA9"})
    {
        property.name = name;
        ExpectFailure(EINVAL);
    }

    property.name = "P";
    property.value.stringValue = NULL;
    ExpectFailure(EINVAL);
    property.type = static_cast<TelemetryPropertyType>(99);
    ExpectFailure(EINVAL);
}

TEST_F(TelemetryEncoderTest, AcceptsNameLimitsAndRejectsOversizedNames)
{
    std::string name(TELEMETRY_MAX_NAME_LENGTH, 'A');
    TelemetryProperty property = {};

    event.name = name.c_str();
    ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
    name.push_back('A');
    event.name = name.c_str();
    ExpectFailure(EMSGSIZE);

    event.name = "Test";
    property.type = TelemetryPropertyBoolean;
    property.name = name.c_str();
    UseProperty(property);
    ExpectFailure(EMSGSIZE);
    name.pop_back();
    property.name = name.c_str();
    EXPECT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
}

TEST_F(TelemetryEncoderTest, RejectsNonFiniteDoubles)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyDouble;
    UseProperty(property);

    for (double value : {std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()})
    {
        property.value.doubleValue = value;
        ExpectFailure(EINVAL);
    }
}

TEST_F(TelemetryEncoderTest, RejectsMalformedUtf8WithoutTouchingBuffer)
{
    TelemetryProperty property = {};

    property.name = "P";
    property.type = TelemetryPropertyString;
    UseProperty(property);

    for (const char* text : {
        "\x80", "\xC0\x80", "\xC1\xBF", "\xC2", "\xC2" "A",
        "\xE0\x80\x80", "\xED\xA0\x80", "\xEF\xBF",
        "\xF0\x80\x80\x80", "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\xFF"})
    {
        property.value.stringValue = text;
        ExpectFailure(EILSEQ);
    }

    property.value.stringValue = "valid";
    event.deviceId = "\xED\xA0\x80";
    ExpectFailure(EILSEQ);
    event.deviceId = NULL;
    event.sdkVersion = "\xF4\x90\x80\x80";
    ExpectFailure(EILSEQ);
    event.sdkVersion = NULL;
    event.sdkEpoch = "\xC2";
    ExpectFailure(EILSEQ);
    event.sdkEpoch = NULL;
    event.iKey = "o:\x80";
    ExpectFailure(EILSEQ);
}

TEST_F(TelemetryEncoderTest, EnforcesPropertyArrayAndCountLimits)
{
    std::array<TelemetryProperty, TELEMETRY_MAX_PROPERTY_COUNT> properties = {};
    std::array<std::string, TELEMETRY_MAX_PROPERTY_COUNT> names = {};
    size_t i = 0;

    event.propertyCount = 1;
    ExpectFailure(EINVAL);

    for (i = 0; i < properties.size(); ++i)
    {
        names[i] = "P" + std::to_string(i);
        properties[i].name = names[i].c_str();
        properties[i].type = TelemetryPropertyBoolean;
    }

    event.properties = properties.data();
    event.propertyCount = properties.size();
    EXPECT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
    event.propertyCount = properties.size() + 1;
    ExpectFailure(EMSGSIZE);
}

TEST_F(TelemetryEncoderTest, RequiresExactCapacityAndLeavesFailedOutputUnchanged)
{
    size_t required = 0;

    ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
    required = encodedSize;
    ExpectFailure(EMSGSIZE, 0);
    ExpectFailure(EMSGSIZE, required - 1);
    buffer.fill(0xA5);
    EXPECT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), required, &encodedSize, NULL));
    EXPECT_EQ(required, encodedSize);
    EXPECT_EQ(0xA5, buffer[required]);
}

TEST_F(TelemetryEncoderTest, EnforcesSerializedLimitEvenWithLargerOutputBuffer)
{
    TelemetryProperty property = {};
    std::string value = {};
    size_t overhead = 0;

    property.name = "P";
    property.type = TelemetryPropertyString;
    value.assign(16000, 'x');
    property.value.stringValue = value.c_str();
    UseProperty(property);
    ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
    overhead = encodedSize - value.size();
    ASSERT_GT(static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE), overhead);
    value.resize(TELEMETRY_MAX_EVENT_SIZE - overhead, 'x');
    property.value.stringValue = value.c_str();
    ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &encodedSize, NULL));
    EXPECT_EQ(static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE), encodedSize);
    EXPECT_EQ(0xA5, buffer[TELEMETRY_MAX_EVENT_SIZE]);

    value.push_back('x');
    property.value.stringValue = value.c_str();
    ExpectFailure(EMSGSIZE, buffer.size());
    value.resize(TELEMETRY_MAX_EVENT_SIZE + 1, 'x');
    property.value.stringValue = value.c_str();
    ExpectFailure(EMSGSIZE, buffer.size());
}

TEST_F(TelemetryEncoderTest, BorrowsPropertiesOnlyForTheDurationOfTheCall)
{
    TelemetryProperty property = {};
    char value[] = "x";

    property.name = "P";
    property.type = TelemetryPropertyString;
    property.value.stringValue = value;
    UseProperty(property);
    ExpectEncoded(PropertyRecord({0x69, 0x01, 'x', 0x00}));
    value[0] = 'y';
    ExpectEncoded(PropertyRecord({0x69, 0x01, 'y', 0x00}));
}
