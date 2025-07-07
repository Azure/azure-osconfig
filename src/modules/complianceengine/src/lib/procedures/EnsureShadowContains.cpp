#include <CommonUtils.h>
#include <Evaluator.h>
#include <Regex.h>
#include <ScopeGuard.h>
#include <shadow.h>
#include <vector>

using std::map;
using std::string;
using std::vector;

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

static const map<Field, string> fieldNamesMap = {
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

static const map<string, Operation> comparisonOperationMap = {
    {"eq", Operation::Equal},
    {"ne", Operation::NotEqual},
    {"lt", Operation::LessThan},
    {"le", Operation::LessOrEqual},
    {"gt", Operation::GreaterThan},
    {"ge", Operation::GreaterOrEqual},
    {"match", Operation::PatternMatch},
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

Result<Field> ParseFieldName(const string& field)
{
    auto it = fieldMap.find(field);
    if (it == fieldMap.end())
    {
        return Error("Invalid field name: " + field, EINVAL);
    }

    return it->second;
}

const string& PrettyFieldName(Field field)
{
    assert(fieldNamesMap.find(field) != fieldNamesMap.end());
    return fieldNamesMap.at(field);
}

Result<Operation> ParseOperation(const string& operation)
{
    auto it = comparisonOperationMap.find(operation);
    if (it == comparisonOperationMap.end())
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

    if (entry[0] == '_')
    {
        // If the password starts with '_', it is likely a legacy BSDi format
        return PasswordEncryptionMethod::BSDi;
    }

    if (entry[0] == '!' || entry[0] == '*')
    {
        // If the password starts with '!', it is locked
        // If it starts with '*', it is not set
        return PasswordEncryptionMethod::None; // No password set
    }

    if (entry[0] != '$')
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

Result<bool> CompareUserEntry(const spwd& entry, Field field, const string& value, Operation operation)
{
    switch (field)
    {
        case Field::Username:
            return Error("Username field comparison is not supported", EINVAL);
        case Field::Password:
            return StringComparison(value, entry.sp_pwdp, operation);
        case Field::EncryptionMethod: {
            OsConfigLogInfo(nullptr, "Comparing encryption methods: entry='%s', supplied='%s'", entry.sp_pwdp, value.c_str());
            if (operation != Operation::Equal && operation != Operation::NotEqual)
            {
                return Error("Unsupported comparison operation for encryption method", EINVAL);
            }

            OsConfigLogInfo(nullptr, "Comparing encryption methods: entry='%s', supplied='%s'", entry.sp_pwdp, value.c_str());
            auto suppliedMethod = ParseEncryptionMethod(value);
            if (!suppliedMethod.HasValue())
            {
                return suppliedMethod.Error();
            }

            OsConfigLogInfo(nullptr, "Supplied encryption method: '%d'", (int)suppliedMethod.Value());
            auto entryMethod = ParseEncryptionMethod(entry);
            if (!entryMethod.HasValue())
            {
                return entryMethod.Error();
            }

            OsConfigLogInfo(nullptr, "Comparing encryption methods: entry='%d', supplied='%d'", (int)entryMethod.Value(), (int)suppliedMethod.Value());
            if (operation == Operation::Equal)
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
    "operation:A comparison operation for the value parameter:M:(eq|ne|lt|le|gt|ge|match)",
    "test_etcShadowPath:Path to the /etc/shadow file to test against::/etc/shadow")
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

    auto etcShadowPath = string("/etc/shadow");
    it = args.find("test_etcShadowPath");
    if (it != args.end())
    {
        etcShadowPath = std::move(it->second);
    }

    auto stream = fopen(etcShadowPath.c_str(), "r");
    if (nullptr == stream)
    {
        OsConfigLogError(context.GetLogHandle(), "Failed to open /etc/shadow file: %s", strerror(errno));
        return Error("Failed to open /etc/shadow file: " + string(strerror(errno)), errno);
    }
    ScopeGuard closeGuard([stream]() { fclose(stream); });

    // Iterate over all users
    spwd fgetspentEntry;
    vector<char> fgetspentBuffer(1024);
    while (true)
    {
        spwd* entry = nullptr;
        // fgetspent_r return 0 on success, -1 and sets errno on failure
        auto status = fgetspent_r(stream, &fgetspentEntry, fgetspentBuffer.data(), fgetspentBuffer.size(), &entry);
        OsConfigLogInfo(context.GetLogHandle(), "fgetspent_r returned %d, entry %p", status, entry);
        if (0 != status || nullptr == entry)
        {
            status = errno;
            if (ERANGE == status)
            {
                OsConfigLogInfo(context.GetLogHandle(), "Buffer size too small for /etc/shadow entry, resizing to %zu bytes", fgetspentBuffer.size() * 2);
                fgetspentBuffer.resize(fgetspentBuffer.size() * 2);
                continue; // Retry with a larger buffer
            }

            if (ENOENT == status)
            {
                OsConfigLogDebug(context.GetLogHandle(), "End of /etc/shadow file reached.");
                break;
            }

            OsConfigLogInfo(context.GetLogHandle(), "Failed to read /etc/shadow entry: %s (%d)", strerror(status), status);
            return Error("Failed to read /etc/shadow entry: " + string(strerror(status)), status);
        }

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

        assert(field.HasValue());
        assert(operation.HasValue());
        OsConfigLogDebug(context.GetLogHandle(), "Checking user '%s' for %s field with value '%s' and operation '%d'.", entry->sp_namp,
            PrettyFieldName(field.Value()).c_str(), value.c_str(), (int)operation.Value());
        auto result = CompareUserEntry(*entry, field.Value(), value, operation.Value());
        if (!result.HasValue())
        {
            return result.Error();
        }
        if (!result.Value())
        {
            return indicators.NonCompliant(PrettyFieldName(field.Value()) + " does not match expected value for user '" + entry->sp_namp + "'");
        }

        if (username.HasValue())
        {
            indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for user '" + entry->sp_namp + "'");
        }
    }

    return indicators.Compliant(PrettyFieldName(field.Value()) + " matches expected value for all tested users");
}
} // namespace ComplianceEngine
