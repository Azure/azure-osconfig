#ifndef NESTED_LIST_FORMATTER_HPP
#define NESTED_LIST_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <memory>
#include <sstream>

namespace formatters
{
    struct NestedListFormatter : public BenchmarkFormatter
    {
        ComplianceEngine::Optional<ComplianceEngine::Error> Begin(ComplianceEngine::Evaluator::Action action) override;
        ComplianceEngine::Optional<ComplianceEngine::Error> AddEntry(const mof::MofEntry& entry, ComplianceEngine::Status status, const std::string& payload) override;
        ComplianceEngine::Result<std::string> Finish(ComplianceEngine::Status status) override;

    private:
        std::ostringstream mOutput;
    };
} // namespace formatters

#endif // NESTED_LIST_FORMATTER_HPP
