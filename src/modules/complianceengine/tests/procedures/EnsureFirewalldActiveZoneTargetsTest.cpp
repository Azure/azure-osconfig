// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureFirewalldActiveZoneTargets.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ComplianceEngine::AuditEnsureFirewalldActiveZoneTargets;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class EnsureFirewalldActiveZoneTargetsTest : public ::testing::Test
{
protected:
    MockContext mockContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("EnsureFirewalldActiveZoneTargets");
    }
};

// ===== firewalld service not active =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, FirewalldNotActive_CommandFails_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Error("inactive", 3)));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, FirewalldNotActive_OutputInactive_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("inactive")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// ===== firewall-cmd --get-active-zones failures =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, GetActiveZonesFails_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Error("command failed", 1)));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, NoActiveZones_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// ===== Single zone, compliant scenarios =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_DefaultTarget_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n"
                                             "  target: default\n"
                                             "  icmp-block-inversion: no\n"
                                             "  interfaces: eth0\n"
                                             "  sources:\n"
                                             "  services: cockpit dhcpv6-client ssh\n"
                                             "  ports:\n"
                                             "  protocols:\n"
                                             "  forward: no\n"
                                             "  masquerade: no\n"
                                             "  forward-ports:\n"
                                             "  source-ports:\n"
                                             "  icmp-blocks:\n"
                                             "  rich rules:\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_DropTarget_MatchesPermanent_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("drop\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=drop --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=drop --get-target")).WillOnce(Return(Result<std::string>("DROP")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=drop"))
        .WillOnce(Return(Result<std::string>("drop (active)\n  target: DROP\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_RejectTarget_MatchesPermanent_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("%%REJECT%%")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: %%REJECT%%\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== Single zone, non-compliant scenarios =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_TargetAccept_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("ACCEPT")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: ACCEPT\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_EmptyTarget_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, SingleZone_TargetMismatch_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: DROP\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// ===== Command failures within zone processing =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, ListInterfacesFails_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Error("command failed", 1)));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, GetPermanentTargetFails_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Error("command failed", 1)));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, ListAllFails_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public")).WillOnce(Return(Error("command failed", 1)));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

// ===== Interface skipping (loopback / virtual bridge) =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, LoopbackInterface_Skipped_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("trusted\n  interfaces: lo\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=trusted --list-interfaces")).WillOnce(Return(Result<std::string>("lo")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, VirtualBridgeInterface_Skipped_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("trusted\n  interfaces: virbr0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=trusted --list-interfaces")).WillOnce(Return(Result<std::string>("virbr0")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, MixedInterfacesIncludingNonLoopback_NotSkipped)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("public\n  interfaces: lo eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("lo eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: lo eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== Multiple zones =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, MultipleZones_AllCompliant_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\ninternal\n  interfaces: eth1\n")));

    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: eth0\n")));

    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=internal --list-interfaces")).WillOnce(Return(Result<std::string>("eth1")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=internal --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=internal"))
        .WillOnce(Return(Result<std::string>("internal (active)\n  target: default\n  interfaces: eth1\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, MultipleZones_SecondNonCompliant_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\ninternal\n  interfaces: eth1\n")));

    // Public zone - compliant
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: eth0\n")));

    // Internal zone - non-compliant (target is ACCEPT)
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=internal --list-interfaces")).WillOnce(Return(Result<std::string>("eth1")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=internal --get-target")).WillOnce(Return(Result<std::string>("ACCEPT")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=internal"))
        .WillOnce(Return(Result<std::string>("internal (active)\n  target: ACCEPT\n  interfaces: eth1\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, MultipleZones_LoopbackSkipped_OtherCompliant_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("trusted\n  interfaces: lo\npublic\n  interfaces: eth0\n")));

    // Trusted zone - loopback, should be skipped
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=trusted --list-interfaces")).WillOnce(Return(Result<std::string>("lo")));

    // Public zone - compliant
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== Case-insensitive comparison =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, CaseInsensitive_AcceptLowercase_ReturnsNonCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("accept")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: accept\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFirewalldActiveZoneTargetsTest, CaseInsensitive_TargetMatchesPermanent_ReturnsCompliant)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones")).WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("Default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

// ===== Zone name parsing edge cases =====

TEST_F(EnsureFirewalldActiveZoneTargetsTest, ZonesOutputWithSources_ParsedCorrectly)
{
    EXPECT_CALL(mockContext, ExecuteCommand("systemctl is-active firewalld.service")).WillOnce(Return(Result<std::string>("active")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --get-active-zones"))
        .WillOnce(Return(Result<std::string>("public\n  interfaces: eth0\n  sources: 10.0.0.0/24\n")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --zone=public --list-interfaces")).WillOnce(Return(Result<std::string>("eth0")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --permanent --zone=public --get-target")).WillOnce(Return(Result<std::string>("default")));
    EXPECT_CALL(mockContext, ExecuteCommand("firewall-cmd --list-all --zone=public"))
        .WillOnce(Return(Result<std::string>("public (active)\n  target: default\n  interfaces: eth0\n")));

    auto result = AuditEnsureFirewalldActiveZoneTargets(indicators, mockContext);

    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}
