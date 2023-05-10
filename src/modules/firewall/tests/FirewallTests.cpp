// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <parson.h>

#include <Firewall.h>

namespace tests
{
    testing::AssertionResult JsonEq(const std::string& expectedJson, const std::string& actualJson)
    {
        JSON_Value* expected = json_parse_string(expectedJson.c_str());
        JSON_Value* actual = json_parse_string(actualJson.c_str());

        testing::AssertionResult result = testing::AssertionSuccess();

        if (expected == nullptr)
        {
            result = testing::AssertionFailure() << "Expected JSON is invalid";
        }
        else if (actual == nullptr)
        {
            result = testing::AssertionFailure() << "Actual JSON is invalid";
        }
        else if (!json_value_equals(expected, actual))
        {
            result = testing::AssertionFailure() << "JSON values are not equal";
        }

        return result;
    }

    class MockFirewall : public GenericFirewall<IpTablesRule, IpTablesPolicy>
    {
    public:
        typedef GenericFirewall::State State;
        typedef IpTablesPolicy Policy;
        typedef IpTablesRule Rule;

        MockFirewall() = default;
        ~MockFirewall() = default;

        State Detect() const override;
        std::string Fingerprint() const override;
        std::vector<Policy> GetDefaultPolicies() const override;

        int SetRules(const std::vector<Rule>& rules) override;
        int SetDefaultPolicies(const std::vector<Policy> policies) override;

    private:
        std::vector<Policy> m_defaultPolicies;
        std::vector<Rule> m_rules;
    };

    MockFirewall::State MockFirewall::Detect() const
    {
        return (!m_rules.empty() && !m_defaultPolicies.empty()) ? State::Enabled : State::Disabled;
    }

    std::string MockFirewall::Fingerprint() const
    {
        // For testing purposes, return the number of rules plus the number of default policies.
        return std::to_string(m_rules.size() + m_defaultPolicies.size());
    }

    std::vector<IpTablesPolicy> MockFirewall::GetDefaultPolicies() const
    {
        return m_defaultPolicies;
    }

    int MockFirewall::SetRules(const std::vector<IpTablesRule>& rules)
    {
        std::vector<std::string> errors;

        for (const auto& rule : rules)
        {
            if (!rule.HasParseError())
            {
                m_rules.push_back(rule);
                OsConfigLogInfo(FirewallLog::Get(), "Added rule: %s", rule.Specification().c_str());
            }
            else
            {
                auto ruleErrors = rule.GetParseError();
                errors.insert(errors.end(), ruleErrors.begin(), ruleErrors.end());
            }
        }

        m_ruleStatusMessage.clear();

        for (const std::string& error : errors)
        {
            m_ruleStatusMessage += error + "\n";
        }

        return errors.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    int MockFirewall::SetDefaultPolicies(const std::vector<IpTablesPolicy> policies)
    {
        std::vector<std::string> errors;

        for (const auto& policy : policies)
        {
            if (!policy.HasParseError())
            {
                m_defaultPolicies.push_back(policy);
            }
            else
            {
                auto policyErrors = policy.GetParseError();
                errors.insert(errors.end(), policyErrors.begin(), policyErrors.end());
            }
        }

        m_policyStatusMessage.clear();

        for (const std::string& error : errors)
        {
            m_policyStatusMessage += error + "\n";
        }

        return errors.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    class FirewallTests : public ::testing::Test
    {
    protected:
        MMI_JSON_STRING payload;
        int payloadSizeBytes;
        std::shared_ptr<FirewallModule<MockFirewall>> firewall;

        void SetUp() override
        {
            payload = nullptr;
            payloadSizeBytes = 0;
            firewall = std::make_shared<FirewallModule<MockFirewall>>(0);
        }

        void TearDown() override
        {
            FREE_MEMORY(payload);
            payloadSizeBytes = 0;
            firewall.reset();
        }

        void ClearPayload()
        {
            FREE_MEMORY(payload);
            payloadSizeBytes = 0;
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
        EXPECT_EQ(EINVAL, firewall->Get("invalid_component", Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), "invalid_object", &payload, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), nullptr, &payloadSizeBytes));
        EXPECT_EQ(EINVAL, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, nullptr));
    }

    TEST_F(FirewallTests, GetFingerprint)
    {
        std::string expectedFingerprint = "\"0\"";
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprint.length());
    }

    TEST_F(FirewallTests, GetState)
    {
        std::string expectedState = "\"disabled\"";
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedState.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedState.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedState.length());
    }

    TEST_F(FirewallTests, GetSetDefaultPolicies)
    {
        std::string policiesJson = "[{\"direction\": \"in\", \"action\": \"accept\"}, {\"direction\": \"out\", \"action\": \"drop\"}]";
        EXPECT_EQ(MMI_OK, firewall->Set(Firewall::m_firewallComponent.c_str(), Firewall::m_desiredDefaultPolicies.c_str(), (MMI_JSON_STRING)policiesJson.c_str(), policiesJson.length()));
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedDefaultPolicies.c_str(), &payload, &payloadSizeBytes));
        EXPECT_TRUE(JsonEq(policiesJson, std::string(payload, payloadSizeBytes)));
    }

    TEST_F(FirewallTests, SetDefaultPoliciesWithError)
    {
        std::string policiesJson = "[{\"direction\": \"in\", \"action\": \"accept\"}, {\"direction\": \"out\", \"action\": \"invalid\"}]";
        std::string initialFingerprint = "\"0\"";
        std::string expectedFingerprint = "\"1\""; // only one policy is valid
        std::string expectedStatus = "\"failure\"";

        // Check the initial fingerprint
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), initialFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, initialFingerprint.length());

        ClearPayload();

        EXPECT_EQ(EXIT_FAILURE, firewall->Set(Firewall::m_firewallComponent.c_str(), Firewall::m_desiredDefaultPolicies.c_str(), (MMI_JSON_STRING)policiesJson.c_str(), policiesJson.length()));

        // Check the new fingerprint
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprint.length());

        ClearPayload();

        // Check the status
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedConfigurationStatus.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedStatus.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedStatus.length());

        ClearPayload();

        // Check the status message
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedConfigurationStatusDetail.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STRNE(std::string(payload, payloadSizeBytes).c_str(), "\"\"");
        EXPECT_GT(payloadSizeBytes, strlen("\"\""));
    }

    std::string RuleJsonArray(std::vector<std::string> rules)
    {
        std::string json = "[";
        for (auto& rule : rules) {
            json += rule + ",";
        }
        json.pop_back();
        json += "]";
        return json;
    }

    std::string Rule(const std::string desiredState, const std::string& action, const std::string& direction, const std::string& protocol = "", const std::string& src = "", const std::string& dst = "", int srcPort = 0, int dstPort = 0)
    {
        std::string rule = "{\"desiredState\": \"" + desiredState + "\", \"action\": \"" + action + "\", \"direction\": \"" + direction + "\"";
        if (!protocol.empty())
        {
            rule += ", \"protocol\": \"" + protocol + "\"";
        }
        if (!src.empty())
        {
            rule += ", \"src\": \"" + src + "\"";
        }
        if (!dst.empty())
        {
            rule += ", \"dst\": \"" + dst + "\"";
        }
        if (srcPort != 0)
        {
            rule += ", \"srcPort\": " + std::to_string(srcPort);
        }
        if (dstPort != 0)
        {
            rule += ", \"dstPort\": " + std::to_string(dstPort);
        }
        rule += "}";
        return rule;
    }

    TEST_F(FirewallTests, SetRules)
    {
        std::vector<std::string> rules = {
            Rule("present", "accept", "in"),
            Rule("present", "accept", "in", "tcp"),
            Rule("present", "reject", "in", "udp", "0.0.0.0"),
            Rule("absent", "reject", "out", "icmp", "0.0.0.0", "0.0.0.0"),
            Rule("absent", "drop", "out", "any", "0.0.0.0", "0.0.0.0", 80),
            Rule("absent", "drop", "out", "any", "0.0.0.0", "0.0.0.0", 80, 443),
        };

        std::string initialFingerprint = "\"0\"";
        std::string expectedFingerprint = "\"" + std::to_string(rules.size()) + "\"";

        std::string json = RuleJsonArray(rules);

        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), initialFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, initialFingerprint.length());

        FREE_MEMORY(payload);
        payloadSizeBytes = 0;

        EXPECT_EQ(MMI_OK, firewall->Set(Firewall::m_firewallComponent.c_str(), Firewall::m_desiredRules.c_str(), (MMI_JSON_STRING)json.c_str(), json.length()));
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprint.length());
    }

    TEST_F(FirewallTests, SetRulesWithError)
    {
        std::vector<std::string> rules = {
            Rule("present", "accept", "in"),
            Rule("invalid", "invalid", "invalid"), // invalid rule
            Rule("absent", "accept", "out", "any", "0.0.0.0", "0.0.0.0", 80, 443),
        };

        std::string initialFingerprint = "\"0\"";
        std::string expectedFingerprint = "\"2\""; // only two rules are valid
        std::string expectedStatus = "\"failure\"";

        std::string json = RuleJsonArray(rules);

        // Check the initial fingerprint
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), initialFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, initialFingerprint.length());

        ClearPayload();

        EXPECT_EQ(EXIT_FAILURE, firewall->Set(Firewall::m_firewallComponent.c_str(), Firewall::m_desiredRules.c_str(), (MMI_JSON_STRING)json.c_str(), json.length()));

        // Check the new fingerprint
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedFingerprint.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), expectedFingerprint.c_str());
        EXPECT_EQ(payloadSizeBytes, expectedFingerprint.length());

        ClearPayload();

        // Check the status
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedConfigurationStatus.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STREQ(std::string(payload, payloadSizeBytes).c_str(), "\"failure\"");
        EXPECT_EQ(payloadSizeBytes, std::string("\"failure\"").length());

        ClearPayload();

        // Check the status message
        EXPECT_EQ(MMI_OK, firewall->Get(Firewall::m_firewallComponent.c_str(), Firewall::m_reportedConfigurationStatusDetail.c_str(), &payload, &payloadSizeBytes));
        EXPECT_STRNE(std::string(payload, payloadSizeBytes).c_str(), "\"\"");
        EXPECT_GT(payloadSizeBytes, strlen("\"\""));
    }

    std::string Policy(std::string direction, std::string action)
    {
        return "{\"direction\": \"" + direction + "\", \"action\": \"" + action + "\"}";
    }

    TEST_F(FirewallTests, ParsePolicy)
    {
        std::vector<std::string> policies = {
            Policy("in", "accept"),
            Policy("out", "accept"),
            Policy("in", "drop"),
            Policy("out", "drop")
        };

        for (auto policyJson : policies)
        {
            JSON_Value* value = json_parse_string(policyJson.c_str());
            JSON_Object* object = json_value_get_object(value);

            EXPECT_NE(nullptr, value);
            EXPECT_NE(nullptr, object);
            EXPECT_TRUE(json_object_has_value_of_type(object, "direction", JSONString));
            EXPECT_TRUE(json_object_has_value_of_type(object, "action", JSONString));

            IpTablesPolicy policy;
            policy.Parse(value);

            EXPECT_FALSE(policy.HasParseError()) << policyJson;
        }
    }

    TEST_F(FirewallTests, ParseRule)
    {
        std::vector<std::string> rules = {
            Rule("present", "accept", "in"),
            Rule("present", "accept", "in", "tcp"),
            Rule("present", "accept", "in", "udp", "0.0.0.0"),
            Rule("absent", "accept", "out", "icmp", "0.0.0.0", "0.0.0.0"),
            Rule("absent", "accept", "out", "any", "0.0.0.0", "0.0.0.0", 80),
            Rule("absent", "accept", "out", "any", "0.0.0.0", "0.0.0.0", 80, 443),
        };

        for (auto ruleJson : rules)
        {
            JSON_Value* value = json_parse_string(ruleJson.c_str());
            JSON_Object* object = json_value_get_object(value);

            EXPECT_NE(nullptr, value);
            EXPECT_NE(nullptr, object);
            EXPECT_TRUE(json_object_has_value_of_type(object, "desiredState", JSONString));
            EXPECT_TRUE(json_object_has_value_of_type(object, "action", JSONString));
            EXPECT_TRUE(json_object_has_value_of_type(object, "direction", JSONString));

            IpTablesRule rule;
            rule.Parse(value);

            EXPECT_FALSE(rule.HasParseError()) << ruleJson;
        }
    }

    TEST_F(FirewallTests, ParseRuleWithError)
    {
        std::vector<std::string> invalidRules = {
            "{\"action\": \"accept\", \"direction\": \"in\"}",
            "{\"desiredState\": 123, \"action\": \"accept\", \"direction\": \"in\"}",
            "{\"desiredState\": \"invalid\", \"action\": \"accept\", \"direction\": \"in\"}",
            "{\"desiredState\": \"present\", \"direction\": \"in\"}",
            "{\"desiredState\": \"present\", \"action\": 123, \"direction\": \"in\"}",
            "{\"desiredState\": \"present\", \"action\": \"invalid\", \"direction\": \"in\"}",
            "{\"desiredState\": \"present\", \"action\": \"accept\"}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": 123}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"invalid\"}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"protocol\": 123}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"protocol\": \"invalid\"}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"sourceAddress\": 123}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"destinationAddress\": 123}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"sourcePort\": \"123\"}",
            "{\"desiredState\": \"present\", \"action\": \"accept\", \"direction\": \"in\", \"destinationPort\": \"123\"}",
        };

        for (auto ruleJson : invalidRules)
        {
            JSON_Value* value = json_parse_string(ruleJson.c_str());
            JSON_Object* object = json_value_get_object(value);

            EXPECT_NE(nullptr, value);
            EXPECT_NE(nullptr, object);

            IpTablesRule rule;
            rule.Parse(value);

            EXPECT_TRUE(rule.HasParseError()) << ruleJson;
        }
    }

    TEST_F(FirewallTests, ParsePolicyWithError)
    {
        std::vector<std::string> invalidPolicies = {
            "{\"action\": \"accept\"}",
            "{\"direction\": 123, \"action\": \"accept\"}",
            "{\"direction\": \"invalid\", \"action\": \"accept\"}",
            "{\"direction\": \"in\"}",
            "{\"direction\": \"in\", \"action\": 123}",
            "{\"direction\": \"in\", \"action\": \"invalid\"}",
        };

        for (auto policyJson : invalidPolicies)
        {
            JSON_Value* value = json_parse_string(policyJson.c_str());
            JSON_Object* object = json_value_get_object(value);

            EXPECT_NE(nullptr, value);
            EXPECT_NE(nullptr, object);

            IpTablesPolicy policy;
            policy.Parse(value);

            EXPECT_TRUE(policy.HasParseError()) << policyJson;
        }
    }
} // namespace tests