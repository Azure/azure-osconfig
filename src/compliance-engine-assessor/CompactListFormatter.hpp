#ifndef COMPACT_LIST_FORMATTER_HPP
#define COMPACT_LIST_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <sstream>

namespace formatters
{
    struct CompactListFormatter : public BenchmarkFormatter
    {
        ComplianceEngine::Optional<ComplianceEngine::Error> Begin(ComplianceEngine::Evaluator::Action action) override;
        ComplianceEngine::Optional<ComplianceEngine::Error> AddEntry(const mof::MofEntry& entry, ComplianceEngine::Status status, const std::string& payload) override;
        ComplianceEngine::Result<std::string> Finish(ComplianceEngine::Status status) override;

    private:
        std::ostringstream mOutput;
    };
} // namespace formatters

#endif // COMPACT_LIST_FORMATTER_HPP
