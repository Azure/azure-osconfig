#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <shadow.h>

using std::map;
using std::string;

namespace
{
using ComplianceEngine::Error;
using ComplianceEngine::Result;

enum class Field
{
    Username,
    Password,
    LastChange,
    Minimum,
    Maximum,
    Warn,
    Inactive,
    Expire,
    Reserved,
    EncryptMethod,
};

Result<Field> ParseField(const string& field)
{
    static const map<string, Field> fieldMap = {
        {"username", Field::Username},
        {"password", Field::Password},
        {"chg_lst", Field::LastChange},
        {"chg_allow", Field::Minimum},
        {"chg_req", Field::Maximum},
        {"exp_warn", Field::Warn},
        {"exp_inact", Field::Inactive},
        {"exp_date", Field::Expire},
        {"flag", Field::Reserved},
        {"encrypt_method", Field::EncryptMethod},
    };

    auto it = fieldMap.find(field);
    if (it == fieldMap.end())
    {
        return Error("Invalid field name: " + field, EINVAL);
    }

    return it->second;
}

const std::string& PrettyFieldName(Field field)
{
    static const map<Field, string> fieldNames = {
        {Field::Username, "login name"},
        {Field::Password, "encrypted password"},
        {Field::LastChange, "last password change date"},
        {Field::Minimum, "minimum password age"},
        {Field::Maximum, "maximum password age"},
        {Field::Warn, "password warning period"},
        {Field::Inactive, "password inactivity period"},
        {Field::Expire, "account expiration date"},
        {Field::Reserved, "reserved field"},
        {Field::EncryptMethod, "password encryption method"},
    };

    assert(fieldNames.find(field) != fieldNames.end());
    return fieldNames.at(field);
}

enum class Operation
{
    Equal,
    NotEqual,
    LessThan,
    LessOrEqual,
    GreaterThan,
    GreaterOrEqual,
    PatternMatch,
};

Result<Operation> ParseOperation(const string& operation)
{
    static const map<string, Operation> operationMap = {
        {"eq", Operation::Equal},
        {"ne", Operation::NotEqual},
        {"lt", Operation::LessThan},
        {"le", Operation::LessOrEqual},
        {"gt", Operation::GreaterThan},
        {"ge", Operation::GreaterOrEqual},
        {"match", Operation::PatternMatch},
    };

    auto it = operationMap.find(operation);
    if (it == operationMap.end())
    {
        return Error("Invalid operation: '" + operation + "'", EINVAL);
    }

    return std::move(it->second);
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

Result<bool> StringComparison(const string& pattern, const string& value, Operation operation)
{
    switch (operation)
    {
        case Operation::PatternMatch:
            try
            {
                OsConfigLogInfo(nullptr, "Matching value '%s' against pattern '%s'.", value.c_str(), pattern.c_str());
                return regex_search(value, regex(pattern));
            }
            catch (const std::exception& e)
            {
                return Error("Pattern match failed: " + string(e.what()), EINVAL);
            }
            break;
        default:
            break;
    }

    return ComplianceEngine::Error("Unsupported operation for string comparison", EINVAL);
}

Result<bool> IntegerComparison(const int lhs, const int rhs, Operation operation)
{
    switch (operation)
    {
        case Operation::Equal:
            return lhs == rhs;
        case Operation::NotEqual:
            return lhs != rhs;
        case Operation::LessThan:
            return lhs < rhs;
        case Operation::LessOrEqual:
            return lhs <= rhs;
        case Operation::GreaterThan:
            return lhs > rhs;
        case Operation::GreaterOrEqual:
            return lhs >= rhs;
        default:
            break;
    }

    return ComplianceEngine::Error("Unsupported operation for integer comparison", EINVAL);
}

// Result<bool> GenericComparison(const string& value, const string& expected, Operation operation)
// {
//     auto lhs = AsInt(value);
//     if (!lhs.HasValue())
//     {
//         return StringComparison(value, expected, operation);
//     }

//     auto rhs = AsInt(expected);
//     if (!rhs.HasValue())
//     {
//         return StringComparison(expected, value, operation);
//     }

//     return IntegerComparison(lhs.Value(), rhs.Value(), operation);
// }

Result<bool> CompareUserEntry(const spwd& entry, Field field, const string& value, Operation operation)
{
    switch (field)
    {
        case Field::Username:
            return StringComparison(value, entry.sp_namp, operation);
        case Field::Password:
            return StringComparison(value, entry.sp_pwdp, operation);
        default:
            break;
    }

    auto intValue = AsInt(value);
    if (!intValue.HasValue())
    {
        return Error("Invalid integer value: " + value, EINVAL);
    }

    switch (field)
    {
        case Field::LastChange:
            return IntegerComparison(entry.sp_lstchg, intValue.Value(), operation);
        case Field::Minimum:
            return IntegerComparison(entry.sp_min, intValue.Value(), operation);
        case Field::Maximum:
            return IntegerComparison(entry.sp_max, intValue.Value(), operation);
        case Field::Warn:
            return IntegerComparison(entry.sp_warn, intValue.Value(), operation);
        case Field::Inactive:
            return IntegerComparison(entry.sp_inact, intValue.Value(), operation);
        case Field::Expire:
            return IntegerComparison(entry.sp_expire, intValue.Value(), operation);
        default:
            break;
    }

    return Error("Unsupported field for comparison", EINVAL);
}
} // anonymous namespace

namespace ComplianceEngine
{
AUDIT_FN(EnsureShadowContains)
{
    UNUSED(context);

    Optional<string> username;
    auto it = args.find("username");
    if (it != args.end())
    {
        username = std::move(it->second);
    }

    it = args.find("field");
    if (it == args.end())
    {
        return Error("Missing 'field' parameter", EINVAL);
    }
    auto field = ParseField(it->second);
    if (!field.HasValue())
    {
        return field.Error();
    }

    it = args.find("value");
    if (it == args.end())
    {
        return Error("Missing 'value' parameter", EINVAL);
    }
    auto value = std::move(it->second);

    it = args.find("operation");
    if (it == args.end())
    {
        return Error("Missing 'operation' parameter", EINVAL);
    }
    auto operation = ParseOperation(it->second);
    if (!operation.HasValue())
    {
        return operation.Error();
    }

    assert(field.HasValue());
    assert(operation.HasValue());
    if (username.HasValue())
    {
        const auto* entry = getspnam(username.Value().c_str());
        if (nullptr == entry)
        {
            OsConfigLogInfo(context.GetLogHandle(), "User '%s' not found in shadow file.", username.Value().c_str());
            return Error("User '" + username.Value() + "' not found in shadow file", ENOENT);
        }

        OsConfigLogInfo(context.GetLogHandle(), "Checking user '%s' for %s field with value '%s' and operation '%d'.", entry->sp_namp,
            PrettyFieldName(field.Value()).c_str(), value.c_str(), (int)operation.Value());
        auto result = CompareUserEntry(*entry, field.Value(), value, operation.Value());
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (!result.Value())
        {
            OsConfigLogInfo(context.GetLogHandle(), "User '%s' does not match expected value for field '%s'.", entry->sp_namp,
                PrettyFieldName(field.Value()).c_str());
            return indicators.NonCompliant(PrettyFieldName(field.Value()) + " does not match expected value for user '" + entry->sp_namp + "'.");
        }

        OsConfigLogInfo(context.GetLogHandle(), "User '%s' matches expected value for field '%s'.", entry->sp_namp, PrettyFieldName(field.Value()).c_str());
        return indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for user '" + entry->sp_namp + "'.");
    }

    setspent();
    std::unique_ptr<spwd, void (*)(spwd*)> endspentGuard(nullptr, [](spwd*) { endspent(); });

    spwd* entry = nullptr;
    while ((entry = getspent()) != nullptr)
    {
        auto result = CompareUserEntry(*entry, field.Value(), value, operation.Value());
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (!result.Value())
        {
            return indicators.NonCompliant(PrettyFieldName(field.Value()) + " does not match expected value for user '" + entry->sp_namp + "'.");
        }
    }

    return indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for user '" + entry->sp_namp + "'.");
}
} // namespace ComplianceEngine
