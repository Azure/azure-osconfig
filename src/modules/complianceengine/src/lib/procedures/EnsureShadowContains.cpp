#include <CommonUtils.h>
#include <EnsureShadowContains.h>
#include <Evaluator.h>
#include <PasswordEntriesIterator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <shadow.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

namespace ComplianceEngine
{
namespace
{
using ComplianceEngine::Error;
using ComplianceEngine::Result;

enum PasswordEncryptionMethod
{
    DES,
    BSDi,
    MD5,
    Blowfish,
    SHA256,
    SHA512,
    YesCrypt,
    None, // Used for entries without a password
};

static const map<Field, string> fieldNamesMap = {
    {Field::Username, "login name"},
    {Field::Password, "encrypted password"},
    {Field::LastChange, "last password change date"},
    {Field::MinAge, "minimum password age"},
    {Field::MaxAge, "maximum password age"},
    {Field::WarnPeriod, "password warning period"},
    {Field::InactivityPeriod, "password inactivity period"},
    {Field::ExpirationDate, "account expiration date"},
    {Field::Reserved, "reserved"},
    {Field::EncryptionMethod, "password encryption method"},
};

// Follows the OVAL specification, adds YesCrypt for future reference
// https://oval.mitre.org/language/version5.10/ovaldefinition/documentation/unix-definitions-schema.html#EntityStateEncryptMethodType
static const map<string, PasswordEncryptionMethod> encryptionTypeMap = {
    {"DES", PasswordEncryptionMethod::DES},
    {"BSDi", PasswordEncryptionMethod::BSDi},
    {"MD5", PasswordEncryptionMethod::MD5},
    {"Blowfish", PasswordEncryptionMethod::Blowfish},
    {"Sun MD5", PasswordEncryptionMethod::MD5},
    {"SHA-256", PasswordEncryptionMethod::SHA256},
    {"SHA-512", PasswordEncryptionMethod::SHA512},
    // Not defined in OVAL, but commonly used
    {"YesCrypt", PasswordEncryptionMethod::YesCrypt},

    // Allows to test against no pasword, e.g. !/*, used in tests
    {"None", PasswordEncryptionMethod::None},
};

static const map<string, PasswordEncryptionMethod> encryptionMethodMap = {
    {"1", PasswordEncryptionMethod::MD5},
    {"2", PasswordEncryptionMethod::Blowfish},
    {"2a", PasswordEncryptionMethod::Blowfish},
    {"2y", PasswordEncryptionMethod::Blowfish},
    {"md5", PasswordEncryptionMethod::MD5},
    {"5", PasswordEncryptionMethod::SHA256},
    {"6", PasswordEncryptionMethod::SHA512},
    {"y", PasswordEncryptionMethod::YesCrypt},
};

const string& PrettyFieldName(Field field)
{
    assert(fieldNamesMap.find(field) != fieldNamesMap.end());
    return fieldNamesMap.at(field);
}

Result<int> AsInt(const string& value)
{
    try
    {
        return std::stoi(value);
    }
    catch (const std::invalid_argument&)
    {
        return Error("Invalid integer value: " + value, EINVAL);
    }
    catch (const std::out_of_range&)
    {
        return Error("Integer value out of range: " + value, ERANGE);
    }
}

Result<bool> StringComparison(const string& lhs, const string& rhs, ComparisonOperation operation)
{
    switch (operation)
    {
        case ComparisonOperation::PatternMatch:
            try
            {
                OsConfigLogDebug(nullptr, "Performing regex match: '%s' against '%s'", lhs.c_str(), rhs.c_str());
                return regex_search(rhs, regex(lhs));
            }
            catch (const std::exception& e)
            {
                return Error("Pattern match failed: " + string(e.what()), EINVAL);
            }
            break;
        case ComparisonOperation::Equal:
            return lhs == rhs;
        case ComparisonOperation::NotEqual:
            return lhs != rhs;
        case ComparisonOperation::LessThan:
            return lhs < rhs;
        case ComparisonOperation::LessOrEqual:
            return lhs <= rhs;
        case ComparisonOperation::GreaterThan:
            return lhs > rhs;
        case ComparisonOperation::GreaterOrEqual:
            return lhs >= rhs;
        default:
            break;
    }

    return ComplianceEngine::Error("Unsupported comparison operation for a string type", EINVAL);
}

Result<bool> IntegerComparison(const int lhs, const int rhs, ComparisonOperation operation)
{
    switch (operation)
    {
        case ComparisonOperation::Equal:
            return lhs == rhs;
        case ComparisonOperation::NotEqual:
            return lhs != rhs;
        case ComparisonOperation::LessThan:
            return lhs < rhs;
        case ComparisonOperation::LessOrEqual:
            return lhs <= rhs;
        case ComparisonOperation::GreaterThan:
            return lhs > rhs;
        case ComparisonOperation::GreaterOrEqual:
            return lhs >= rhs;
        default:
            break;
    }

    return ComplianceEngine::Error("Unsupported comparison operation for an integer type", EINVAL);
}

Result<PasswordEncryptionMethod> ParseEncryptionMethod(const string& method)
{
    auto it = encryptionTypeMap.find(method);
    if (it == encryptionTypeMap.end())
    {
        return Error("Invalid encryption method: " + method, EINVAL);
    }

    return it->second;
}

Result<PasswordEncryptionMethod> ParseEncryptionMethod(const spwd& shadowEntry)
{
    if (nullptr == shadowEntry.sp_pwdp)
    {
        return Error("Shadow entry does not contain a valid password field", EINVAL);
    }

    const string entry = shadowEntry.sp_pwdp;
    if (entry.empty())
    {
        return PasswordEncryptionMethod::None; // No password set
    }

    if ('_' == entry[0])
    {
        // If the password starts with '_', it is likely a legacy BSDi format
        return PasswordEncryptionMethod::BSDi;
    }

    if (('!' == entry[0]) || ('*' == entry[0]))
    {
        // If the password starts with '!', it is locked
        // If it starts with '*', it is not set
        return PasswordEncryptionMethod::None; // No password set
    }

    if ('$' != entry[0])
    {
        // If the password does not start with a '$', it is likely a legacy DES format
        return PasswordEncryptionMethod::DES;
    }

    auto index = entry.find('$', 1);
    if (index == string::npos)
    {
        return Error("Invalid password format in shadow entry", EINVAL);
    }

    const auto prefix = entry.substr(1, index - 1); // Extract the prefix between the first and second '$'
    const auto it = encryptionMethodMap.find(prefix);
    if (it == encryptionMethodMap.end())
    {
        return Error("Unsupported password encryption method: " + prefix, EINVAL);
    }

    return it->second;
}

Result<bool> CompareUserEntry(const spwd& entry, Field field, const string& value, ComparisonOperation operation)
{
    switch (field)
    {
        case Field::Username:
            return Error("Username field comparison is not supported", EINVAL);
        case Field::Password:
            return StringComparison(value, entry.sp_pwdp, operation);
        case Field::EncryptionMethod: {
            if (operation != ComparisonOperation::Equal && operation != ComparisonOperation::NotEqual)
            {
                return Error("Unsupported comparison operation for encryption method", EINVAL);
            }

            auto suppliedMethod = ParseEncryptionMethod(value);
            if (!suppliedMethod.HasValue())
            {
                return suppliedMethod.Error();
            }

            auto entryMethod = ParseEncryptionMethod(entry);
            if (!entryMethod.HasValue())
            {
                return entryMethod.Error();
            }

            if (operation == ComparisonOperation::Equal)
            {
                return entryMethod.Value() == suppliedMethod.Value();
            }

            // NotEqual
            return entryMethod.Value() != suppliedMethod.Value();
        }
        default:
            break;
    }

    auto intValue = AsInt(value);
    if (!intValue.HasValue())
    {
        return Error("invalid " + PrettyFieldName(field) + " parameter value", EINVAL);
    }

    switch (field)
    {
        case Field::LastChange:
            return IntegerComparison(entry.sp_lstchg, intValue.Value(), operation);
        case Field::MinAge:
            return IntegerComparison(entry.sp_min, intValue.Value(), operation);
        case Field::MaxAge:
            return IntegerComparison(entry.sp_max, intValue.Value(), operation);
        case Field::WarnPeriod:
            return IntegerComparison(entry.sp_warn, intValue.Value(), operation);
        case Field::InactivityPeriod:
            return IntegerComparison(entry.sp_inact, intValue.Value(), operation);
        case Field::ExpirationDate:
            return IntegerComparison(entry.sp_expire, intValue.Value(), operation);
        default:
            break;
    }

    return Error(PrettyFieldName(field) + " field comparison is not supported", EINVAL);
}
} // anonymous namespace

Result<Status> AuditEnsureShadowContains(const EnsureShadowContainsParams& params, IndicatorsTree& indicators, ContextInterface& context)
{
    assert(params.test_etcShadowPath.HasValue());
    assert(params.username_operation.HasValue());
    auto range = PasswordEntryRange::Make(params.test_etcShadowPath.Value(), context.GetLogHandle());
    if (!range.HasValue())
    {
        return range.Error();
    }

    for (const auto& entry : range.Value())
    {
        if (params.username.HasValue())
        {
            OsConfigLogInfo(context.GetLogHandle(), "Checking user '%s' for username match with '%s'.", entry.sp_namp, params.username.Value().c_str());
            auto result = StringComparison(params.username.Value(), entry.sp_namp, params.username_operation.Value());
            if (!result.HasValue())
            {
                return result.Error();
            }
            if (!result.Value())
            {
                continue; // Skip this user if it does not match the specified username
            }
        }

        OsConfigLogInfo(context.GetLogHandle(), "Checking user '%s' for %s field with value '%s' and operation '%d'.", entry.sp_namp,
            PrettyFieldName(params.field).c_str(), params.value.c_str(), (int)params.operation);
        auto result = CompareUserEntry(entry, params.field, params.value, params.operation);
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (!result.Value())
        {
            return indicators.NonCompliant(PrettyFieldName(params.field) + " does not match expected value for user '" + entry.sp_namp + "'");
        }

        if (params.username.HasValue())
        {
            indicators.Compliant(PrettyFieldName(params.field) + " matches expected value for user '" + entry.sp_namp + "'");
        }
    }

    return indicators.Compliant(PrettyFieldName(params.field) + " matches expected value for all tested users");
}
} // namespace ComplianceEngine
