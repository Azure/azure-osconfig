// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Firewall.h>

namespace tests
{
    static const std::string expectedFingerprint = "abc123";
    static const std::string expectedFingerprintJson = "\"" + expectedFingerprint + "\"";

    class MockUtility : public GenericFirewall
    {
    public:
        typedef GenericFirewall::State State;

        MockUtility() = default;
        ~MockUtility() = default;

        State Detect() const override;
        std::string Fingerprint() const override;
    };

    MockUtility::State MockUtility::Detect() const
    {
        return State::Enabled;
    }

    std::string MockUtility::Fingerprint() const
    {
        return expectedFingerprint;
    }

    class FirewallTests : public ::testing::Test
    {
    protected:
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        std::shared_ptr<FirewallModule<MockUtility>> firewall;

        void SetUp() override {
            payload = nullptr;
            payloadSizeBytes = 0;
            firewall = std::make_shared<FirewallModule<MockUtility>>(0);
        }

        void TearDown() override {
            if (nullptr != payload)
            {
                delete[] payload;
            }
            payloadSizeBytes = 0;
            firewall.reset();
        }
    };

    TEST_F(FirewallTests, GetInfo)
    {
        EXPECT_EQ(MMI_OK, Firewall::GetInfo("test_client", &payload, &payloadSizeBytes));
        EXPECT_STREQ(payload, Firewall::m_moduleInfo.c_str());
        EXPECT_EQ(payloadSizeBytes, strlen(Firewall::m_moduleInfo.c_str()));
    }

    TEST_F(FirewallTests, GetInvalidInput)
    {
        EXPECT_EQ(EINVAL, firewall->Get("invalid_component", Firewall::m_firewallReportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), "invalid_object", &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_firewallReportedFingerprint.c_str(), nullptr, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_firewallReportedFingerprint.c_str(), &payload, nullptr));
    }

    TEST_F(FirewallTests, GetFingerprint)
    {
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_firewallReportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedFingerprintJson.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprintJson.size());
    }

    TEST_F(FirewallTests, GetState)
    {
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_firewallReportedState.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), "1");
        EXPECT_EQ(payloadSizeBytes, strlen("1"));
    }
} // namespace tests