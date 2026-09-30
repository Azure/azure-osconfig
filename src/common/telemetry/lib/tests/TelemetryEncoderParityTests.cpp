// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <TelemetryEncoder.h>

#include <CsProtocol_types.hpp>
#include <bond/CompactBinaryProtocolWriter.hpp>
#include <bond/generated/CsProtocol_writers.hpp>

#include <array>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

static_assert(CsProtocol::CS_VER_MAJOR == 3, "Encoder parity requires the shipping Common Schema 3 configuration");

class TelemetryEncoderParityTest : public ::testing::Test
{
protected:
    TelemetryEvent event = {};

    void SetUp() override
    {
        event.name = "Test";
        event.iKey = "o:parity";
        event.time = 1;
    }

    static void RequireDefinedSdkShift(int64_t value)
    {
        // The pinned SDK shifts signed values left. Keep that oracle defined;
        // the common encoder tests cover negative values and integer extremes.
        if ((value < 0) || (value > std::numeric_limits<int64_t>::max() / 2))
        {
            throw std::out_of_range("SDK parity input would invoke an undefined signed shift");
        }
    }

    static TelemetryProperty StringProperty(const char* name, const char* value)
    {
        TelemetryProperty property = {};
        property.name = name;
        property.type = TelemetryPropertyString;
        property.value.stringValue = value;
        return property;
    }

    std::vector<unsigned char> EncodeWithSdk() const
    {
        RequireDefinedSdkShift(event.time);
        RequireDefinedSdkShift(event.flags);
        RequireDefinedSdkShift(event.sequence);

        CsProtocol::Record record;
        record.ver = CsProtocol::CS_VER_STRING;
        record.name = event.name;
        record.time = event.time;
        record.iKey = event.iKey;
        record.flags = event.flags;
        record.baseType = "custom";

        if ((NULL != event.deviceId) && ('\0' != event.deviceId[0]))
        {
            record.extDevice.resize(1);
            record.extDevice[0].localId = event.deviceId;
        }

        const std::string sdkVersion = (NULL != event.sdkVersion) ? event.sdkVersion : "";
        const std::string sdkEpoch = (NULL != event.sdkEpoch) ? event.sdkEpoch : "";

        if (!sdkVersion.empty() || !sdkEpoch.empty() || (0 != event.sequence))
        {
            record.extSdk.resize(1);
            record.extSdk[0].libVer = sdkVersion;
            record.extSdk[0].epoch = sdkEpoch;
            record.extSdk[0].seq = event.sequence;
        }

        record.data.resize(1);

        for (size_t i = 0; i < event.propertyCount; ++i)
        {
            const TelemetryProperty& property = event.properties[i];
            CsProtocol::Value value;

            switch (property.type)
            {
                case TelemetryPropertyString:
                    value.type = CsProtocol::ValueKind::ValueString;
                    value.stringValue = property.value.stringValue;
                    break;

                case TelemetryPropertyInt64:
                    RequireDefinedSdkShift(property.value.int64Value);
                    value.type = CsProtocol::ValueKind::ValueInt64;
                    value.longValue = property.value.int64Value;
                    break;

                case TelemetryPropertyDouble:
                    value.type = CsProtocol::ValueKind::ValueDouble;
                    value.doubleValue = property.value.doubleValue;
                    break;

                case TelemetryPropertyBoolean:
                    value.type = CsProtocol::ValueKind::ValueBool;
                    value.longValue = property.value.booleanValue ? 1 : 0;
                    break;

                default:
                    throw std::invalid_argument("Unsupported property type in SDK parity fixture");
            }

            if (!record.data[0].properties.emplace(property.name, value).second)
            {
                throw std::invalid_argument("Duplicate property name in SDK parity fixture");
            }
        }

        std::vector<unsigned char> bytes;
        bond_lite::CompactBinaryProtocolWriter writer(bytes);
        bond_lite::Serialize(writer, record, false);
        return bytes;
    }

    void ExpectMatchesSdk()
    {
        const auto expected = EncodeWithSdk();
        std::array<unsigned char, TELEMETRY_MAX_EVENT_SIZE + 1> buffer = {};
        size_t size = 0;
        buffer.fill(0xA5);

        ASSERT_LE(expected.size(), static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE));
        ASSERT_EQ(0, TelemetryEncodeEvent(&event, buffer.data(), TELEMETRY_MAX_EVENT_SIZE, &size, NULL));
        ASSERT_EQ(expected.size(), size);
        EXPECT_EQ(expected, std::vector<unsigned char>(buffer.begin(), buffer.begin() + size));
        EXPECT_EQ(0xA5, buffer[size]);
        EXPECT_EQ(0xA5, buffer[TELEMETRY_MAX_EVENT_SIZE]);
    }
};

