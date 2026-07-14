#ifndef COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP
#define COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP

#include <Evaluator.h>
#include <Mof.hpp>
#include <Optional.h>
#include <Result.h>
#include <chrono>
#include <string>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
// Provenance of the host the scan actually ran on. The benchmark definitions
// are architecture-agnostic, so the arch/distribution a result was produced on
// is recorded in the result (not the definition) for multi-arch traceability.
struct HostInfo
{
    std::string arch;                // uname(2) machine, e.g. "x86_64" / "aarch64"
    std::string distribution;        // /etc/os-release-derived distribution
    std::string distributionVersion; // /etc/os-release VERSION_ID
};

struct BenchmarkFormatter
{
    static std::string ToISODatetime(const std::chrono::system_clock::time_point& tp);
    std::chrono::time_point<std::chrono::steady_clock> mBegin;

    BenchmarkFormatter();
    virtual ~BenchmarkFormatter() = default;
    BenchmarkFormatter(const BenchmarkFormatter&) = default;
    BenchmarkFormatter& operator=(const BenchmarkFormatter&) = default;
    BenchmarkFormatter(BenchmarkFormatter&&) = default;
    BenchmarkFormatter& operator=(BenchmarkFormatter&&) = default;

    // Supplies host provenance to be recorded in the output. Only the JSON
    // formatter emits it today; the other formatters ignore it. Set before
    // Begin().
    void SetHostInfo(HostInfo info)
    {
        mHostInfo = std::move(info);
    }

    virtual Optional<Error> Begin(Action action) = 0;
    virtual Optional<Error> AddEntry(const MOF::Resource& entry, Status status, const std::string& payload) = 0;
    virtual Result<std::string> Finish(Status status) = 0;

protected:
    Optional<HostInfo> mHostInfo;
};
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP
