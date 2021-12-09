// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Tpm2Utils.h>
#include <Mmi.h>

class TestTpm : public Tpm
{
public:
    TestTpm(const unsigned int maxPayloadSizeBytes) : Tpm(maxPayloadSizeBytes)
    {
        m_hasCapabilitiesFile = true;
    }

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
    const unsigned int maxPayloadSizeBytes = 4000;

    const char* tpmVersionNumber = "\"1.2\"";
    const char* tpmManufacturerNameSTMicroelectronics = "\"STMicroelectronics\"";
    const char* tpmManufacturerNameLong = "\"Manufacturer name is long and contains numb3rs and $pec!@l characters\"";
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

    const std::string tpmDetected = std::to_string(TestTpm::Status::TpmDetected);
    const std::string tpmNotDetected = std::to_string(TestTpm::Status::TpmNotDetected);

    TEST(TpmTests, UnsignedInt8ToUnsignedInt64)
    {
        uint8_t test[8] = {0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61}; // Hexadecimal representation of {'t','e','s','t','d','a','t','a'}
        uint8_t* inputBuf = &test[0];
        uint32_t dataOffset = 0;
        uint32_t dataLength = 4;
        uint64_t data = 0;

        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(nullptr, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, nullptr), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, TPM_RESPONSE_MAX_SIZE, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, INT_MAX + 1, dataOffset, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, 0, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, TPM_RESPONSE_MAX_SIZE - 1, dataLength, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, 9, &data), EINVAL);
        EXPECT_EQ(Tpm2Utils::UnsignedInt8ToUnsignedInt64(inputBuf, TPM_RESPONSE_MAX_SIZE, dataOffset, dataLength, &data), MMI_OK);
        EXPECT_EQ(data, 0x74657374);
    }

    TEST(TpmTests, BufferToString)
    {
        unsigned char buf[9] = {'t', 'e', 's', 't', 'd', 'a', 't', 'a', '\0'};
        std::string str;

        EXPECT_EQ(Tpm2Utils::BufferToString(nullptr, str), EINVAL);
        EXPECT_EQ(str, empty);

        EXPECT_EQ(Tpm2Utils::BufferToString(&buf[0], str), MMI_OK);
        EXPECT_EQ(str, "testdata");
    }

    TEST(TpmTests, GetStatus)
    {
        TestTpm tpm(maxPayloadSizeBytes);
        tpm.m_commandOutput.push_back(tpmDeviceDirectory);

        std::string data;
        tpm.GetStatus(data);
        EXPECT_STREQ(data.c_str(), tpmDetected.c_str());

        data.clear();
        tpm.GetStatus(data);
        EXPECT_STREQ(data.c_str(), tpmNotDetected.c_str());
    }

    TEST(TpmTests, GetVersionFromCapabilitiesFile)
    {
        TestTpm tpm(maxPayloadSizeBytes);
        tpm.m_commandOutput.push_back(tpmDetails);

        std::string data;
        tpm.GetVersionFromCapabilitiesFile(data);
        EXPECT_STREQ(data.c_str(), tpmVersionNumber);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsLeadingAndTrailingWhitespace);
        tpm.GetVersionFromCapabilitiesFile(data);
        EXPECT_STREQ(data.c_str(), tpmVersionNumber);
    }

    TEST(TpmTests, GetManufacturerFromCapabilitiesFile)
    {
        TestTpm tpm(maxPayloadSizeBytes);
        tpm.m_commandOutput.push_back(tpmDetails);

        std::string data;
        tpm.GetManufacturerFromCapabilitiesFile(data);
        EXPECT_STREQ(data.c_str(), tpmManufacturerNameSTMicroelectronics);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsLeadingAndTrailingWhitespace);
        tpm.GetManufacturerFromCapabilitiesFile(data);
        EXPECT_STREQ(data.c_str(), tpmManufacturerNameSTMicroelectronics);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsManufacturerNameLong);
        tpm.GetManufacturerFromCapabilitiesFile(data);
        EXPECT_STREQ(data.c_str(), tpmManufacturerNameLong);
    }

    TEST(TpmTests, GetObjectNameUnknown)
    {
        const char* objectNameUnknown = "ObjectNameUnknown";
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        TestTpm tpm(maxPayloadSizeBytes);
        EXPECT_EQ(tpm.Get(objectNameUnknown, &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);
    }

    TEST(TpmTests, MmiGetInfo)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MmiGetInfo(nullptr, &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        EXPECT_EQ(MmiGetInfo(clientName, nullptr, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payloadSizeBytes, 0);

        EXPECT_EQ(MmiGetInfo(clientName, &payload, nullptr), EINVAL);
        EXPECT_EQ(payload, nullptr);

        EXPECT_EQ(MmiGetInfo(clientName, &payload, &payloadSizeBytes), MMI_OK);

        EXPECT_NE(payload, nullptr);
        delete payload;
    }

    TEST(TpmTests, MmiOpen)
    {
        MMI_HANDLE handle = MmiOpen(nullptr, maxPayloadSizeBytes);
        EXPECT_EQ(handle, nullptr);

        handle = MmiOpen(clientName, maxPayloadSizeBytes);
        EXPECT_NE(handle, nullptr);

        Tpm* tpm = reinterpret_cast<Tpm*>(handle);

        EXPECT_NE(tpm, nullptr);
        delete tpm;
    }

    TEST(TpmTests, MmiGet)
    {
        TestTpm tpm = TestTpm(maxPayloadSizeBytes);

        MMI_HANDLE clientSession = reinterpret_cast<void*>(&tpm);
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        EXPECT_EQ(MmiGet(nullptr, TPM, TPM_STATUS, &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* componentNameUnknown = "ComponentNameUnknown";
        EXPECT_EQ(MmiGet(clientSession, componentNameUnknown, TPM_STATUS, &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* objectNameUnknown = "ObjectNameUnknown";
        EXPECT_EQ(MmiGet(clientSession, TPM, objectNameUnknown, &payload, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        EXPECT_EQ(MmiGet(clientSession, TPM, TPM_STATUS, nullptr, &payloadSizeBytes), EINVAL);
        EXPECT_EQ(payloadSizeBytes, 0);

        EXPECT_EQ(MmiGet(clientSession, TPM, TPM_STATUS, &payload, nullptr), EINVAL);
        EXPECT_EQ(payload, nullptr);

        tpm.m_commandOutput.push_back(tpmDeviceDirectory);

        EXPECT_EQ(MmiGet(clientSession, TPM, TPM_STATUS, &payload, &payloadSizeBytes), MMI_OK);

        std::string result(payload, payloadSizeBytes);
        EXPECT_STREQ(result.c_str(), tpmDetected.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;
    }
}