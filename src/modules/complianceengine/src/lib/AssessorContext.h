// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_ASSESSORCONTEXT_H
#define COMPLIANCEENGINE_ASSESSORCONTEXT_H

#include "CommonContext.h"
#include "Logging.h"

#include <ftw.h>
#include <stdexcept>
#include <unistd.h>

namespace ComplianceEngine
{

class AssessorContext : public CommonContext
{
public:
    AssessorContext(OsConfigLogHandle log)
        : CommonContext(log, CreateTempDir())
    {
    }
    AssessorContext(const AssessorContext&) = delete;
    AssessorContext& operator=(const AssessorContext&) = delete;
    AssessorContext(AssessorContext&&) = delete;
    AssessorContext& operator=(AssessorContext&&) = delete;

    ~AssessorContext() override
    {
        const std::string statePath = GetStatePath();
        nftw(
            statePath.c_str(),
            [](const char* fpath, const struct stat*, int typeflag, struct FTW*) -> int {
                if (typeflag == FTW_DP)
                {
                    (void)rmdir(fpath);
                }
                else
                {
                    (void)unlink(fpath);
                }
                return 0; // best-effort cleanup: keep walking even if a removal fails
            },
            64, FTW_DEPTH | FTW_PHYS);
    }

private:
    static std::string CreateTempDir()
    {
        char tmpl[] = "/tmp/compliance-engine-assessor.XXXXXX";
        if (mkdtemp(tmpl) == nullptr)
        {
            throw std::runtime_error("AssessorContext: failed to create temporary state directory");
        }
        return std::string(tmpl);
    }
};

} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_ASSESSORCONTEXT_H
