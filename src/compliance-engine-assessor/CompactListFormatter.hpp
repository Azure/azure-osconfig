#ifndef COMPACT_LIST_FORMATTER_HPP
#define COMPACT_LIST_FORMATTER_HPP

#include <BenchmarkFormatter.hpp>
#include <sstream>

namespace formatters
{
    struct CompactListFormatter : public BenchmarkFormatter
    {
        compliance::Optional<compliance::Error> Begin(compliance::Evaluator::Action action) override;
        compliance::Optional<compliance::Error> AddEntry(const mof::MofEntry& entry, compliance::Status status, const std::string& payload) override;
        compliance::Result<std::string> Finish(compliance::Status status) override;

    private:
        std::ostringstream mOutput;
    };
} // namespace formatters

#endif // COMPACT_LIST_FORMATTER_HPP
