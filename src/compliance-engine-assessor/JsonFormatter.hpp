#ifndef JSON_FORMATTER_HPP
#define JSON_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <memory>
#include <JsonWrapper.h>

struct json_object_t;

namespace formatters
{
    struct JsonFormatter : public BenchmarkFormatter
    {
        ComplianceEngine::Optional<ComplianceEngine::Error> Begin(ComplianceEngine::Evaluator::Action action) override;
        ComplianceEngine::Optional<ComplianceEngine::Error> AddEntry(const mof::MofEntry& entry, ComplianceEngine::Status status, const std::string& payload) override;
        ComplianceEngine::Result<std::string> Finish(ComplianceEngine::Status status) override;

    private:
        ComplianceEngine::JsonWrapper mJson;
    };
} // namespace formatters

#endif // JSON_FORMATTER_HPP
