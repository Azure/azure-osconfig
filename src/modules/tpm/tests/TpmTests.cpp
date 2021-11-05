// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Tpm.h>
#include <Mmi.h>

class TestTpm : public Tpm
{
public:
    TestTpm(int maxPayloadSizeBytes) : Tpm(maxPayloadSizeBytes) {}
    ~TestTpm();
    std::string RunCommand(const char* command) override;
    std::vector<std::string> m_commandOutput;
    int m_maxPayloadSizeBytes;
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
    const int maxPayloadSizeBytes = 4000;

    const char* tpmVersionNumber = "\"1.2\"";
    const char* microsoftVirtualTpmVersionNumber = "\"2.0\"";
    const char* tpmManufacturerNameSTMicroelectronics = "\"STMicroelectronics\"";
    const char* tpmManufacturerNameMicrosoft = "\"Microsoft\"";
    const char* tpmManufacturerNameLong = "\"Manufacturer name is long and contains numb3rs and $pec!@l characters\"";
    const char* empty = "";
    const char* quotes = "\"\"";
    const char* clientName = "ClientName";

    const std::string tpmDeviceDirectory = "/dev/tpm0";
    const std::string tpmDetails = "Manufacturer: 0x53544d6963726f656c656374726f6e696373\n"
                                   "TCG version: 1.2\n";
    const std::string tpmDetailsLeadingAndTrailingWhitespace = "Manufacturer: 0x202053544d6963726f656c656374726f6e6963732020\n"
                                   "TCG version:   1.2  \n";
    const std::string tpmDetailsVersionNotFound = "Manufacturer: 0x53544d6963726f656c656374726f6e696373\n";
    const std::string tpmDetailsManufacturerNotFound = "TCG version: 1.2\n";
    const std::string tpmDetailsManufacturerNameLong =
    "Manufacturer: 0x4d616e756661637475726572206e61"
    "6d65206973206c6f6e6720616e642063"
    "6f6e7461696e73206e756d6233727320"
    "616e64202470656321406c2063686172"
    "616374657273\n"
    "TCG version: 1.2\n";
    const std::string microsoftVirtualTpmDetails = "Microsoft Virtual TPM 2.0";

    const std::string tpmDetected = std::to_string(TestTpm::Status::TpmDetected);
    const std::string tpmNotDetected = std::to_string(TestTpm::Status::TpmNotDetected);

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

    TEST(TpmTests, GetVersion)
    {
        TestTpm tpm(maxPayloadSizeBytes);
        tpm.m_commandOutput.push_back(tpmDetails);

        std::string data;
        tpm.GetVersion(data);

        EXPECT_STREQ(data.c_str(), tpmVersionNumber);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsLeadingAndTrailingWhitespace);
        tpm.GetVersion(data);

        EXPECT_STREQ(data.c_str(), tpmVersionNumber);

        data.clear();
        tpm.m_commandOutput.push_back(empty);
        tpm.m_commandOutput.push_back(microsoftVirtualTpmDetails);
        tpm.GetVersion(data);

        EXPECT_STREQ(data.c_str(), microsoftVirtualTpmVersionNumber);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsVersionNotFound);
        tpm.GetVersion(data);

        EXPECT_STREQ(data.c_str(), quotes);
    }

    TEST(TpmTests, GetManufacturer)
    {
        TestTpm tpm(maxPayloadSizeBytes);
        tpm.m_commandOutput.push_back(tpmDetails);

        std::string data;
        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), tpmManufacturerNameSTMicroelectronics);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsLeadingAndTrailingWhitespace);
        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), tpmManufacturerNameSTMicroelectronics);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsManufacturerNameLong);
        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), tpmManufacturerNameLong);

        data.clear();
        tpm.m_commandOutput.push_back(empty);
        tpm.m_commandOutput.push_back(microsoftVirtualTpmDetails);
        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), tpmManufacturerNameMicrosoft);

        data.clear();
        tpm.m_commandOutput.push_back(tpmDetailsManufacturerNotFound);
        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), quotes);

        tpm.GetManufacturer(data);

        EXPECT_STREQ(data.c_str(), quotes);
    }

    TEST(TpmTests, GetObjectNameUnknown)
    {
        const char* objectNameUnknown = "ObjectNameUnknown";
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        TestTpm tpm(maxPayloadSizeBytes);
        int status = tpm.Get(objectNameUnknown, &payload, &payloadSizeBytes);

        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);
    }

    TEST(TpmTests, MmiGetInfo)
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

        int status = MmiGet(nullptr, TPM, TPM_STATUS, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* componentNameUnknown = "ComponentNameUnknown";
        status = MmiGet(clientSession, componentNameUnknown, TPM_STATUS, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        const char* objectNameUnknown = "ObjectNameUnknown";
        status = MmiGet(clientSession, TPM, objectNameUnknown, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payload, nullptr);
        EXPECT_EQ(payloadSizeBytes, 0);

        status = MmiGet(clientSession, TPM, TPM_STATUS, nullptr, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payloadSizeBytes, 0);

        status = MmiGet(clientSession, TPM, TPM_STATUS, &payload, nullptr);
        EXPECT_EQ(status, EINVAL);
        EXPECT_EQ(payload, nullptr);

        tpm.m_commandOutput.push_back(tpmDeviceDirectory);

        status = MmiGet(clientSession, TPM, TPM_STATUS, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string result(payload, payloadSizeBytes);
        EXPECT_STREQ(result.c_str(), tpmDetected.c_str());

        EXPECT_NE(payload, nullptr);
        delete payload;
    }
}