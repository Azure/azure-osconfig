// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CopyFailMitigation.h>

#include <Logging.h>
#include <StringTools.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace ComplianceEngine
{
namespace
{
constexpr const char* cProgramName = "copyfail_afalg";
constexpr const char* cConfigPath = "/etc/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm.allow";
constexpr const char* cStateDirectory = "/var/lib/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm";
constexpr const char* cBpfFsDirectory = "/sys/fs/bpf";
constexpr const char* cLoaderScriptPath = "/usr/lib/osconfig/runtime-threat-mitigations/copyfail-afalg-lsm-load.sh";
constexpr const char* cSystemdUnitPath = "/etc/systemd/system/osconfig-copyfail-afalg-lsm.service";
constexpr std::size_t cMaxAllowedExecutablePaths = 16;
constexpr std::size_t cMaxBpfPathLength = 256;

#include "CopyFailMitigationBpfObject.inc"

std::string GetParentDirectory(const std::string& path)
{
    const auto pos = path.find_last_of('/');
    if (std::string::npos == pos)
    {
        return ".";
    }
    if (0 == pos)
    {
        return "/";
    }
    return path.substr(0, pos);
}

Result<bool> EnsureDirectory(const std::string& path, const mode_t mode)
{
    if (path.empty())
    {
        return Error("Directory path is empty", EINVAL);
    }

    std::string current;
    std::size_t pos = 0;
    if ('/' == path[0])
    {
        current = "/";
        pos = 1;
    }

    while (pos <= path.size())
    {
        const auto next = path.find('/', pos);
        const auto component = path.substr(pos, (std::string::npos == next) ? std::string::npos : next - pos);
        if (!component.empty())
        {
            if ((!current.empty()) && (current.back() != '/'))
            {
                current += "/";
            }
            current += component;

            if ((0 != mkdir(current.c_str(), mode)) && (EEXIST != errno))
            {
                const int status = errno;
                return Error("Failed to create directory '" + current + "': " + strerror(status), status);
            }

            struct stat statbuf;
            if (0 != stat(current.c_str(), &statbuf))
            {
                const int status = errno;
                return Error("Failed to stat directory '" + current + "': " + strerror(status), status);
            }
            if (!S_ISDIR(statbuf.st_mode))
            {
                return Error("Path '" + current + "' exists but is not a directory", ENOTDIR);
            }
        }

        if (std::string::npos == next)
        {
            break;
        }
        pos = next + 1;
    }

    if (0 != chmod(path.c_str(), mode))
    {
        const int status = errno;
        return Error("Failed to chmod directory '" + path + "': " + strerror(status), status);
    }

    return true;
}

Result<bool> WriteTextFile(const std::string& path, const std::string& content, const mode_t mode)
{
    auto directory = EnsureDirectory(GetParentDirectory(path), 0755);
    if (!directory.HasValue())
    {
        return directory.Error();
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file)
    {
        return Error("Failed to open file '" + path + "' for writing", errno);
    }

    file << content;
    if (!file)
    {
        return Error("Failed to write file '" + path + "'", EIO);
    }

    file.close();
    if (!file)
    {
        return Error("Failed to close file '" + path + "'", EIO);
    }

    if (0 != chmod(path.c_str(), mode))
    {
        const int status = errno;
        return Error("Failed to chmod file '" + path + "': " + strerror(status), status);
    }

    return true;
}

Result<bool> WriteBinaryFile(const std::string& path, const unsigned char* content, const unsigned int contentLength, const mode_t mode)
{
    auto directory = EnsureDirectory(GetParentDirectory(path), 0755);
    if (!directory.HasValue())
    {
        return directory.Error();
    }

    std::ofstream file(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file)
    {
        return Error("Failed to open file '" + path + "' for writing", errno);
    }

    file.write(reinterpret_cast<const char*>(content), contentLength);
    if (!file)
    {
        return Error("Failed to write file '" + path + "'", EIO);
    }

    file.close();
    if (!file)
    {
        return Error("Failed to close file '" + path + "'", EIO);
    }

    if (0 != chmod(path.c_str(), mode))
    {
        const int status = errno;
        return Error("Failed to chmod file '" + path + "': " + strerror(status), status);
    }

    return true;
}

Result<std::vector<std::string>> ParseAllowedExecutablePaths(const CopyFailMitigationParams& params)
{
    const auto rawPaths = TrimWhiteSpaces(params.allowedExecutablePaths.ValueOr(""));
    std::vector<std::string> paths;
    if (rawPaths.empty())
    {
        return paths;
    }

    std::size_t pos = 0;
    while (pos <= rawPaths.size())
    {
        const auto next = rawPaths.find('|', pos);
        auto path = TrimWhiteSpaces(rawPaths.substr(pos, (std::string::npos == next) ? std::string::npos : next - pos));
        if (path.empty())
        {
            return Error("Allowed executable path entry is empty", EINVAL);
        }
        if ('/' != path[0])
        {
            return Error("Allowed executable path '" + path + "' is not absolute", EINVAL);
        }
        if ((std::string::npos != path.find('\n')) || (std::string::npos != path.find('\r')))
        {
            return Error("Allowed executable path contains a newline", EINVAL);
        }
        if (path.size() >= cMaxBpfPathLength)
        {
            return Error("Allowed executable path '" + path + "' exceeds the eBPF path length limit", EINVAL);
        }
        paths.push_back(path);
        if (paths.size() > cMaxAllowedExecutablePaths)
        {
            return Error("Too many allowed executable paths", EINVAL);
        }
        if (std::string::npos == next)
        {
            break;
        }
        pos = next + 1;
    }

    return paths;
}

std::string BuildAllowListConfig(const std::vector<std::string>& paths)
{
    std::ostringstream config;
    for (const auto& path : paths)
    {
        config << path << "\n";
    }
    return config.str();
}

std::string HexByte(const unsigned int value)
{
    std::ostringstream hex;
    hex << std::hex << std::setw(2) << std::setfill('0') << (value & 0xff);
    return hex.str();
}

std::string PathValueHex(const std::string& path)
{
    std::ostringstream hex;
    for (std::size_t i = 0; i < cMaxBpfPathLength; ++i)
    {
        if (0 != i)
        {
            hex << " ";
        }
        const unsigned int value = (i < path.size()) ? static_cast<unsigned char>(path[i]) : 0;
        hex << HexByte(value);
    }
    return hex.str();
}

std::string BuildLoaderScript(const std::string& stateDirectory, const std::string& bpfFsDirectory)
{
    const std::string pinDirectory = bpfFsDirectory + "/osconfig";
    std::ostringstream script;
    script << "#!/bin/sh\n";
    script << "set -eu\n";
    script << "STATE_DIR=\"" << EscapeForShell(stateDirectory) << "\"\n";
    script << "BPF_FS=\"" << EscapeForShell(bpfFsDirectory) << "\"\n";
    script << "PIN_DIR=\"" << EscapeForShell(pinDirectory) << "\"\n";
    script << "MAP_DIR=\"${PIN_DIR}/" << cProgramName << "_maps\"\n";
    script << "OBJ=\"${STATE_DIR}/copyfail-afalg-lsm.bpf.o\"\n";
    script << "find_bpftool() {\n";
    script << "    if command -v bpftool >/dev/null 2>&1 && bpftool version >/dev/null 2>&1; then command -v bpftool; return 0; fi\n";
    script << "    for candidate in /usr/lib/linux-tools/*/bpftool /usr/sbin/bpftool /usr/bin/bpftool; do\n";
    script << "        if [ -x \"${candidate}\" ] && \"${candidate}\" version >/dev/null 2>&1; then echo \"${candidate}\"; return 0; fi\n";
    script << "    done\n";
    script << "    echo 'bpftool is required' >&2\n";
    script << "    return 1\n";
    script << "}\n";
    script << "BPFTOOL=$(find_bpftool)\n";
    script << "mkdir -p \"${STATE_DIR}\" \"${BPF_FS}\"\n";
    script << "mountpoint -q \"${BPF_FS}\" || mount -t bpf bpf \"${BPF_FS}\"\n";
    script << "rm -rf \"${PIN_DIR}\"\n";
    script << "mkdir -p \"${PIN_DIR}\" \"${MAP_DIR}\"\n";
    script << "\"${BPFTOOL}\" prog loadall \"${OBJ}\" \"${PIN_DIR}\" pinmaps \"${MAP_DIR}\" autoattach\n";
    return script.str();
}

std::string BuildMapUpdateScript(const std::vector<std::string>& paths)
{
    std::ostringstream script;
    for (std::size_t i = 0; i < paths.size(); ++i)
    {
        script << "\"${BPFTOOL}\" map update pinned \"${MAP_DIR}/allowed_paths\" key hex " << PathValueHex(paths[i]) << " value hex 01 any\n";
    }
    script << "\"${BPFTOOL}\" prog show name " << cProgramName << " >/dev/null 2>&1\n";
    return script.str();
}

std::string BuildSystemdUnit()
{
    std::ostringstream unit;
    unit << "[Unit]\n";
    unit << "Description=OSConfig Copy Fail AF_ALG eBPF LSM mitigation\n";
    unit << "After=local-fs.target\n\n";
    unit << "[Service]\n";
    unit << "Type=oneshot\n";
    unit << "RemainAfterExit=yes\n";
    unit << "ExecStart=" << cLoaderScriptPath << "\n\n";
    unit << "[Install]\n";
    unit << "WantedBy=multi-user.target\n";
    return unit.str();
}

std::string BuildAuditCommand()
{
    return std::string("bpftool prog show name ") + cProgramName + " >/dev/null 2>&1";
}
} // namespace

Result<Status> AuditCopyFailMitigation(const CopyFailMitigationParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto paths = ParseAllowedExecutablePaths(params);
    if (!paths.HasValue())
    {
        return paths.Error();
    }

    const auto configPath = context.GetSpecialFilePath(cConfigPath);
    const auto expectedConfig = BuildAllowListConfig(paths.Value());
    auto existingConfig = context.GetFileContents(configPath);
    if (!existingConfig.HasValue())
    {
        return indicators.NonCompliant("Copy Fail AF_ALG eBPF LSM allow-list is not configured: " + existingConfig.Error().message);
    }
    if (existingConfig.Value() != expectedConfig)
    {
        return indicators.NonCompliant("Copy Fail AF_ALG eBPF LSM allow-list does not match expected policy assignment");
    }

    auto commandOutput = context.ExecuteCommand(BuildAuditCommand());
    if (!commandOutput.HasValue())
    {
        return indicators.NonCompliant("Copy Fail AF_ALG eBPF LSM program is not active: " + commandOutput.Error().message);
    }

    return indicators.Compliant("Copy Fail AF_ALG eBPF LSM mitigation is active");
}

Result<Status> RemediateCopyFailMitigation(const CopyFailMitigationParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    auto paths = ParseAllowedExecutablePaths(params);
    if (!paths.HasValue())
    {
        return paths.Error();
    }

    const auto configPath = context.GetSpecialFilePath(cConfigPath);
    const auto stateDirectory = context.GetSpecialFilePath(cStateDirectory);
    const auto bpfFsDirectory = context.GetSpecialFilePath(cBpfFsDirectory);
    const auto loaderScriptPath = context.GetSpecialFilePath(cLoaderScriptPath);
    const auto systemdUnitPath = context.GetSpecialFilePath(cSystemdUnitPath);

    auto result = EnsureDirectory(stateDirectory, 0755);
    if (!result.HasValue())
    {
        return result.Error();
    }

    result = WriteTextFile(configPath, BuildAllowListConfig(paths.Value()), 0644);
    if (!result.HasValue())
    {
        return result.Error();
    }

    result = WriteBinaryFile(stateDirectory + "/copyfail-afalg-lsm.bpf.o", gCopyFailMitigationBpfObject, gCopyFailMitigationBpfObject_len, 0644);
    if (!result.HasValue())
    {
        return result.Error();
    }

    result = WriteTextFile(loaderScriptPath, BuildLoaderScript(stateDirectory, bpfFsDirectory) + BuildMapUpdateScript(paths.Value()), 0755);
    if (!result.HasValue())
    {
        return result.Error();
    }

    result = WriteTextFile(systemdUnitPath, BuildSystemdUnit(), 0644);
    if (!result.HasValue())
    {
        return result.Error();
    }

    std::string command = "/bin/sh \"" + EscapeForShell(loaderScriptPath) + "\"";
    command += " && if command -v systemctl >/dev/null 2>&1; then systemctl daemon-reload >/dev/null 2>&1 && systemctl enable osconfig-copyfail-afalg-lsm.service >/dev/null 2>&1 || true; fi";
    auto commandOutput = context.ExecuteCommand(command);
    if (!commandOutput.HasValue())
    {
        return indicators.NonCompliant("Failed to load Copy Fail AF_ALG eBPF LSM mitigation: " + commandOutput.Error().message);
    }

    return indicators.Compliant("Copy Fail AF_ALG eBPF LSM mitigation loaded");
}

} // namespace ComplianceEngine