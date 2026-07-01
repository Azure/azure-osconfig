// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_CONTEXTINTERFACE_H
#define COMPLIANCEENGINE_CONTEXTINTERFACE_H

#include "FilesystemScanner.h"
#include "Logging.h"
#include "Result.h"

#include <string>
#include <vector>

namespace ComplianceEngine
{
// A single network interface as reported by the system: its name and the IFF_* flags
// bitmask (see <net/if.h>). Produced by ContextInterface::GetNetworkInterfaces().
struct InterfaceInfo
{
    std::string name;
    unsigned int flags = 0;
};

class ContextInterface
{
public:
    virtual ~ContextInterface() = 0;
    virtual Result<std::string> ExecuteCommand(const std::string& cmd) const = 0;
    virtual Result<std::string> GetFileContents(const std::string& filePath) const = 0;

    // Enumerates the host's network interfaces with their IFF_* flags. On success returns
    // the list (possibly empty when the system genuinely has no interfaces); on failure
    // returns an Error whose code is the underlying errno. This is the mockable seam that
    // replaces direct /proc/net/dev + /sys/class/net reads.
    virtual Result<std::vector<InterfaceInfo>> GetNetworkInterfaces() const = 0;

    virtual OsConfigLogHandle GetLogHandle() const = 0;
    virtual std::string GetSpecialFilePath(const std::string& path) const = 0;

    virtual FilesystemScanner& GetFilesystemScanner() = 0;

    // Returns the path to be used for storing state.
    virtual std::string GetStatePath() const = 0;
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_CONTEXT_H
