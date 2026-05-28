// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESOR_MOF_HPP
#define COMPLIANCE_ENGINE_ASSESOR_MOF_HPP

#include <BenchmarkInfo.h>
#include <Engine.h>
#include <Optional.h>
#include <Result.h>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>

namespace ComplianceEngine
{
namespace MOF
{

// Adversarial-input bounds. The MOF parser is intentionally loose, but it must
// not let a hostile producer drive the assessor to unbounded memory use.
//
// kMaxLineLength is enforced when reading a line: anything longer is rejected
// with an error. The longest legitimate line is the base64-encoded procedure
// payload, which currently tops out well below this limit.
constexpr std::size_t kMaxLineLength = 8u * 1024u * 1024u; // 8 MiB per line

// kMaxEntriesPerStream bounds how many "instance of OsConfigResource" entries
// ParseAll will accept from a single stream. Real benchmarks ship a few
// thousand rules; this limit is generous but finite.
constexpr std::size_t kMaxEntriesPerStream = 100000u;

struct Resource
{
    std::string resourceID;
    CISBenchmarkInfo benchmarkInfo;
    std::string procedure;
    Optional<std::string> payload;
    std::string ruleName;
    bool hasInitAudit = false;

    // Parses a single "instance of OsConfigResource as ..." body from `stream`,
    // starting at the first line AFTER the "instance of OsConfigResource as"
    // line. Stops when it sees the terminating "};" or EOF.
    static Result<Resource> ParseSingleEntry(std::istream& stream);
};

// Iterates over a MOF stream, locates each "instance of OsConfigResource as"
// header, parses the following body with ParseSingleEntry, and invokes
// `callback` for every successfully parsed entry. Stops early (returning the
// callback's error) if the callback returns one.
//
// Bounds: line length is capped at kMaxLineLength and the total number of
// parsed entries is capped at kMaxEntriesPerStream.
using EntryCallback = std::function<Optional<Error>(Resource&&)>;
Optional<Error> ParseAll(std::istream& stream, const EntryCallback& callback);

} // namespace MOF
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_ASSESOR_MOF_HPP
