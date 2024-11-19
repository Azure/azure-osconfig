// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <CommonUtils.h>
#include <Mmi.h>
#include <Tpm.h>

class TestTpm : public Tpm
{
public:
    TestTpm(const unsigned int maxPayloadSizeBytes) : Tpm(maxPayloadSizeBytes) {}

    ~TestTpm();
    std::string RunCommand(const char* command) override;
    std::vector<std::string> m_commandOutput;
    size_t m_callsToRunCommand = 0;
};

TestTpm::~TestTpm() {}

std::string TestTpm::RunCommand(const char* command)
{
    UNUSED(command);

    std::string commandOutput;
    if (this->m_callsToRunCommand < this->m_commandOutput.size())
    {
        commandOutput = this->m_commandOutput[this->m_callsToRunCommand++];
    }

    return commandOutput;
}

namespace OSConfig::Platform::Tests
{
    const char* tpmVersion = "1.2";
    const char* tpmManufacturer = "STMicroelectronics";
    const char* tpmManufacturerNameLong = "Manufacturer name is long and contains numb3rs and $pec!@l characters";
    const char* empty = "";
    const char* clientName = "ClientName";

    const std::string tpmDeviceDirectory = "/dev/tpm0";
    const std::string tpmDetails = "Manufacturer: 0x53544d6963726f656c656374726f6e696373\n"
                                   "TCG version: 1.2\n";
    const std::string tpmDetailsLeadingAndTrailingWhitespace = "Manufacturer: 0x202053544d6963726f656c656374726f6e6963732020\n"
                                   "TCG version:   1.2  \n";
    const std::string tpmDetailsManufacturerNameLong =
    "Manufacturer: 0x4d616e756661637475726572206e61"
    "6d65206973206c6f6e6720616e642063"
    "6f6e7461696e73206e756d6233727320"
    "616e64202470656321406c2063686172"
    "616374657273\n"
    "TCG version: 1.2\n";

    class TpmTests : public ::testing::Test
    {
    protected:
        std::shared_ptr<TestTpm> tpm;

        void SetUp() override
        {
            tpm = std::make_shared<TestTpm>(0);
        }

        void TearDown() override
        {
            tpm.reset();
        }
    };

    TEST_F(TpmTests, HexToString)
    {
        EXPECT_EQ(0x0, Tpm::HexVal('0'));
        EXPECT_EQ(0x1, Tpm::HexVal('1'));
        EXPECT_EQ(0x2, Tpm::HexVal('2'));
        EXPECT_EQ(0x3, Tpm::HexVal('3'));
        EXPECT_EQ(0x4, Tpm::HexVal('4'));
        EXPECT_EQ(0x5, Tpm::HexVal('5'));
        EXPECT_EQ(0x6, Tpm::HexVal('6'));
        EXPECT_EQ(0x7, Tpm::HexVal('7'));
        EXPECT_EQ(0x8, Tpm::HexVal('8'));
        EXPECT_EQ(0x9, Tpm::HexVal('9'));
        EXPECT_EQ(0xA, Tpm::HexVal('A'));
        EXPECT_EQ(0xB, Tpm::HexVal('B'));
        EXPECT_EQ(0xC, Tpm::HexVal('C'));
        EXPECT_EQ(0xD, Tpm::HexVal('D'));
        EXPECT_EQ(0xE, Tpm::HexVal('E'));
        EXPECT_EQ(0xF, Tpm::HexVal('F'));
        EXPECT_EQ(0xA, Tpm::HexVal('a'));
        EXPECT_EQ(0xB, Tpm::HexVal('b'));
        EXPECT_EQ(0xC, Tpm::HexVal('c'));
        EXPECT_EQ(0xD, Tpm::HexVal('d'));
        EXPECT_EQ(0xE, Tpm::HexVal('e'));
        EXPECT_EQ(0xF, Tpm::HexVal('f'));

        std::string str = "7465737464617461";  // Hexadecimal string representation of "testdata"
        std::string expected = "testdata";
        std::string actual = Tpm::HexToString(str);
        EXPECT_STREQ(expected.c_str(), actual.c_str());
    }

