#ifndef MMI_FORMATTER_HPP
#define MMI_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <memory>
#include <sstream>

namespace formatters
{
    struct MmiFormatter : public BenchmarkFormatter
    {
        ComplianceEngine::Optional<ComplianceEngine::Error> Begin(ComplianceEngine::Evaluator::Action action) override;
        ComplianceEngine::Optional<ComplianceEngine::Error> AddEntry(const mof::MofEntry& entry, ComplianceEngine::Status status, const std::string& payload) override;
        ComplianceEngine::Result<std::string> Finish(ComplianceEngine::Status status) override;

    private:
        std::ostringstream mOutput;
    };
} // namespace formatters

#endif // MMI_FORMATTER_HPP
