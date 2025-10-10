#ifndef COMPLIANCE_ENGINE_JSON_FORMATTER_HPP
#define COMPLIANCE_ENGINE_JSON_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <JsonWrapper.h>
#include <memory>

struct json_object_t;

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
struct JsonFormatter : public BenchmarkFormatter
{
    Optional<Error> Begin(Action action) override;
    Optional<Error> AddEntry(const MOF::Resource& entry, Status status, const std::string& payload) override;
    Result<std::string> Finish(Status status) override;

private:
    JsonWrapper mJson;
};
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_JSON_FORMATTER_HPP
