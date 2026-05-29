#include "Mof.hpp"

#include <gtest/gtest.h>

#include <sstream>

using ComplianceEngine::MOF::Resource;

TEST(MofTest, ParseSingleEntry_UnescapesStringValues)
{
    std::istringstream stream(
        "instance of OsConfigResource as $OsConfigResource0ref\n"
        "{\n"
        "    ResourceID = \"Test \\\"quoted\\\" rule\";\n"
        "    PayloadKey = \"/cis/almalinux/8.*/v3.0.0/4/2/4\";\n"
        "    ProcedureObjectValue = \"eyJhdWRpdCI6e319\";\n"
        "    InitObjectName = \"initEnsureSshdAccessIsConfigured\";\n"
        "    ReportedObjectName = \"auditEnsureSshdAccessIsConfigured\";\n"
        "    DesiredObjectValue = \"{\\\"allowusersAllowgroupsDenyusersDenygroupsValue\\\":\\\"\\\\\\\\S+\\\"}\";\n"
        "};\n");

    auto result = Resource::ParseSingleEntry(stream);

    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_EQ(result.Value().resourceID, "Test \"quoted\" rule");
    EXPECT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "{\"allowusersAllowgroupsDenyusersDenygroupsValue\":\"\\\\S+\"}");
}
