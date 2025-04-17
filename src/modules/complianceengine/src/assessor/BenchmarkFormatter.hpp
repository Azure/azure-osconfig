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

    virtual Optional<Error> Begin(Action action) = 0;
    virtual Optional<Error> AddEntry(const MOF::Resource& entry, Status status, const std::string& payload) = 0;
    virtual Result<std::string> Finish(Status status) = 0;
};
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_BENCHMARK_FORMATTER_HPP
