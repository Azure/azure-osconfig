// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Indicators.h"
#include "LuaEvaluator.h"
#include "MockContext.h"
#include "Result.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using ComplianceEngine::Action;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::LuaEvaluator;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class LuaProceduresTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mIndicators.Push("LuaProceduresTest");
        char tmpl[] = "/tmp/lua_proc_testXXXXXX";
        char* dir = mkdtemp(tmpl);
        ASSERT_NE(dir, nullptr) << "mkdtemp failed: " << strerror(errno);
        mTempRoot = dir;
        // helper lambdas
        auto mkdirp = [](const std::string& p) {
            if (mkdir(p.c_str(), 0700) != 0 && errno != EEXIST)
            {
                throw std::runtime_error(std::string("mkdir failed: ") + p + ": " + strerror(errno));
            }
        };
        // Layout:
        // root/
        //   a.txt
        //   b.log
        //   sub1/ (contains c.conf and nested/ d.txt )
        //   sub2/ (contains ignore.tmp)
        std::ofstream(mTempRoot + "/a.txt") << "A";
        std::ofstream(mTempRoot + "/b.log") << "B";
        mkdirp(mTempRoot + "/sub1");
        mkdirp(mTempRoot + "/sub1/nested");
        std::ofstream(mTempRoot + "/sub1/c.conf") << "C";
        std::ofstream(mTempRoot + "/sub1/nested/d.txt") << "D";
        mkdirp(mTempRoot + "/sub2");
        std::ofstream(mTempRoot + "/sub2/ignore.tmp") << "I";
    }

    void TearDown() override
    {
        // Best-effort recursive removal (only files/dirs we created)
        auto unlinkIf = [](const std::string& p) { unlink(p.c_str()); };
        unlinkIf(mTempRoot + "/a.txt");
        unlinkIf(mTempRoot + "/b.log");
        unlinkIf(mTempRoot + "/sub1/c.conf");
        unlinkIf(mTempRoot + "/sub1/nested/d.txt");
        unlinkIf(mTempRoot + "/sub2/ignore.tmp");
        rmdir((mTempRoot + "/sub1/nested").c_str());
        rmdir((mTempRoot + "/sub1").c_str());
        rmdir((mTempRoot + "/sub2").c_str());
        rmdir(mTempRoot.c_str());
    }

    std::string MakeScript(const std::string& path, const std::string& pattern, bool recursive)
    {
        std::string patArg = pattern.empty() ? "nil" : ("'" + pattern + "'");
        std::string recArg = recursive ? "true" : "false";
        return "local t = {}\n"
               "for f in ce.ListDirectory('" +
               path + "', " + patArg + ", " + recArg +
               ") do t[#t+1]=f end\n"
               "table.sort(t)\n"
               "local r=''; for i,v in ipairs(t) do r = r .. v .. ';' end\n"
               "return true, r";
    }

    IndicatorsTree mIndicators;
    MockContext mContext;
    std::string mTempRoot;

    // Helpers for permission-based iterator tests
    static std::string MakePermsScript(const std::string& hasExpr, const std::string& noExpr)
    {
        std::string script;
        script += "local t={} ";
        script += "for p in ce.GetFilesystemEntriesWithPerms(\"" + hasExpr + "\", \"" + noExpr + "\") do ";
        script += "t[#t+1]=p end table.sort(t) return true, table.concat(t,';')";
        return script;
    }

    static std::string MakeSystemdConfigTestScript(const std::string& filename)
    {
        return R"(local t = ce.GetSystemdConfig(')" + filename + R"(')
-- print("A value: " .. t["A"]["value"])
-- print("B value: " .. t["B"]["value"])
-- print("A defined in: " .. t["A"]["src"])
-- print("B defined in: " .. t["B"]["src"])
if t["A"]["value"] ~= "foo" then return false, "A value mismatch" end
if t["B"]["value"] ~= "bar" then return false, "B value mismatch" end
if not string.find(t["A"]["src"], ".conf") then return false, "A src mismatch" end
if not string.find(t["B"]["src"], ".conf") then return false, "B src mismatch" end
return true
)";
    }
};

TEST_F(LuaProceduresTest, GetFilesystemEntriesWithPermsBasic)
{
    std::string scanRoot = mContext.GetFilesystemScannerRoot();
    std::string execPath = scanRoot + "/perm_exec.sh";
    std::string readPath = scanRoot + "/perm_read.txt";
    std::ofstream(execPath) << "#!/bin/sh\n";
    std::ofstream(readPath) << "data";
    ::chmod(execPath.c_str(), 0755);
    ::chmod(readPath.c_str(), 0644);

    LuaEvaluator evaluator;

    auto runScript = [&](const std::string& luaScript) {
        auto res = evaluator.Evaluate(luaScript.c_str(), mIndicators, mContext, Action::Audit);
        ASSERT_TRUE(res.HasValue());
        EXPECT_EQ(res.Value(), Status::Compliant);
    };

    // First script: require owner execute, exclude group write
    runScript(MakePermsScript("00001", "00020"));
    // Second script: exclude owner execute (select non-exec file)
    runScript(MakePermsScript("0", "00001"));
}

