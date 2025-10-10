#ifndef COMPLIANCE_ENGINE_ASSESOR_MOF_HPP
#define COMPLIANCE_ENGINE_ASSESOR_MOF_HPP

#include <BenchmarkInfo.h>
#include <Engine.h>
#include <Optional.h>
#include <Result.h>
#include <string>

namespace ComplianceEngine
{
namespace MOF
{
struct SemVer
{
    int major;
    int minor;
    int patch;

    static Result<SemVer> Parse(const std::string& version);
};

struct Resource
{
    std::string resourceID;
    CISBenchmarkInfo benchmarkInfo;
    std::string procedure;
    Optional<std::string> payload;
    std::string ruleName;
    bool hasInitAudit = false;

    static Result<Resource> ParseSingleEntry(std::istream& stream);
};
} // namespace MOF
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_ASSESOR_MOF_HPP
