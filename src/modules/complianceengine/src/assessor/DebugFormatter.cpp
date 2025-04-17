#include <DebugFormatter.hpp>
#include <version.h>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
using ComplianceEngine::Action;
using ComplianceEngine::Error;
using ComplianceEngine::Evaluator;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::string;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::chrono::system_clock;

Optional<Error> DebugFormatter::Begin(const Action action)
{
    mOutput << "Action: " << (action == Action::Audit ? "Audit" : "Remediation") << "\n";
    mOutput << "OsConfig Version: " << OSCONFIG_VERSION << "\n";
    mOutput << "Timestamp: " << ToISODatetime(system_clock::now()) << "\n";
    mOutput << "Rules:\n";
    return Optional<Error>();
}

Optional<Error> DebugFormatter::AddEntry(const MOF::Resource& entry, const Status status, const string& payload)
{
    mOutput << entry.resourceID << ":\n";
    mOutput << payload << "\n";
    mOutput << "Status: " << (status == Status::Compliant ? "Compliant" : "NonCompliant") << "\n";
    return Optional<Error>();
}

Result<string> DebugFormatter::Finish(const Status status)
{
    mOutput << "Duration: " << std::chrono::duration_cast<milliseconds>(steady_clock::now() - mBegin).count() << " ms\n";
    mOutput << "Status: " << (status == Status::Compliant ? "Compliant" : "NonCompliant") << "\n";
    mOutput << "End of Report";
    return mOutput.str();
}
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
