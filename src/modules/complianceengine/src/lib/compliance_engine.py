# This file is auto-generated. Do not edit manually.
# Python bindings for ComplianceEngine built-in procedures.
# Requires: marshmallow-dataclass, marshmallow

from __future__ import annotations

import dataclasses
import re
from enum import Enum
from typing import List, Optional, Union

import marshmallow
import marshmallow.fields as mf
import marshmallow_dataclass


# ---------------------------------------------------------------------------
# Shared helper types
# ---------------------------------------------------------------------------

class OctalString(mf.String):
    """Marshmallow field for octal permission strings, e.g. '0644'."""

    def _validate(self, value):
        super()._validate(value)
        if not re.fullmatch(r'[0-7]{3,4}', value):
            raise marshmallow.ValidationError(
                f"Expected an octal permission string (3-4 octal digits), got {value!r}"
            )


class PatternString(mf.String):
    """Marshmallow field for regex-pattern strings."""

    def _validate(self, value):
        super()._validate(value)
        try:
            re.compile(value)
        except re.error as exc:
            raise marshmallow.ValidationError(
                f"Invalid regex pattern {value!r}: {exc}"
            ) from exc


class SeparatedField(mf.Field):
    """Marshmallow field for a delimiter-separated list of strings."""

    def __init__(self, delimiter: str = '|', *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.delimiter = delimiter

    def _serialize(self, value, attr, obj, **kwargs):
        if value is None:
            return None
        if isinstance(value, list):
            return self.delimiter.join(str(v) for v in value)
        return str(value)

    def _deserialize(self, value, attr, data, **kwargs):
        if value is None:
            return None
        if not isinstance(value, str):
            raise marshmallow.ValidationError("Expected a string")
        return value.split(self.delimiter)


# ---------------------------------------------------------------------------
# Enums
# ---------------------------------------------------------------------------

class Behavior(str, Enum):
    CHECK_IF_EXISTS = "check_if_exists"
    AT_LEAST_ONE_EXISTS = "at_least_one_exists"
    ALL_EXIST = "all_exist"
    ANY_EXIST = "any_exist"
    NONE_EXIST = "none_exist"
    ONLY_ONE_EXISTS = "only_one_exists"


class ComparisonOperation(str, Enum):
    EQ = "eq"
    NE = "ne"
    LT = "lt"
    LE = "le"
    GT = "gt"
    GE = "ge"
    MATCH = "match"


class DConfOperation(str, Enum):
    EQ = "eq"
    NE = "ne"


class EnsureSshdOptionMode(str, Enum):
    REGULAR = "regular"
    ALL_MATCHES = "all_matches"


class EnsureSshdOptionOperation(str, Enum):
    REGEX = "regex"
    MATCH = "match"
    NOT_MATCH = "not_match"
    LT = "lt"
    LE = "le"
    GT = "gt"
    GE = "ge"


class Field(str, Enum):
    USERNAME = "username"
    PASSWORD = "password"
    LAST_CHANGE = "last_change"
    MIN_AGE = "min_age"
    MAX_AGE = "max_age"
    WARN_PERIOD = "warn_period"
    INACTIVITY_PERIOD = "inactivity_period"
    EXPIRATION_DATE = "expiration_date"
    RESERVED = "reserved"
    ENCRYPTION_METHOD = "encryption_method"


class GsettingsKeyType(str, Enum):
    NUMBER = "number"
    STRING = "string"


class GsettingsOperationType(str, Enum):
    EQ = "eq"
    NE = "ne"
    LT = "lt"
    GT = "gt"
    IS_UNLOCKED = "is-unlocked"


class IgnoreCase(str, Enum):
    MATCHPATTERN_STATEPATTERN = "matchPattern statePattern"
    MATCHPATTERN = "matchPattern"
    STATEPATTERN = "statePattern"


class Operation(str, Enum):
    PATTERN_MATCH = "pattern match"


class PackageManagerType(str, Enum):
    AUTODETECT = "autodetect"
    RPM = "rpm"
    DPKG = "dpkg"


class RegexType(str, Enum):
    """Extended regex inverted"""
    P = "P"
    E = "E"
    PV = "Pv"
    EV = "Ev"


class SshKeyType(str, Enum):
    PUBLIC = "public"
    PRIVATE = "private"


class SystemdParameterOperator(str, Enum):
    LT = "lt"
    LE = "le"
    GT = "gt"
    GE = "ge"
    EQ = "eq"


# ---------------------------------------------------------------------------
# Procedure parameter dataclasses
# ---------------------------------------------------------------------------

@marshmallow_dataclass.dataclass
class AuditFailure:
    """Parameters for the AuditFailure procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class AuditGetParamValues:
    """Parameters for the AuditGetParamValues procedure."""

    KEY1: Optional[str] = dataclasses.field(default=None)

    KEY2: Optional[str] = dataclasses.field(default=None)

    KEY3: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class AuditNotApplicable:
    """Parameters for the AuditNotApplicable procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class AuditSuccess:
    """Parameters for the AuditSuccess procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class AuditdRulesCheck:
    """Parameters for the AuditdRulesCheck procedure."""

    # Item being audited
    searchItem: str = dataclasses.field(default=marshmallow.missing)

    # Option the checked rule line cannot include
    excludeOption: Optional[str] = dataclasses.field(default=None)

    # Options that should be included on the rule line, colon separated
    requiredOptions: List[str] = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": SeparatedField(delimiter=':')})


@marshmallow_dataclass.dataclass
class EnsureAccountsWithoutShellAreLocked:
    """Parameters for the EnsureAccountsWithoutShellAreLocked procedure."""

    # List of users to be excluded from check
    excludeUsers: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter=',', allow_none=True)})

    # Parse /etc/login.defs and skip users with uid below UID_MIN
    skip_below_uid_min: Optional[bool] = dataclasses.field(default=None)

    # Parse /etc/shells and if user does not have valid shell skip it
    skip_invalid_shells: Optional[bool] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureAllGroupsFromEtcPasswdExistInEtcGroup:
    """Parameters for the EnsureAllGroupsFromEtcPasswdExistInEtcGroup procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureApparmorProfiles:
    """Parameters for the EnsureApparmorProfiles procedure."""

    # Set for enforce (L2) mode, complain (L1) mode by default
    enforce: Optional[bool] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureDconf:
    """Parameters for the EnsureDconf procedure."""

    # dconf key name to be checked
    key: str = dataclasses.field(default=marshmallow.missing)

    # Value to be verified using the operation
    value: str = dataclasses.field(default=marshmallow.missing)

    # Type of operation, one of eq, ne
    # pattern: ^(eq|ne)$
    operation: DConfOperation = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(DConfOperation, by_value=True, allow_none=False)})