TEST_F(LuaProceduresTest, ListDirectory_NonRecursiveAllFiles)
{
    LuaEvaluator evaluator;
    auto script = MakeScript(mTempRoot, "", false);
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    // Expect only top-level files a.txt and b.log (no directories, no recursion)
    auto& msg = mIndicators.GetRootNode()->indicators.back().message;
    EXPECT_NE(msg.find("a.txt;"), std::string::npos);
    EXPECT_NE(msg.find("b.log;"), std::string::npos);
    EXPECT_EQ(msg.find("c.conf"), std::string::npos); // not recursive
}

TEST_F(LuaProceduresTest, ListDirectory_PatternFilter)
{
    LuaEvaluator evaluator;
    auto script = MakeScript(mTempRoot, "*.txt", false);
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    auto& msg = mIndicators.GetRootNode()->indicators.back().message;
    EXPECT_NE(msg.find("a.txt;"), std::string::npos);
    EXPECT_EQ(msg.find("b.log"), std::string::npos); // filtered
}

TEST_F(LuaProceduresTest, ListDirectory_RecursivePattern)
{
    LuaEvaluator evaluator;
    auto script = MakeScript(mTempRoot, "*.txt", true);
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    auto& msg = mIndicators.GetRootNode()->indicators.back().message;
    EXPECT_NE(msg.find("a.txt;"), std::string::npos);
    EXPECT_NE(msg.find("sub1/nested/d.txt;"), std::string::npos);
    EXPECT_EQ(msg.find("c.conf"), std::string::npos); // pattern mismatch
}

TEST_F(LuaProceduresTest, ListDirectory_RecursiveAll)
{
    LuaEvaluator evaluator;
    auto script = MakeScript(mTempRoot, "", true);
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    auto& msg = mIndicators.GetRootNode()->indicators.back().message;
    EXPECT_NE(msg.find("a.txt;"), std::string::npos);
    EXPECT_NE(msg.find("b.log;"), std::string::npos);
    EXPECT_NE(msg.find("sub1/c.conf;"), std::string::npos);
    EXPECT_NE(msg.find("sub1/nested/d.txt;"), std::string::npos);
    EXPECT_NE(msg.find("sub2/ignore.tmp;"), std::string::npos);
}

TEST_F(LuaProceduresTest, ListDirectory_DirectoriesNotReturned)
{
    LuaEvaluator evaluator;
    // script attempts to detect if any directory name is yielded
    std::string script = "local t = {}; for f in ce.ListDirectory('" + mTempRoot +
                         "', nil, true) do t[#t+1]=f end "
                         "for i,v in ipairs(t) do if v=='sub1' or v=='sub2' or v=='sub1/nested' then return false, 'Directory yielded: '..v end end "
                         "return true, table.concat(t,';')";
    auto result = evaluator.Evaluate(script, mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LuaProceduresTest, GetSystemdConfig_1)
{
    LuaEvaluator evaluator;
    const auto filename = mTempRoot + "/sub1/c.conf";
    const auto cmd = "/usr/bin/systemd-analyze cat-config " + filename;
    const std::string output = "# " + filename + "\nA=foo\nB=bar";
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(cmd))).WillOnce(::testing::Return(Result<std::string>(output)));
    const auto result = evaluator.Evaluate(MakeSystemdConfigTestScript(filename), mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LuaProceduresTest, GetSystemdConfig_2)
{
    LuaEvaluator evaluator;
    const auto filename = mTempRoot + "/sub1/c.conf";
    const auto cmd = "/usr/bin/systemd-analyze cat-config " + filename;
    const std::string output = "# " + filename + "\nA=foo";
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(cmd))).WillOnce(::testing::Return(Result<std::string>(output)));
    const auto result = evaluator.Evaluate(MakeSystemdConfigTestScript(filename), mIndicators, mContext, Action::Audit);
    // The script expects the B key, it bails out
    ASSERT_FALSE(result.HasValue());
}

TEST_F(LuaProceduresTest, GetSystemdConfig_3)
{
    LuaEvaluator evaluator;
    const auto filename = mTempRoot + "/sub1/c.conf";
    const auto cmd = "/usr/bin/systemd-analyze cat-config " + filename;
    const std::string output = "# " + filename + "\nA=foo\nB=baz";
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(cmd))).WillOnce(::testing::Return(Result<std::string>(output)));
    const auto result = evaluator.Evaluate(MakeSystemdConfigTestScript(filename), mIndicators, mContext, Action::Audit);
    ASSERT_TRUE(result.HasValue());
    // The script expects the B value is "bar"
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}
