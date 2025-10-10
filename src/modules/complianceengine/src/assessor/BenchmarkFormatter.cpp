#include <BenchmarkFormatter.hpp>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace ComplianceEngine
{
namespace BenchmarkFormatters
{
using std::string;
using std::chrono::system_clock;

string BenchmarkFormatter::ToISODatetime(const system_clock::time_point& tp)
{
    const auto time = system_clock::to_time_t(tp);
    const auto tm = *std::gmtime(&time); // Convert to UTC time

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

BenchmarkFormatter::BenchmarkFormatter()
{
    mBegin = std::chrono::steady_clock::now();
}
} // namespace BenchmarkFormatters
} // namespace ComplianceEngine
