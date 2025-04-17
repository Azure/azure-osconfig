#ifndef BENCHMARK_FORMATTER_HPP
#define BENCHMARK_FORMATTER_HPP

#include <string>
#include <Optional.h>
#include <Result.h>
#include <chrono>
#include <Evaluator.h>
#include <Mof.hpp>

namespace formatters
{
    struct BenchmarkFormatter
    {
        static std::string to_iso_datetime(const std::chrono::system_clock::time_point& tp);
        std::chrono::time_point<std::chrono::steady_clock> mBegin;

        BenchmarkFormatter();
        virtual ~BenchmarkFormatter() = default;
        virtual ComplianceEngine::Optional<ComplianceEngine::Error> Begin(ComplianceEngine::Evaluator::Action action) = 0;
        virtual ComplianceEngine::Optional<ComplianceEngine::Error> AddEntry(const mof::MofEntry& entry, ComplianceEngine::Status status, const std::string& payload) = 0;
        virtual ComplianceEngine::Result<std::string> Finish(ComplianceEngine::Status status) = 0;
    };
} // namespace formatters

#endif // BENCHMARK_FORMATTER_HPP
