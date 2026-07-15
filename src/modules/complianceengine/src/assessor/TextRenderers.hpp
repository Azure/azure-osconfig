// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESSOR_TEXT_RENDERERS_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_TEXT_RENDERERS_HPP

#include <Result.h>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{
// Human-readable text presentations produced by the `render` subcommand from a
// over the JSON).
enum class TextStyle
{
    // Per-rule header line plus an indented indicator tree.
    NestedList,
    // One line per rule: status + section + name (no indicators).
    CompactList,
    // Verbose: identity, title, parameters, and the full indicator tree.
    Debug
};

Result<std::string> RenderText(const std::string& canonicalJson, TextStyle style);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_TEXT_RENDERERS_HPP
