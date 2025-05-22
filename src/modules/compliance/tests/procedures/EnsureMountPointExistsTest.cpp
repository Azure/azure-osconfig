#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

using compliance::AuditEnsureMountPointExists;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;
using ::testing::Return;

class EnsureMountPointExistsTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;
    const std::string findmntOutput =
        "/                                                               /dev/sdc                                                                  "
        "ext4          rw,relatime,discard,errors=remount-ro,data=ordered\n"
        "/init                                                           rootfs[/init]                                                             "
        "rootfs        ro,size=16418640k,nr_inodes=4104660\n"
        "/dev                                                            none                                                                      "
        "devtmpfs      rw,nosuid,relatime,size=16418640k,nr_inodes=4104660,mode=755\n"
        "/sys                                                            sysfs                                                                     "
        "sysfs         rw,nosuid,nodev,noexec,noatime\n"
        "/proc                                                           proc                                                                      "
        "proc          rw,nosuid,nodev,noexec,noatime\n"
        "/dev/pts                                                        devpts                                                                    "
        "devpts        rw,nosuid,noexec,noatime,gid=5,mode=620,ptmxmode=000\n"
        "/run                                                            none                                                                      "
        "tmpfs         rw,nosuid,nodev,mode=755\n"
        "/run/lock                                                       none                                                                      "
        "tmpfs         rw,nosuid,nodev,noexec,noatime\n"
        "/run/shm                                                        none                                                                      "
        "tmpfs         rw,nosuid,nodev,noatime\n"
        "/dev/shm                                                        none                                                                      "
        "tmpfs         rw,nosuid,nodev,noatime\n"
        "/run/user                                                       none                                                                      "
        "tmpfs         rw,nosuid,nodev,noexec,noatime,mode=755\n"
        "/proc/sys/fs/binfmt_misc                                        binfmt_misc                                                               "
        "binfmt_misc   rw,relatime\n";

    void SetUp() override
    {
        indicators.Push("EnsureMountPointExists");
    }
};

TEST_F(EnsureMountPointExistsTest, AuditNoArgument)
{
    std::map<std::string, std::string> args;

    auto result = AuditEnsureMountPointExists(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No mount point provided");
}

TEST_F(EnsureMountPointExistsTest, AuditMountPointExists)
{
    EXPECT_CALL(mContext, ExecuteCommand("findmnt -knl")).WillOnce(Return(Result<std::string>(findmntOutput)));

    std::map<std::string, std::string> args;
    args["mountPoint"] = "/dev/shm";

    auto result = AuditEnsureMountPointExists(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureMountPointExistsTest, AuditMountPointDoesNotExist)
{
    EXPECT_CALL(mContext, ExecuteCommand("findmnt -knl")).WillOnce(Return(Result<std::string>(findmntOutput)));

    std::map<std::string, std::string> args;
    args["mountPoint"] = "/tmp";

    auto result = AuditEnsureMountPointExists(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureMountPointExistsTest, AuditFindmntCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand("findmnt -knl")).WillOnce(Return(Result<std::string>(Error("Failed to execute findmnt command", -1))));

    std::map<std::string, std::string> args;
    args["mountPoint"] = "/mnt/data";

    auto result = AuditEnsureMountPointExists(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_NE(result.Error().message.find("Failed to execute findmnt command"), std::string::npos);
}
