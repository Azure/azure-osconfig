// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESSOR_MOF_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_MOF_HPP

#include <BenchmarkInfo.h>
#include <Logging.h>
#include <Optional.h>
#include <Result.h>
#include <istream>
#include <memory>
#include <streambuf>
#include <string>

namespace ComplianceEngine
{
namespace MOF
{
// A single parsed MOF resource entry.
//
// Only the fields actually consumed by the assessor's main loop and output
// formatters are stored. The parser validates every field the augmentation
// engine emits (see MofResourceRange), but deliberately discards the ones the
// assessor does not use (the constant ComponentName/ExpectedObjectValue/
// ConfigurationName/ModuleName fields, ModuleVersion, SourceInfo, etc.).
struct Resource
{
    // ResourceID, e.g. "1.1.1.1 Ensure cramfs kernel module is not available".
    // Emitted in the canonical result JSON as `title` (the human-readable rule
    // title), reusing the definition's field name.
    std::string resourceID;

    // RuleId: the stable, benchmark-agnostic rule identifier (a UUID derived
    // from the payload key by the augmentation engine). Retained and emitted in
    // the canonical result JSON as `ruleId` so tooling can join to a rule
    // reliably rather than matching on ruleName/section.
    std::string ruleId;

    // Benchmark info parsed from the PayloadKey. `.distribution` and `.version`
    // drive the applicability check in the main loop (Match against the detected
    // system), `.section` drives section filtering (main loop and JSON
    // formatter); the whole struct is retained so the formatters need no changes.
    CISBenchmarkInfo benchmarkInfo;

    // Base64-encoded rule payload (ProcedureObjectValue). The parser does NOT
    // decode or validate this blob; base64/JSON parsing is owned by the
    // ComplianceEngine (Engine/Procedure), which keeps the parser/engine
    // boundary clear. See MofResourceRange for the validation the parser does do.
    std::string procedure;

    // DesiredObjectValue. The augmentation engine emits an empty string for
    // every rule today; an empty value is modelled as an absent payload.
    Optional<std::string> payload;

    // The rule name shared by the ProcedureObjectName ("procedure<Name>"),
    // InitObjectName ("init<Name>"), ReportedObjectName ("audit<Name>") and
    // DesiredObjectName ("remediate<Name>") fields. The parser enforces that all
    // four agree before storing the common suffix here.
    std::string ruleName;

    // True when the entry carries an InitObjectName (the engine always emits one).
    bool hasInitAudit = false;
};

class MofResourceRange;

// Input iterator over the MOF resource entries in a stream.
//
// Dereferencing yields a `const Result<Resource>&`: a per-entry parse error is
// delivered in-band as an Error value so the caller can check each entry and
// bail out, rather than being thrown. Once an entry fails to parse (or end of
// input is reached) the iterator becomes equal to end(); a desynchronized
// stream cannot be resumed reliably, so iteration stops.
class MofResourceIterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Result<Resource>;
    using difference_type = std::ptrdiff_t;
    using pointer = const Result<Resource>*;
    using reference = const Result<Resource>&;

    reference operator*() const;
    pointer operator->() const;
    MofResourceIterator& operator++();
    bool operator==(const MofResourceIterator& other) const;
    bool operator!=(const MofResourceIterator& other) const;

private:
    friend class MofResourceRange;
    explicit MofResourceIterator(MofResourceRange& range);
    MofResourceIterator() = default; // end iterator

    void Advance();

    MofResourceRange* mRange = nullptr; // nullptr == end
    bool mErrored = false;
    Result<Resource> mCurrent = ComplianceEngine::Error("uninitialized MOF iterator");
};

// Owns the input stream (RAII) and streams strictly-validated MOF resource
// entries from it. Construct via the Make* factories, which encapsulate the
// input-hardening safeguards so callers never open the input file themselves.
//
// Strictness: the parser targets the exact, regular format emitted by the
// Compliance Augmentation Engine. It requires every expected field to be
// present, rejects unknown field keys, validates the constant fields
// (ComponentName/ExpectedObjectValue/ConfigurationName) and enforces that the
// four object-name fields share a single rule name. The base64 ProcedureObject
// payload is passed through untouched; decoding/JSON validation is the
// ComplianceEngine's responsibility.
class MofResourceRange
{
public:
    // Opens a regular file on disk, applying the full input-hardening posture
    // (path-traversal rejection, root-owned non-writable parent directory,
    // O_NOFOLLOW open, regular-file/ownership/mode fstat checks) before the
    // first byte is read. The verified fd is owned and closed on destruction.
    static Result<MofResourceRange> Make(const std::string& path, OsConfigLogHandle logHandle);

    // Streams from stdin (or any externally-owned istream representing stdin).
    // The stream is not owned. stdin has no on-disk identity, so the file-based
    // safeguards do not apply, but the size/line/entry caps still bound resource
    // usage.
    static Result<MofResourceRange> Make(std::istream& stream, OsConfigLogHandle logHandle);

    // Streams from an externally-owned istream. Intended for unit tests and the
    // fuzzer; applies the same strict parsing and caps as the other factories.
    static Result<MofResourceRange> MakeFromStream(std::istream& stream, OsConfigLogHandle logHandle);

    MofResourceRange(MofResourceRange&& other) noexcept;
    MofResourceRange& operator=(MofResourceRange&&) = delete;
    MofResourceRange(const MofResourceRange&) = delete;
    MofResourceRange& operator=(const MofResourceRange&) = delete;
    ~MofResourceRange() = default;

    MofResourceIterator begin(); // NOLINT(*-identifier-naming)
    MofResourceIterator end();   // NOLINT(*-identifier-naming)

private:
    friend class MofResourceIterator;
    MofResourceRange(std::istream& stream, OsConfigLogHandle logHandle) noexcept;

    // Parses the next entry from the stream, enforcing the size/line/entry caps.
    // Returns:
    //   - a Resource             -> a parsed entry,
    //   - an empty Optional      -> clean end of input (no more entries),
    //   - an Error               -> malformed input or a cap was exceeded.
    Result<Optional<Resource>> ParseNext();

    std::istream* mStream;
    OsConfigLogHandle mLog;
    // For file inputs the range owns the streambuf that bridges the verified fd
    // into an istream (and the istream itself). For stdin/test inputs these are
    // null and mStream references the caller's stream. Declared before
    // mOwnedStream so the istream is destroyed before the streambuf it uses.
    std::unique_ptr<std::streambuf> mOwnedBuf;
    std::unique_ptr<std::istream> mOwnedStream;
    size_t mBytesConsumed = 0;
    size_t mEntryCount = 0;
};
} // namespace MOF
} // namespace ComplianceEngine
#endif // COMPLIANCE_ENGINE_ASSESSOR_MOF_HPP
