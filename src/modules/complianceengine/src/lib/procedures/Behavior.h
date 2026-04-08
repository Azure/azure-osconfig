#ifndef COMPLIANCEENGINE_PROCEDURES_BEHAVIOR_H
#define COMPLIANCEENGINE_PROCEDURES_BEHAVIOR_H

// Used by EnsureFilePermissions, FileRegexMatch procedure to determine how the function should
// interpret results.
namespace ComplianceEngine
{
enum class Behavior
{
    /// label: check_if_exists
    CheckIfExists,

    /// label: at_least_one_exists
    AtLeastOneExists,

    /// label: all_exist
    AllExist,

    /// label: any_exist
    AnyExist,

    /// label: none_exist
    NoneExist,

    /// label: only_one_exists
    OnlyOneExists,
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_PROCEDURES_BEHAVIOR_H
