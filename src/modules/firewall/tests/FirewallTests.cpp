// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Firewall.h>

namespace tests
{
    static const std::string expectedFingerprint = "abc123";
    static const std::string expectedFingerprintJson = "\"" + expectedFingerprint + "\"";

    class MockUtility : public GenericUtility
    {
    public:
        typedef GenericUtility::State State;

        MockUtility() = default;
        ~MockUtility() = default;

        State Detect() const override;
        std::string Hash() const override;
    };

    MockUtility::State MockUtility::Detect() const
    {
        return State::Enabled;
    }

    std::string MockUtility::Hash() const
    {
        return expectedFingerprint;
    }

    class FirewallTests : public ::testing::Test
    {
    protected:
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        std::shared_ptr<GenericFirewall<MockUtility>> firewall;

        void SetUp() override {
            payload = nullptr;
            payloadSizeBytes = 0;
            firewall = std::make_shared<GenericFirewall<MockUtility>>(0);
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
        EXPECT_STREQ(payload, Firewall::MODULE_INFO.c_str());
        EXPECT_EQ(payloadSizeBytes, strlen(Firewall::MODULE_INFO.c_str()));
    }

    TEST_F(FirewallTests, GetInvalidInput)
    {
        EXPECT_EQ(EINVAL, firewall->Get("invalid_component", Firewall::FIREWALL_REPORTED_FINGERPRINT.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::FIREWALL_COMPONENT.c_str(), "invalid_object", &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::FIREWALL_COMPONENT.c_str(), Firewall::FIREWALL_REPORTED_FINGERPRINT.c_str(), nullptr, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::FIREWALL_COMPONENT.c_str(), Firewall::FIREWALL_REPORTED_FINGERPRINT.c_str(), &payload, nullptr));
    }

    TEST_F(FirewallTests, GetFingerprint)
    {
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::FIREWALL_COMPONENT.c_str(), Firewall::FIREWALL_REPORTED_FINGERPRINT.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(payload, expectedFingerprintJson.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprintJson.size());
    }

    TEST_F(FirewallTests, GetState)
    {
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::FIREWALL_COMPONENT.c_str(), Firewall::FIREWALL_REPORTED_STATE.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(payload, "1");
        EXPECT_EQ(payloadSizeBytes, strlen("1"));
    }
} // namespace tests