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
        compliance::Optional<compliance::Error> Begin(compliance::Evaluator::Action action) override;
        compliance::Optional<compliance::Error> AddEntry(const mof::MofEntry& entry, compliance::Status status, const std::string& payload) override;
        compliance::Result<std::string> Finish(compliance::Status status) override;

    private:
        compliance::JsonWrapper mJson;
    };
} // namespace formatters

#endif // JSON_FORMATTER_HPP
