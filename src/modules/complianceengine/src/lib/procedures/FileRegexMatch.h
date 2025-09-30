// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#ifndef COMPLIANCEENGINE_PROCEDURES_FILEREGEXMATCH_H
#define COMPLIANCEENGINE_PROCEDURES_FILEREGEXMATCH_H

#include <Evaluator.h>
#include <Regex.h>

namespace ComplianceEngine
{
// Used by FileRegexMatch procedure and represents an operation using the provided patterns.
// Currently only 'pattern match' operation is supported.
enum class Operation
{
    /// label: pattern match
    Match,
};

// Used by FileRegexMatch procedure to determine how the function should
// interpret matching results.
enum class Behavior
{
    /// label: all_exist
    AllExist,

    /// label: any_exist
    AnyExist,

    /// label: at_least_one_exists
    AtLeastOneExists,

    /// label: none_exist
    NoneExist,

    /// label: only_one_exists
    OnlyOneExists,
};

// Used by FileRegexMatch procedure to determine case sensitivity
// for the matchPattern and statePattern fields.
enum class IgnoreCase
{
    /// label: matchPattern statePattern
    Both,

    /// label: matchPattern
    MatchPattern,

    /// label: statePattern
    StatePattern,
};

// Parameters used by the FileRegexMatch procedure.
struct AuditFileRegexMatchParams
{
    /// A directory name contining files to check
    std::string path;

    /// A pattern to match file names in the provided path
    regex filenamePattern;

    /// Operation to perform on the file contents
    /// pattern: ^pattern match$
    Optional<Operation> matchOperation = Operation::Match;

    /// The pattern to match against the file contents
    std::string matchPattern;

    /// Operation to perform on each line that matches the 'matchPattern'
    /// pattern: ^pattern match$
    Optional<Operation> stateOperation = Operation::Match;

    /// The pattern to match against each line that matches the 'statePattern'
    Optional<std::string> statePattern;

    /// Determine whether a match or state should ignore case sensitivity 'matchPattern' and 'statePattern' or none when empty'
    /// pattern: ^(matchPattern\sstatePattern|matchPattern|statePattern)$
    Optional<IgnoreCase> ignoreCase;

    /// Determine the function behavior
    /// pattern: ^(all_exist|any_exist|at_least_one_exists|none_exist|only_one_exists)$
    Optional<Behavior> behavior = Behavior::AllExist;
};

Result<Status> AuditFileRegexMatch(const AuditFileRegexMatchParams& params, IndicatorsTree& indicators, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_PROCEDURES_FILEREGEXMATCH_H
