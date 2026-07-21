#ifndef COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP
#define COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP

#include <DistributionInfo.h>
#include <Evaluator.h>
#include <JsonWrapper.h>
#include <Mof.hpp>
#include <Optional.h>
#include <Result.h>
#include <chrono>
#include <map>
#include <string>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
// Formats a compliance scan run as a canonical JSON result document. Obtain an
// instance via Begin(), which initialises the result envelope and binds host
// provenance from the supplied DistributionInfo. Call AddEntry() for each
// evaluated rule and Finish() to obtain the serialised JSON.
class BenchmarkFormatter
{
public:
    static Result<BenchmarkFormatter> Begin(DistributionInfo distributionInfo, Action action);

    ~BenchmarkFormatter() = default;
    BenchmarkFormatter(const BenchmarkFormatter&) = delete;
    BenchmarkFormatter& operator=(const BenchmarkFormatter&) = delete;
    BenchmarkFormatter(BenchmarkFormatter&&) = default;
    BenchmarkFormatter& operator=(BenchmarkFormatter&&) = default;

    Optional<Error> AddEntry(const MOF::Resource& entry, Status status, const std::string& payload, const std::map<std::string, std::string>& parameters) &;
    Result<std::string> Finish(Status status) &&;

private:
    static std::string ToISODatetime(const std::chrono::system_clock::time_point& tp);
    explicit BenchmarkFormatter(DistributionInfo distributionInfo);

    std::chrono::time_point<std::chrono::steady_clock> mBegin;
    DistributionInfo mDistributionInfo;
    JsonWrapper mJson;
};
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP
