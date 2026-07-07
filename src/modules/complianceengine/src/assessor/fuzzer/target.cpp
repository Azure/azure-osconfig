// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// libFuzzer target for the MOF parser used by the compliance-engine-assessor.
//
// Scope: parser only (MofResourceRange / MofResourceIterator). The engine and
// procedure evaluation are intentionally out of scope — the goal is to ensure
// the parser is crash-free and bounded against adversarial input.
//
// Run:
//   ./mof-parser-fuzzer -max_total_time=60 seed_corpus/

#include "Mof.hpp"

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

using ComplianceEngine::MOF::MofResourceRange;

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (size == 0)
    {
        return 0; // not interesting
    }

    try
    {
        std::string input(reinterpret_cast<const char*>(data), size);
        std::istringstream stream(input);

        auto rangeResult = MofResourceRange::Make(stream, nullptr);
        if (!rangeResult.HasValue())
        {
            return 0; // rejected at open; not a crash
        }

        // Drain the iterator, discarding results. We only care that the parser
        // is crash-free and that every error is surfaced as a Result<> rather
        // than an exception or undefined behaviour.
        for (const auto& entry : rangeResult.Value())
        {
            (void)entry;
        }
    }
    catch (...)
    {
        // The parser must never throw. Surface any exception as a crash so
        // libFuzzer records the reproducer.
        __builtin_trap();
    }
    return 0;
}
