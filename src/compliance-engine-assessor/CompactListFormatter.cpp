#include <CompactListFormatter.hpp>
#include <sstream>
#include <iomanip>
#include <version.h>

namespace formatters
{
    using compliance::Error;
    using compliance::Optional;
    using compliance::Result;
    using compliance::Status;
    using compliance::Evaluator;
    using std::string;
    using std::chrono::system_clock;
    using std::chrono::steady_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    Optional<Error> CompactListFormatter::Begin(Evaluator::Action action)
    {
        mOutput << "Action: " << (action == Evaluator::Action::Audit ? "Audit" : "Remediation") << "\n";
        mOutput << "OsConfig Version: " << OSCONFIG_VERSION << "\n";
        mOutput << "Timestamp: " << to_iso_datetime(system_clock::now()) << "\n";
        mOutput << "Rules:\n";
        return Optional<Error>();
    }

    Optional<Error> CompactListFormatter::AddEntry(const mof::MofEntry& entry, Status status, const string& payload)
    {
        mOutput << entry.resourceID << ":\n";
        mOutput << payload;
        mOutput << "Status: " << (status == Status::Compliant ? "Compliant" : "NonCompliant") << "\n";
        return Optional<Error>();
    }

    Result<string> CompactListFormatter::Finish(compliance::Status status)
    {
        mOutput << "Duration: " << std::chrono::duration_cast<milliseconds>(steady_clock::now() - mBegin).count() << " ms\n";
        mOutput << "Status: " << (status == Status::Compliant ? "Compliant" : "NonCompliant") << "\n";
        mOutput << "End of Report";
        return mOutput.str();
    }
} // namespace formatters