TEST_F(TelemetryEncoderParityTest, MinimalRecordMatchesSdk)
{
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, TimestampAndFlagsMatchSdk)
{
    event.time = INT64_C(621355968000000000);

    for (int64_t flags : {0, 0x0101, 0x0202, 0x0301})
    {
        SCOPED_TRACE(flags);
        event.flags = flags;
        ExpectMatchesSdk();
    }
}

TEST_F(TelemetryEncoderParityTest, BaselinePropertiesMatchSdk)
{
    event.name = "BaselineRun";
    event.time = INT64_C(621355968000000000);
    event.flags = 0x0202;
    const TelemetryProperty properties[] = {
        StringProperty("DistroName", "Test Linux"),
        StringProperty("CorrelationId", "test-run"),
        StringProperty("Version", "test-version"),
        StringProperty("Timestamp", "1970-01-01 00:00:00+0000"),
        StringProperty("BaselineName", "Test Baseline"),
        StringProperty("Mode", "audit-only"),
        StringProperty("DurationSeconds", "0.50")
    };
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, ErrorPropertiesMatchSdk)
{
    event.name = "StatusTrace";
    const TelemetryProperty properties[] = {
        StringProperty("DistroName", "Test Linux"),
        StringProperty("CorrelationId", "test-run"),
        StringProperty("Version", "test-version"),
        StringProperty("Timestamp", "1970-01-01 00:00:00+0000"),
        StringProperty("FileName", "Test.c"),
        StringProperty("LineNumber", "123"),
        StringProperty("ScenarioName", "TestScenario"),
        StringProperty("FunctionName", "TestFunction"),
        StringProperty("RuleCodename", "TestRule"),
        StringProperty("CallingFunctionName", "TestCaller"),
        StringProperty("Microseconds", "42"),
        StringProperty("ResultCode", "5"),
        StringProperty("ResultString", "Test failure")
    };
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, CrashPropertiesMatchSdk)
{
    event.name = "CrashDetected";
    const TelemetryProperty properties[] = {
        StringProperty("DistroName", "Test Linux"),
        StringProperty("CorrelationId", "reporting-run"),
        StringProperty("Version", "test-version"),
        StringProperty("Timestamp", "1970-01-01 00:00:00+0000"),
        StringProperty("CrashInfo", "Synthetic previous-crash evidence\nframe 0")
    };
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, OptionalDeviceAndSdkMetadataMatchSdk)
{
    for (const char* deviceId : {static_cast<const char*>(NULL), "", "c:test-device"})
    {
        event.deviceId = deviceId;
        ExpectMatchesSdk();
    }

    event.sdkVersion = "";
    event.sdkEpoch = "";
    ExpectMatchesSdk();
    event.sdkVersion = "test-client-version";
    ExpectMatchesSdk();
    event.sdkEpoch = "test-epoch";
    ExpectMatchesSdk();
    event.sequence = 128;
    ExpectMatchesSdk();
    event.sdkVersion = NULL;
    ExpectMatchesSdk();
    event.sdkEpoch = NULL;
    ExpectMatchesSdk();
    event.sdkEpoch = "test-epoch";
    event.sequence = 0;
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, TypedDefaultValuesMatchSdk)
{
    TelemetryProperty properties[4] = {};
    properties[0] = StringProperty("EmptyString", "");
    properties[1].name = "ZeroInteger";
    properties[1].type = TelemetryPropertyInt64;
    properties[1].value.int64Value = 0;
    properties[2].name = "ZeroDouble";
    properties[2].type = TelemetryPropertyDouble;
    properties[2].value.doubleValue = 0.0;
    properties[3].name = "FalseBoolean";
    properties[3].type = TelemetryPropertyBoolean;
    properties[3].value.booleanValue = false;
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, IntegerPrecisionAndVarintBoundariesMatchSdk)
{
    const int64_t values[] = {
        0, 1, 63, 64, 127, 128, 8191, 8192,
        INT64_C(9007199254740991), INT64_C(9007199254740992), INT64_C(9007199254740993),
        std::numeric_limits<int64_t>::max() / 2
    };
    TelemetryProperty property = {};
    property.name = "Integer";
    property.type = TelemetryPropertyInt64;
    event.properties = &property;
    event.propertyCount = 1;

    for (int64_t value : values)
    {
        SCOPED_TRACE(value);
        property.value.int64Value = value;
        ExpectMatchesSdk();
    }
}

