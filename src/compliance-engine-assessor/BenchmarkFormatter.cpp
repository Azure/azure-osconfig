#include <BenchmarkFormatter.hpp>
#include <iomanip>
#include <cstdlib>
#include <sstream>

namespace formatters
{
using std::string;
using std::chrono::system_clock;

string BenchmarkFormatter::to_iso_datetime(const system_clock::time_point& tp)
{
    auto time = system_clock::to_time_t(tp);
    auto tm = *std::gmtime(&time); // Convert to UTC time

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

BenchmarkFormatter::BenchmarkFormatter()
{
    mBegin = std::chrono::steady_clock::now();
}
} // namespace formatters
