#ifndef COMPLIANCE_ENGINE_COMPACT_LIST_FORMATTER_HPP
#define COMPLIANCE_ENGINE_COMPACT_LIST_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <sstream>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
struct CompactListFormatter : public BenchmarkFormatter
{
    Optional<Error> Begin(Action action) override;
    Optional<Error> AddEntry(const MOF::Resource& entry, Status status, const std::string& payload) override;
    Result<std::string> Finish(Status status) override;

private:
    std::ostringstream mOutput;
};
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_COMPACT_LIST_FORMATTER_HPP