TEST_F(TelemetryEncoderParityTest, FiniteDoublesMatchSdk)
{
    const uint16_t byteOrder = 1;

    if (1 != *reinterpret_cast<const unsigned char*>(&byteOrder))
    {
        GTEST_SKIP() << "The pinned SDK writes native-endian doubles; the common tests retain little-endian fixtures";
    }

    const double values[] = {
        0.0, -0.0, 1.5, -1.0, 1.0e-100, 1.0e100,
        std::numeric_limits<double>::min(), std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest(), std::numeric_limits<double>::denorm_min()
    };
    TelemetryProperty property = {};
    property.name = "Double";
    property.type = TelemetryPropertyDouble;
    event.properties = &property;
    event.propertyCount = 1;

    for (double value : values)
    {
        SCOPED_TRACE(value);
        property.value.doubleValue = value;
        ExpectMatchesSdk();
    }
}

TEST_F(TelemetryEncoderParityTest, Utf8AndStringLengthBoundariesMatchSdk)
{
    TelemetryProperty property = StringProperty("Text", "");
    event.properties = &property;
    event.propertyCount = 1;

    for (const char* value : {"", "\"\\\n\t", "\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"})
    {
        property.value.stringValue = value;
        ExpectMatchesSdk();
    }

    for (size_t length : {127U, 128U, 16300U})
    {
        SCOPED_TRACE(length);
        const std::string value(length, 'x');
        property.value.stringValue = value.c_str();
        ExpectMatchesSdk();
    }
}

TEST_F(TelemetryEncoderParityTest, MixedTypesAndPropertyOrderMatchSdk)
{
    TelemetryProperty properties[3] = {};
    properties[0] = StringProperty("Z", "last");
    properties[1].name = "A";
    properties[1].type = TelemetryPropertyBoolean;
    properties[1].value.booleanValue = true;
    properties[2].name = "Middle";
    properties[2].type = TelemetryPropertyInt64;
    properties[2].value.int64Value = 128;
    event.properties = properties;
    event.propertyCount = ARRAY_SIZE(properties);
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, MaximumNamesAndPropertyCountMatchSdk)
{
    std::array<TelemetryProperty, TELEMETRY_MAX_PROPERTY_COUNT> properties = {};
    std::array<std::string, TELEMETRY_MAX_PROPERTY_COUNT> names = {};
    const std::string eventName(TELEMETRY_MAX_NAME_LENGTH, 'E');

    for (size_t i = 0; i < properties.size(); ++i)
    {
        names[i] = "P" + std::to_string(properties.size() - i);
        names[i].append(TELEMETRY_MAX_NAME_LENGTH - names[i].size(), 'x');
        properties[i].name = names[i].c_str();
        properties[i].type = TelemetryPropertyInt64;
        properties[i].value.int64Value = static_cast<int64_t>(i);
    }

    event.name = eventName.c_str();
    event.properties = properties.data();
    event.propertyCount = properties.size();
    ExpectMatchesSdk();
}

TEST_F(TelemetryEncoderParityTest, SdkMeasuredEventLimitMatchesAndExcessIsRejected)
{
    std::string value(16000, 'x');
    TelemetryProperty property = StringProperty("Text", value.c_str());
    event.properties = &property;
    event.propertyCount = 1;

    const size_t overhead = EncodeWithSdk().size() - value.size();
    ASSERT_LT(overhead, static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE));
    value.resize(TELEMETRY_MAX_EVENT_SIZE - overhead, 'x');
    property.value.stringValue = value.c_str();
    ASSERT_EQ(static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE), EncodeWithSdk().size());
    ExpectMatchesSdk();

    value.push_back('x');
    property.value.stringValue = value.c_str();
    ASSERT_EQ(static_cast<size_t>(TELEMETRY_MAX_EVENT_SIZE + 1), EncodeWithSdk().size());
    std::array<unsigned char, TELEMETRY_MAX_EVENT_SIZE + 1> buffer = {};
    buffer.fill(0xA5);
    const auto original = buffer;
    size_t size = 99;
    EXPECT_EQ(EMSGSIZE, TelemetryEncodeEvent(&event, buffer.data(), buffer.size(), &size, NULL));
    EXPECT_EQ(0U, size);
    EXPECT_EQ(original, buffer);
}
