// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_ENGINE_ASSESSOR_JUNIT_RENDERER_HPP
#define COMPLIANCE_ENGINE_ASSESSOR_JUNIT_RENDERER_HPP

#include <Result.h>
#include <string>

namespace ComplianceEngine
{
namespace Assessor
{
// Renders a canonical assessor result JSON (as emitted by `audit` / `remediate`)
// into a JUnit XML document.
//
// - one <testcase classname=<section> name=<ruleName>> per rule,
// - a <failure> only for NonCompliant rules (Compliant rules are bare
//   passing <testcase/>),
// - the failure body carries the rule's Parameters and Indicators, modelled on
//   the augmentation engine's tests/reporting/junit.py.
//
// `section` is used verbatim as the classname; it is framework-agnostic (a
// dotted CIS number or a STIG id), so the renderer makes no CIS-specific
// assumptions. `suiteName` names the <testsuite>; the assessor does not know
// which benchmark package it came from, so the caller supplies it.
Result<std::string> RenderJUnit(const std::string& canonicalJson, const std::string& suiteName);

} // namespace Assessor
} // namespace ComplianceEngine

#endif // COMPLIANCE_ENGINE_ASSESSOR_JUNIT_RENDERER_HPP
