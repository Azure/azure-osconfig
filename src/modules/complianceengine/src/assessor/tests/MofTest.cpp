// Smoke tests for the MOF parser library. Verifies the lib links and that
// ParseSingleEntry accepts a well-formed MOF body. The MOF stream iterator
// (ParseAll), bounded-line reader, and adversarial-input tests land in the
// MOF parser-rework PR along with their own tests.

#include <Mof.hpp>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using ComplianceEngine::MOF::Resource;

namespace
{
constexpr const char* kValidBody = R"(    ResourceID = "Some Rule";
    PayloadKey = "/cis/ubuntu/22.04/v2.0.0/1/1/1";
    ProcedureObjectName = "procedureMyRule";
    ProcedureObjectValue = "base64data==";
    InitObjectName = "initMyRule";
    ReportedObjectName = "auditMyRule";
    DesiredObjectValue = "mask=0600";
};
)";
} // namespace

TEST(MofSmokeTest, ParseSingleEntryHappyPath)
{
    std::istringstream s(kValidBody);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    EXPECT_EQ(result.Value().resourceID, "Some Rule");
    EXPECT_EQ(result.Value().ruleName, "MyRule");
    EXPECT_EQ(result.Value().procedure, "base64data==");
    EXPECT_TRUE(result.Value().hasInitAudit);
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "mask=0600");
    EXPECT_EQ(result.Value().benchmarkInfo.section, "1.1.1");
}

TEST(MofSmokeTest, JsonEscapedDesiredObjectValue)
{
    // Production MOFs use JSON for DesiredObjectValue with MOF-escaped inner
    // quotes. Verifies pr1's GetValue escape handling is preserved through
    // the library extraction.
    const std::string body =
        "    ResourceID = \"Some Rule\";\n"
        "    PayloadKey = \"/cis/rhel/8/v4.0.0/1/2/3\";\n"
        "    ProcedureObjectName = \"procedureMyRule\";\n"
        "    ProcedureObjectValue = \"base64data==\";\n"
        "    InitObjectName = \"initMyRule\";\n"
        "    ReportedObjectName = \"auditMyRule\";\n"
        "    DesiredObjectValue = \"{\\\"mountPoint\\\":\\\"/tmp\\\"}\";\n"
        "};\n";
    std::istringstream s(body);
    auto result = Resource::ParseSingleEntry(s);
    ASSERT_TRUE(result.HasValue()) << result.Error().message;
    ASSERT_TRUE(result.Value().payload.HasValue());
    EXPECT_EQ(result.Value().payload.Value(), "{\"mountPoint\":\"/tmp\"}");
}
