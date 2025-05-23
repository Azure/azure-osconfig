#ifndef MMI_FORMATTER_HPP
#define MMI_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <memory>
#include <sstream>

namespace formatters
{
    struct MmiFormatter : public BenchmarkFormatter
    {
        compliance::Optional<compliance::Error> Begin(compliance::Evaluator::Action action) override;
        compliance::Optional<compliance::Error> AddEntry(const mof::MofEntry& entry, compliance::Status status, const std::string& payload) override;
        compliance::Result<std::string> Finish(compliance::Status status) override;

    private:
        std::ostringstream mOutput;
    };
} // namespace formatters

#endif // MMI_FORMATTER_HPP