@marshmallow_dataclass.dataclass
class EnsureDefaultShellTimeoutIsConfigured:
    """Parameters for the EnsureDefaultShellTimeoutIsConfigured procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureDefaultUserUmaskIsConfigured:
    """Parameters for the EnsureDefaultUserUmaskIsConfigured procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureFileExists:
    """Parameters for the EnsureFileExists procedure."""

    # A filename containing to check for existence
    filename: str = dataclasses.field(default=marshmallow.missing)


@marshmallow_dataclass.dataclass
class EnsureFilePermissions:
    """Parameters for the EnsureFilePermissions procedure."""

    # Path to the file
    filename: str = dataclasses.field(default=marshmallow.missing)

    # Required owner of the file, single or | separated, first one is used for remediation
    owner: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter='|', allow_none=True)})

    # Required group of the file, single or | separated, first one is used for remediation
    group: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter='|', allow_none=True)})

    # Required octal permissions of the file
    # pattern: ^[0-7]{3,4}$
    permissions: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": OctalString(allow_none=True)})

    # Required octal permissions of the file - mask
    # pattern: ^[0-7]{3,4}$
    mask: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": OctalString(allow_none=True)})

    # Behavior when checking file existence
    behavior: Optional[Behavior] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(Behavior, by_value=True, allow_none=True)})


@marshmallow_dataclass.dataclass
class EnsureFilePermissionsCollection:
    """Parameters for the EnsureFilePermissionsCollection procedure."""

    # Directory path
    directory: str = dataclasses.field(default=marshmallow.missing)

    # Whether to recurse
    recurse: Optional[bool] = dataclasses.field(default=None)

    # File pattern
    ext: str = dataclasses.field(default=marshmallow.missing)

    # Required owner of the file, single or | separated, first one is used for remediation
    owner: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter='|', allow_none=True)})

    # Required group of the file, single or | separated, first one is used for remediation
    group: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter='|', allow_none=True)})

    # Required octal permissions of the file
    # pattern: ^[0-7]{3,4}$
    permissions: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": OctalString(allow_none=True)})

    # Required octal permissions of the file - mask
    # pattern: ^[0-7]{3,4}$
    mask: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": OctalString(allow_none=True)})

    # Behavior when checking file existence
    behavior: Optional[Behavior] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(Behavior, by_value=True, allow_none=True)})


@marshmallow_dataclass.dataclass
class EnsureFilesystemOption:
    """Parameters for the EnsureFilesystemOption procedure."""

    # Filesystem mount point
    mountpoint: str = dataclasses.field(default=marshmallow.missing)

    # Comma-separated list of options that must be set
    optionsSet: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter=',', allow_none=True)})

    # Comma-separated list of options that must not be set
    optionsNotSet: Optional[List[str]] = dataclasses.field(default=None, metadata={"marshmallow_field": SeparatedField(delimiter=',', allow_none=True)})

    # Location of the fstab file
    test_fstab: Optional[str] = dataclasses.field(default=None)

    # Location of the mtab file
    test_mtab: Optional[str] = dataclasses.field(default=None)

    # Location of the mount binary
    test_mount: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureFirewalldActiveZoneTargets:
    """Parameters for the EnsureFirewalldActiveZoneTargets procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureGroupIsOnlyGroupWith:
    """Parameters for the EnsureGroupIsOnlyGroupWith procedure."""

    # A pattern or value to match group names against
    group: str = dataclasses.field(default=marshmallow.missing)

    # A value to match the GID against
    # pattern: \d+
    gid: Optional[int] = dataclasses.field(default=None)

    # Alternative path to the /etc/group file to test against
    test_etcGroupPath: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureGsettings:
    """Parameters for the EnsureGsettings procedure."""

    # Name of the gsettings schema to get
    schema: str = dataclasses.field(default=marshmallow.missing)

    # Name of gsettings key to get
    key: str = dataclasses.field(default=marshmallow.missing)

    # Type of key, possible options string,number
    # pattern: ^(number|string)$
    keyType: GsettingsKeyType = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(GsettingsKeyType, by_value=True, allow_none=False)})

    # Type of operation to perform on variable one of eq, ne, lt, gt,is-unlocked
    # pattern: ^(eq|ne|lt|gt|is-unlocked)$
    operation: GsettingsOperationType = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(GsettingsOperationType, by_value=True, allow_none=False)})

    # Value of operation to check according to the operation
    value: str = dataclasses.field(default=marshmallow.missing)


@marshmallow_dataclass.dataclass
class EnsureInteractiveUsersDotFilesAccessIsConfigured:
    """Parameters for the EnsureInteractiveUsersDotFilesAccessIsConfigured procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureInteractiveUsersHomeDirectoriesAreConfigured:
    """Parameters for the EnsureInteractiveUsersHomeDirectoriesAreConfigured procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureIp6tablesOpenPorts:
    """Parameters for the EnsureIp6tablesOpenPorts procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureIptablesOpenPorts:
    """Parameters for the EnsureIptablesOpenPorts procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureKernelModuleUnavailable:
    """Parameters for the EnsureKernelModuleUnavailable procedure."""

    # Name of the kernel module
    moduleName: str = dataclasses.field(default=marshmallow.missing)


@marshmallow_dataclass.dataclass
class EnsureLogfileAccess:
    """Parameters for the EnsureLogfileAccess procedure."""

    # Path to log directory to check, default /var/log
    path: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureMTAsLocalOnly:
    """Parameters for the EnsureMTAsLocalOnly procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureMountPointExists:
    """Parameters for the EnsureMountPointExists procedure."""

    # Mount point to check
    mountPoint: str = dataclasses.field(default=marshmallow.missing)


