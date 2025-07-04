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
    MinAge,
    MaxAge,
    WarnPeriod,
    InactivityPeriod,
    ExpirationDate,
    Reserved,
    EncryptionMethod,
};

Result<Field> ParseFieldName(const string& field)
{
    static const map<string, Field> fieldMap = {
        {"username", Field::Username},
        {"password", Field::Password},
        {"last_change", Field::LastChange},
        {"min_age", Field::MinAge},
        {"max_age", Field::MaxAge},
        {"warn_period", Field::WarnPeriod},
        {"inactivity_period", Field::InactivityPeriod},
        {"expiration_date", Field::ExpirationDate},
        {"flag", Field::Reserved},
        {"encryption_method", Field::EncryptionMethod},
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
        {Field::MinAge, "minimum password age"},
        {Field::MaxAge, "maximum password age"},
        {Field::WarnPeriod, "password warning period"},
        {Field::InactivityPeriod, "password inactivity period"},
        {Field::ExpirationDate, "account expiration date"},
        {Field::Reserved, "reserved field"},
        {Field::EncryptionMethod, "password encryption method"},
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

Result<bool> StringComparison(const string& lhs, const string& rhs, Operation operation)
{
    switch (operation)
    {
        case Operation::PatternMatch:
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

    return ComplianceEngine::Error("Unsupported comparison operation for a string type", EINVAL);
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

    return ComplianceEngine::Error("Unsupported comparison operation for an integer type", EINVAL);
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
            return Error("Username field comparison is not supported", EINVAL);
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

namespace ComplianceEngine
{
AUDIT_FN(EnsureShadowContains, "username:A pattern or value to match usernames against",
    "username_operation:A comparison operation for the username parameter::(eq|ne|lt|le|gt|ge|match)",
    "field:The /etc/shadow entry field to match "
    "against:M:(password|last_change|min_age|max_age|warn_period|inactivity_period|expiration_date|encryption_method)",
    "value:A pattern or value to match against the specified field:M",
    "operation:A comparison operation for the value parameter:M:(eq|ne|lt|le|gt|ge|match)")
{
    UNUSED(context);

    Optional<string> username;
    auto it = args.find("username");
    if (it != args.end())
    {
        username = std::move(it->second);
    }

    auto usernameOperation = Operation::Equal;
    it = args.find("username_operation");
    if (it != args.end())
    {
        auto operation = ParseOperation(it->second);
        if (!operation.HasValue())
        {
            return operation.Error();
        }
        usernameOperation = operation.Value();
    }

    it = args.find("field");
    if (it == args.end())
    {
        return Error("Missing 'field' parameter", EINVAL);
    }
    auto field = ParseFieldName(it->second);
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

    // Iterate over all users
    setspent();
    std::unique_ptr<spwd, void (*)(spwd*)> endspentGuard(nullptr, [](spwd*) { endspent(); });

    spwd* entry = nullptr;
    while (nullptr != (entry = getspent()))
    {
        if (username.HasValue())
        {
            OsConfigLogDebug(context.GetLogHandle(), "Checking user '%s' for username match with '%s'.", entry->sp_namp, username.Value().c_str());
            auto result = StringComparison(username.Value(), entry->sp_namp, usernameOperation);
            if (!result.HasValue())
            {
                return result.Error();
            }
            if (!result.Value())
            {
                continue; // Skip this user if it does not match the specified username
            }
        }

        OsConfigLogDebug(context.GetLogHandle(), "Checking user '%s' for %s field with value '%s' and operation '%d'.", entry->sp_namp,
            PrettyFieldName(field.Value()).c_str(), value.c_str(), (int)operation.Value());
        auto result = CompareUserEntry(*entry, field.Value(), value, operation.Value());
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (!result.Value())
        {
            return indicators.NonCompliant(PrettyFieldName(field.Value()) + " does not match expected value for user '" + entry->sp_namp + "'.");
        }

        if (username.HasValue())
        {
            indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for user '" + entry->sp_namp + "'.");
        }
    }

    return indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for all tested users.");
}
} // namespace ComplianceEngine