    TEST_F(TpmTests, UnsignedInt8ToUnsignedInt64)
    {
        uint8_t test[8] = {0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61}; // Hexadecimal representation of {'t','e','s','t','d','a','t','a'}
        uint8_t* inputBuf = &test[0];
        uint32_t dataOffset = 0;
        uint32_t dataLength = 4;
        uint64_t data = 0;

        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(nullptr, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, nullptr), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, TPM_RESPONSE_MAX_SIZE, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, INT_MAX + 1, dataOffset, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, 0, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, TPM_RESPONSE_MAX_SIZE - 1, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, 9, &data), EINVAL);
        EXPECT_EQ(Tpm::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, &data), MMI_OK);
        EXPECT_EQ(data, 0x74657374);
    }

    TEST_F(TpmTests, LoadProperties)
    {
        std::string expectedVersion = "\"" + std::string(tpmVersion) + "\"";
        std::string expectedManufacturer = "\"" + std::string(tpmManufacturer) + "\"";

        tpm->m_commandOutput.push_back(tpmDeviceDirectory);
        tpm->m_commandOutput.push_back(tpmDetails);

        tpm->LoadProperties();

        EXPECT_EQ(Tpm::Status::TpmDetected, tpm->GetStatus());
        EXPECT_EQ(expectedVersion, tpm->GetVersion().c_str());
        EXPECT_EQ(expectedManufacturer, tpm->GetManufacturer().c_str());
    }

    TEST_F(TpmTests, GetPropertiesFromCapabilitiesFile)
    {
        Tpm::Properties properties;

        tpm->m_commandOutput.push_back(tpmDetails);

        EXPECT_EQ(0, tpm->GetPropertiesFromCapabilitiesFile(properties));
        EXPECT_STREQ(tpmVersion, properties.version.c_str());
        EXPECT_STREQ(tpmManufacturer, properties.manufacturer.c_str());

        tpm->m_commandOutput.push_back(tpmDetailsLeadingAndTrailingWhitespace);

        EXPECT_EQ(0, tpm->GetPropertiesFromCapabilitiesFile(properties));
        EXPECT_STREQ(tpmVersion, properties.version.c_str());
        EXPECT_STREQ(tpmManufacturer, properties.manufacturer.c_str());

        tpm->m_commandOutput.push_back(tpmDetailsManufacturerNameLong);

        EXPECT_EQ(0, tpm->GetPropertiesFromCapabilitiesFile(properties));
        EXPECT_STREQ(tpmVersion, properties.version.c_str());
        EXPECT_STREQ(tpmManufacturerNameLong, properties.manufacturer.c_str());
    }

    TEST_F(TpmTests, GetInvalidObject)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        tpm->m_commandOutput.push_back(tpmDeviceDirectory);
        tpm->m_commandOutput.push_back(tpmDetails);

        EXPECT_EQ(tpm->Get(Tpm::m_tpm.c_str(), Tpm::m_tpmStatus.c_str(), nullptr, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(tpm->Get(Tpm::m_tpm.c_str(), Tpm::m_tpmStatus.c_str(), &payload, nullptr), EINVAL);
        EXPECT_EQ(tpm->Get(Tpm::m_tpm.c_str(), "unknown", &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        FREE_MEMORY(payload);
    }

    TEST_F(TpmTests, GetStatus)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        std::string expectedStatus = std::to_string(static_cast<int>(Tpm::Status::TpmDetected));

        tpm->m_commandOutput.push_back(tpmDeviceDirectory);
        tpm->m_commandOutput.push_back(tpmDetails);

        EXPECT_EQ(MMI_OK, tpm->Get(Tpm::m_tpm.c_str(), Tpm::m_tpmStatus.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(payloadSizeBytes, expectedStatus.length());
        ASSERT_NE(payload, nullptr);
        std::string payloadString(payload, payloadSizeBytes);
        EXPECT_STREQ(expectedStatus.c_str(), payloadString.c_str());

        FREE_MEMORY(payload);
    }

    TEST_F(TpmTests, GetVersion)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        std::string expectedVersion = "\"" + std::string(tpmVersion) + "\"";

        tpm->m_commandOutput.push_back(tpmDeviceDirectory);
        tpm->m_commandOutput.push_back(tpmDetails);

        EXPECT_EQ(MMI_OK, tpm->Get(Tpm::m_tpm.c_str(), Tpm::m_tpmVersion.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(payloadSizeBytes, expectedVersion.length());
        ASSERT_NE(payload, nullptr);
        std::string payloadString(payload, payloadSizeBytes);
        EXPECT_STREQ(expectedVersion.c_str(), payloadString.c_str());

        FREE_MEMORY(payload);
    }

    TEST_F(TpmTests, GetManufacturer)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        std::string expectedManufacturer = "\"" + std::string(tpmManufacturer) + "\"";

        tpm->m_commandOutput.push_back(tpmDeviceDirectory);
        tpm->m_commandOutput.push_back(tpmDetails);

        EXPECT_EQ(MMI_OK, tpm->Get(Tpm::m_tpm.c_str(), Tpm::m_tpmManufacturer.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(payloadSizeBytes, expectedManufacturer.length());
        ASSERT_NE(payload, nullptr);
        std::string payloadString(payload, payloadSizeBytes);
        EXPECT_STREQ(expectedManufacturer.c_str(), payloadString.c_str());

        FREE_MEMORY(payload);
    }
}