@marshmallow_dataclass.dataclass
class EnsureNoDuplicateEntriesExist:
    """Parameters for the EnsureNoDuplicateEntriesExist procedure."""

    # The file to be checked for duplicate entries
    filename: str = dataclasses.field(default=marshmallow.missing)

    # A single character used to separate entries
    delimiter: str = dataclasses.field(default=marshmallow.missing)

    # Column index to check for duplicates
    column: int = dataclasses.field(default=marshmallow.missing)

    # Context for the entries used in the messages
    context: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureNoUnowned:
    """Parameters for the EnsureNoUnowned procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureNoUserHasPrimaryShadowGroup:
    """Parameters for the EnsureNoUserHasPrimaryShadowGroup procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureNoWritables:
    """Parameters for the EnsureNoWritables procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsurePasswordChangeIsInPast:
    """Parameters for the EnsurePasswordChangeIsInPast procedure."""

    # Path to the shadow file to test against
    test_etcShadowPath: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureRootPath:
    """Parameters for the EnsureRootPath procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureShadowContains:
    """Parameters for the EnsureShadowContains procedure."""

    # A pattern or value to match usernames against
    username: Optional[str] = dataclasses.field(default=None)

    # A comparison operation for the username parameter
    # pattern: ^(eq|ne|lt|le|gt|ge|match)$
    username_operation: Optional[ComparisonOperation] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(ComparisonOperation, by_value=True, allow_none=True)})

    # The /etc/shadow entry field to match against
    # pattern: ^(password|last_change|min_age|max_age|warn_period|inactivity_period|expiration_date|encryption_method)$
    field: Field = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(Field, by_value=True, allow_none=False)})

    # A pattern or value to match against the specified field
    value: str = dataclasses.field(default=marshmallow.missing)

    # A comparison operation for the value parameter
    # pattern: ^(eq|ne|lt|le|gt|ge|match)$
    operation: ComparisonOperation = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(ComparisonOperation, by_value=True, allow_none=False)})

    # Path to the /etc/shadow file to test against
    test_etcShadowPath: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureSshKeyPerms:
    """Parameters for the EnsureSshKeyPerms procedure."""

    # Key type - public or private
    # pattern: ^(public|private)$
    type: SshKeyType = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(SshKeyType, by_value=True, allow_none=False)})


@marshmallow_dataclass.dataclass
class EnsureSshdOption:
    """Parameters for the EnsureSshdOption procedure."""

    # Name of the SSH daemon option, might be a comma-separated list
    # pattern: ^[a-z0-9]+(,[a-z0-9]+)*$
    option: List[str] = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": SeparatedField(delimiter=',')})

    # One of Regex, list of regexes, string or integer threshold the option value is evaluated against
    value: str = dataclasses.field(default=marshmallow.missing)

    # (regex|match|not_match|lt|le|gt|ge) optional, defaults to 'regex'
    # pattern: ^(regex|match|not_match|lt|le|gt|ge)$
    op: Optional[EnsureSshdOptionOperation] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(EnsureSshdOptionOperation, by_value=True, allow_none=True)})

    # Mode, one of (regular|all_matches). Optional, defaults to 'regular'
    # pattern: ^(regular|all_matches)$
    mode: Optional[EnsureSshdOptionMode] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(EnsureSshdOptionMode, by_value=True, allow_none=True)})

    # Read extra configuration files in sysconfig and crypto policies
    readExtraConfigs: Optional[bool] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureSysctl:
    """Parameters for the EnsureSysctl procedure."""

    # Name of the sysctl
    # pattern: ^([a-zA-Z0-9_]+[\.a-zA-Z0-9_-]+)$
    sysctlName: str = dataclasses.field(default=marshmallow.missing)

    # Regex that the value of sysctl has to match
    value: str = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": PatternString()})


