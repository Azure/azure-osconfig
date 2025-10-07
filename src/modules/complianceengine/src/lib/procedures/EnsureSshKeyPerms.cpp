// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Evaluator.h>
#include <FilePermissionsHelpers.h>
#include <FileTreeWalk.h>
#include <sstream>

// TODO(bob): delete
#include <iostream>
namespace ComplianceEngine
{
namespace
{
// Helper routine shared by audit and remediation for SSH key permission checks.
// When isRemediation == true it will remediate permissions/ownership, otherwise only audit.
Result<Status> EnsureSshKeyPermsHelper(bool isPublic, bool isRemediation, IndicatorsTree& indicators, ContextInterface& context)
{
    (void)context; // log handle not needed currently

    const std::string baseDir = context.GetSpecialFilePath("/etc/ssh");

    // Prepare static args map passed to existing file permission helpers.
    // owner=root, group=root|ssh_keys
    // Public keys: permission mask 0133 (i.e. we remove group/world write/execute and others read?) -> apply as mask (bits to be cleared)
    // Private keys: permissions 0600 (mask 0137 to enforce?) - requirement given: permissions 0137 (interpretation: mask for private keys?)
    // From user instructions: public key uses mask 0133; private key uses permissions 0137.
    // For consistency with existing helpers which treat 'permissions' as bits that MUST be set (perms != (st_mode & perms) -> NonCompliant) and 'mask' as bits that MUST NOT be set (statbuf.st_mode & mask) -> NonCompliant, we:
    //  - pass mask for public keys (bits forbidden)
    //  - pass mask for private keys too if semantics is to forbid bits (private keys should typically be 0600). However user explicitly said 'permissions 0137'. That suggests we want to CLEAR these bits. In helper logic supplying 'permissions' would require those bits to be present which is opposite. Thus we pass it as 'mask'.
    // So: treat provided numbers as masks of forbidden bits.

    auto processFile = [&](const std::string& dir, const std::string& name, const struct stat& sb) -> Result<Status> {
        if (!S_ISREG(sb.st_mode))
        {
            return Status::Compliant; // ignore non-regular
        }
        std::string fullPath = dir + "/" + name;

        auto content = context.GetFileContents(fullPath);
        if (!content.HasValue())
        {
            return Status::Compliant; // ignore unreadable
        }
        std::istringstream iss(content.Value());
        std::string firstLine;
        if (!std::getline(iss, firstLine))
        {
            return Status::Compliant;
        }

        bool isKey = false;
        if (isPublic)
        {
            // Public key line starts with one of these prefixes
            static const char* prefixes[] = {"ssh-dss", "ssh-rsa", "ecdsa-sha2-", "ssh-ed25519"};
            for (auto p : prefixes)
            {
                if (firstLine.rfind(p, 0) == 0)
                {
                    isKey = true;
                    break;
                }
            }
        }
        else
        {
            static const char* privPrefixes[] = {
                "SSH PRIVATE KEY", "-----BEGIN OPENSSH PRIVATE KEY-----", "-----BEGIN PRIVATE KEY-----", "-----BEGIN ENCRYPTED PRIVATE KEY"};
            for (auto p : privPrefixes)
            {
                std::cerr << "[" << __func__ << ":" << __LINE__ << "] Checking prefix: '" << p << "' against line: '" << firstLine << "'" << std::endl;
                if (firstLine.rfind(p, 0) == 0)
                {
                    isKey = true;
                    std::cerr << "[" << __func__ << ":" << __LINE__ << "] Matched private key prefix" << std::endl;
                    break;
                }
            }
        }

        if (!isKey)
        {
            std::cerr << "[" << __func__ << ":" << __LINE__ << "] Not a key" << std::endl;
            return Status::Compliant;
        }

        std::map<std::string, std::string> args;
        args["owner"] = "root";
        args["group"] = "root|ssh_keys";
        if (isPublic)
        {
            args["mask"] = "0133"; // forbidden bits
        }
        else
        {
            args["mask"] = "0137"; // private key stricter forbidden bits
        }

        Result<Status> result = isRemediation ? RemediateEnsureFilePermissionsHelper(fullPath, args, indicators, context) :
                                                AuditEnsureFilePermissionsHelper(fullPath, args, indicators, context);
        if (!result.HasValue())
        {
            return result.Error();
        }
        return result.Value();
    };

    auto walkResult = FileTreeWalk(baseDir, processFile, BreakOnNonCompliant::True, context);
    if (!walkResult.HasValue())
    {
        return walkResult.Error();
    }
    return walkResult.Value();
}
} // anonymous namespace

AUDIT_FN(EnsureSshKeyPerms, "type:Key type - public or private:M:^(public|private)$")
{
    auto it = args.find("type");
    if (it == args.end())
    {
        return Error("Missing required parameter 'type'", EINVAL);
    }
    bool isPublic = (it->second == "public");
    return EnsureSshKeyPermsHelper(isPublic, false, indicators, context);
}

REMEDIATE_FN(EnsureSshKeyPerms, "type:Key type - public or private:M:^(public|private)$")
{
    auto it = args.find("type");
    if (it == args.end())
    {
        return Error("Missing required parameter 'type'", EINVAL);
    }
    bool isPublic = (it->second == "public");
    return EnsureSshKeyPermsHelper(isPublic, true, indicators, context);
}

} // namespace ComplianceEngine