@marshmallow_dataclass.dataclass
class EnsureSystemAccountsDoNotHaveValidShell:
    """Parameters for the EnsureSystemAccountsDoNotHaveValidShell procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureUfwOpenPorts:
    """Parameters for the EnsureUfwOpenPorts procedure."""

    pass


@marshmallow_dataclass.dataclass
class EnsureUserIsOnlyAccountWith:
    """Parameters for the EnsureUserIsOnlyAccountWith procedure."""

    # A value to match usernames against
    username: str = dataclasses.field(default=marshmallow.missing)

    # A value to match the UID against
    # pattern: \d+
    uid: Optional[int] = dataclasses.field(default=None)

    # A value to match the GID against
    # pattern: \d+
    gid: Optional[int] = dataclasses.field(default=None)

    # Alternative path to the /etc/passwd file to test against
    test_etcPasswdPath: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureWirelessIsDisabled:
    """Parameters for the EnsureWirelessIsDisabled procedure."""

    # Optional path to the sysfs net class directory to test against
    test_sysfs_class_net: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class EnsureXdmcp:
    """Parameters for the EnsureXdmcp procedure."""

    pass


@marshmallow_dataclass.dataclass
class ExecuteCommandGrep:
    """Parameters for the ExecuteCommandGrep procedure."""

    # Command to be executed
    command: str = dataclasses.field(default=marshmallow.missing)

    # Awk transformation in the middle, optional
    awk: Optional[str] = dataclasses.field(default=None)

    # Regex to be matched
    regex: str = dataclasses.field(default=marshmallow.missing)

    # Type of regex, P for Perl (default) or E for Extended
    type: Optional[RegexType] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(RegexType, by_value=True, allow_none=True)})


@marshmallow_dataclass.dataclass
class FileRegexMatch:
    """Parameters for the FileRegexMatch procedure."""

    # A directory name containing files to check
    path: str = dataclasses.field(default=marshmallow.missing)

    # A pattern to match file names in the provided path
    filenamePattern: str = dataclasses.field(default=marshmallow.missing)

    # Operation to perform on the file contents
    # pattern: ^pattern match$
    matchOperation: Optional[Operation] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(Operation, by_value=True, allow_none=True)})

    # The pattern to match against the file contents
    matchPattern: str = dataclasses.field(default=marshmallow.missing)

    # Operation to perform on each line that matches the 'matchPattern'
    # pattern: ^pattern match$
    stateOperation: Optional[Operation] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(Operation, by_value=True, allow_none=True)})

    # The pattern to match against each line that matches the 'statePattern'
    statePattern: Optional[str] = dataclasses.field(default=None)

    # Determine whether a match or state should ignore case sensitivity 'matchPattern' and 'statePattern' or none when empty'
    # pattern: ^(matchPattern\sstatePattern|matchPattern|statePattern)$
    ignoreCase: Optional[IgnoreCase] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(IgnoreCase, by_value=True, allow_none=True)})

    # Determine the function behavior
    # pattern: ^(all_exist|any_exist|at_least_one_exists|none_exist|only_one_exists)$
    behavior: Optional[Behavior] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(Behavior, by_value=True, allow_none=True)})


@marshmallow_dataclass.dataclass
class LoginDefsOption:
    """Parameters for the LoginDefsOption procedure."""

    # The login.defs option name to check (e.g., PASS_MAX_DAYS, PASS_MIN_DAYS, PASS_WARN_AGE, ENCRYPT_METHOD)
    option: str = dataclasses.field(default=marshmallow.missing)

    # The expected value to compare against
    value: str = dataclasses.field(default=marshmallow.missing)

    # The comparison operation to apply
    # pattern: ^(eq|ne|lt|le|gt|ge)$
    comparison: ComparisonOperation = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": mf.Enum(ComparisonOperation, by_value=True, allow_none=False)})


@marshmallow_dataclass.dataclass
class PackageInstalled:
    """Parameters for the PackageInstalled procedure."""

    # Package name
    packageName: str = dataclasses.field(default=marshmallow.missing)

    # Minimum package version to check against (optional)
    minPackageVersion: Optional[str] = dataclasses.field(default=None)

    # Package manager, autodetected by default
    # pattern: ^(rpm|dpkg)$
    packageManager: Optional[PackageManagerType] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(PackageManagerType, by_value=True, allow_none=True)})

    # Cache path
    test_cachePath: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class RemediationFailure:
    """Parameters for the RemediationFailure procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class RemediationNotApplicable:
    """Parameters for the RemediationNotApplicable procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class RemediationParametrized:
    """Parameters for the RemediationParametrized procedure."""

    # Expected remediation result - success or failure
    # pattern: (success|failure)
    result: str = dataclasses.field(default=marshmallow.missing)


@marshmallow_dataclass.dataclass
class RemediationSuccess:
    """Parameters for the RemediationSuccess procedure."""

    # The message to be logged
    message: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class SCE:
    """Parameters for the SCE procedure."""

    # Script path
    scriptName: str = dataclasses.field(default=marshmallow.missing)

    # Environment as passed to the SCE script
    ENVIRONMENT: Optional[str] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class SystemdParameter:
    """Parameters for the SystemdParameter procedure."""

    # Parameter name
    parameter: str = dataclasses.field(default=marshmallow.missing)

    # Regex for the value, can be used instead of operator + value comparison
    valueRegex: Optional[str] = dataclasses.field(default=None)

    # Operator for the value
    op: Optional[SystemdParameterOperator] = dataclasses.field(default=None, metadata={"marshmallow_field": mf.Enum(SystemdParameterOperator, by_value=True, allow_none=True)})

    # Value to compare with
    value: Optional[str] = dataclasses.field(default=None)

    # Config filename
    file: Optional[str] = dataclasses.field(default=None)

    # Block in which the parameter is expected to be (e.g. [Unit], [Service], etc.)
    block: Optional[str] = dataclasses.field(default=None)

    # Directory to search for config files
    dir: Optional[str] = dataclasses.field(default=None)

    # If the value is not found return Compliant
    passOnNotFound: Optional[bool] = dataclasses.field(default=None)


@marshmallow_dataclass.dataclass
class SystemdUnitState:
    """Parameters for the SystemdUnitState procedure."""

    # Name of the systemd unit
    unitName: str = dataclasses.field(default=marshmallow.missing)

    # value of systemd ActiveState of unitName to match
    ActiveState: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": PatternString(allow_none=True)})

    # value of systemd LoadState of unitName to match
    LoadState: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": PatternString(allow_none=True)})

    # value of systemd UnitFileState of unitName to match
    UnitFileState: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": PatternString(allow_none=True)})

    # value of systemd property Unit, used in systemd.timer, name of unit to run when timer elapses
    Unit: Optional[str] = dataclasses.field(default=None, metadata={"marshmallow_field": PatternString(allow_none=True)})


@marshmallow_dataclass.dataclass
class UfwStatus:
    """Parameters for the UfwStatus procedure."""

    # Regex that the status must match
    statusRegex: str = dataclasses.field(default=marshmallow.missing, metadata={"marshmallow_field": PatternString()})


_PROCEDURE_REGISTRY: dict = {
    "AuditFailure": AuditFailure,
    "AuditGetParamValues": AuditGetParamValues,
    "AuditNotApplicable": AuditNotApplicable,
    "AuditSuccess": AuditSuccess,
    "AuditdRulesCheck": AuditdRulesCheck,
    "EnsureAccountsWithoutShellAreLocked": EnsureAccountsWithoutShellAreLocked,
    "EnsureAllGroupsFromEtcPasswdExistInEtcGroup": EnsureAllGroupsFromEtcPasswdExistInEtcGroup,
    "EnsureApparmorProfiles": EnsureApparmorProfiles,
    "EnsureDconf": EnsureDconf,
    "EnsureDefaultShellTimeoutIsConfigured": EnsureDefaultShellTimeoutIsConfigured,
    "EnsureDefaultUserUmaskIsConfigured": EnsureDefaultUserUmaskIsConfigured,
    "EnsureFileExists": EnsureFileExists,
    "EnsureFilePermissions": EnsureFilePermissions,
    "EnsureFilePermissionsCollection": EnsureFilePermissionsCollection,
    "EnsureFilesystemOption": EnsureFilesystemOption,
    "EnsureFirewalldActiveZoneTargets": EnsureFirewalldActiveZoneTargets,
    "EnsureGroupIsOnlyGroupWith": EnsureGroupIsOnlyGroupWith,
    "EnsureGsettings": EnsureGsettings,
    "EnsureInteractiveUsersDotFilesAccessIsConfigured": EnsureInteractiveUsersDotFilesAccessIsConfigured,
    "EnsureInteractiveUsersHomeDirectoriesAreConfigured": EnsureInteractiveUsersHomeDirectoriesAreConfigured,
    "EnsureIp6tablesOpenPorts": EnsureIp6tablesOpenPorts,
    "EnsureIptablesOpenPorts": EnsureIptablesOpenPorts,
    "EnsureKernelModuleUnavailable": EnsureKernelModuleUnavailable,
    "EnsureLogfileAccess": EnsureLogfileAccess,
    "EnsureMTAsLocalOnly": EnsureMTAsLocalOnly,
    "EnsureMountPointExists": EnsureMountPointExists,
    "EnsureNoDuplicateEntriesExist": EnsureNoDuplicateEntriesExist,
    "EnsureNoUnowned": EnsureNoUnowned,
    "EnsureNoUserHasPrimaryShadowGroup": EnsureNoUserHasPrimaryShadowGroup,
    "EnsureNoWritables": EnsureNoWritables,
    "EnsurePasswordChangeIsInPast": EnsurePasswordChangeIsInPast,
    "EnsureRootPath": EnsureRootPath,
    "EnsureShadowContains": EnsureShadowContains,
    "EnsureSshKeyPerms": EnsureSshKeyPerms,
    "EnsureSshdOption": EnsureSshdOption,
    "EnsureSysctl": EnsureSysctl,
    "EnsureSystemAccountsDoNotHaveValidShell": EnsureSystemAccountsDoNotHaveValidShell,
    "EnsureUfwOpenPorts": EnsureUfwOpenPorts,
    "EnsureUserIsOnlyAccountWith": EnsureUserIsOnlyAccountWith,
    "EnsureWirelessIsDisabled": EnsureWirelessIsDisabled,
    "EnsureXdmcp": EnsureXdmcp,
    "ExecuteCommandGrep": ExecuteCommandGrep,
    "FileRegexMatch": FileRegexMatch,
    "LoginDefsOption": LoginDefsOption,
    "PackageInstalled": PackageInstalled,
    "RemediationFailure": RemediationFailure,
    "RemediationNotApplicable": RemediationNotApplicable,
    "RemediationParametrized": RemediationParametrized,
    "RemediationSuccess": RemediationSuccess,
    "SCE": SCE,
    "SystemdParameter": SystemdParameter,
    "SystemdUnitState": SystemdUnitState,
    "UfwStatus": UfwStatus,
}

# ---------------------------------------------------------------------------
# Rule expression combinators
# ---------------------------------------------------------------------------


@dataclasses.dataclass
class LuaExpression:
    """Inline Lua script expression."""

    script: str


@dataclasses.dataclass
class AllOf:
    """Logical AND: all conditions must be compliant (short-circuit)."""

    conditions: List["RuleExpression"]


@dataclasses.dataclass
class AnyOf:
    """Logical OR: at least one condition must be compliant (short-circuit)."""

    conditions: List["RuleExpression"]


@dataclasses.dataclass
class Not:
    """Logical NOT: inverts the inner expression (audit-only, no remediation)."""

    expression: "RuleExpression"


# Union of all possible nodes in a rule expression tree
RuleExpression = Union[AllOf, AnyOf, Not, LuaExpression]


def serialize_expression(expr) -> dict:
    """Serialise a rule expression tree to a JSON-compatible dict.
    Typed procedure params classes are serialised by class name and Schema.dump()."""
    if isinstance(expr, AllOf):
        return {"allOf": [serialize_expression(e) for e in expr.conditions]}
    if isinstance(expr, AnyOf):
        return {"anyOf": [serialize_expression(e) for e in expr.conditions]}
    if isinstance(expr, Not):
        return {"not": serialize_expression(expr.expression)}
    if isinstance(expr, LuaExpression):
        return {"Lua": {"script": expr.script}}
    # Typed params class instance: serialise via its marshmallow Schema
    cls = type(expr)
    if not hasattr(cls, 'Schema'):
        raise TypeError(f"Cannot serialise {cls.__name__}: not a marshmallow dataclass")
    params = {k: v for k, v in cls.Schema().dump(expr).items() if v is not None}
    return {cls.__name__: params}


def deserialize_expression(data: dict) -> RuleExpression:
    """Deserialise a JSON-compatible dict into a rule expression tree."""
    if not isinstance(data, dict) or len(data) != 1:
        raise ValueError(f"Expected a dict with exactly one key, got: {data!r}")
    key, value = next(iter(data.items()))
    if key == "allOf":
        return AllOf(conditions=[deserialize_expression(e) for e in value])
    if key == "anyOf":
        return AnyOf(conditions=[deserialize_expression(e) for e in value])
    if key == "not":
        return Not(expression=deserialize_expression(value))
    if key == "Lua":
        return LuaExpression(script=value["script"])
    cls = _PROCEDURE_REGISTRY.get(key)
    if cls is None:
        raise ValueError(f"Unknown procedure name: {key!r}")
    return cls.Schema().load(value)
