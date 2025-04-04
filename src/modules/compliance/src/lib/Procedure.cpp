// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Procedure.h"

#include "Logging.h"

#include <cassert>
#include <parson.h>

namespace compliance
{
namespace
{
std::size_t SkipSpaces(const std::string& input, std::size_t pos)
{
    while (pos < input.size() && isspace(input[pos]))
    {
        ++pos;
    }
    return pos;
}

Result<std::size_t> ParseKey(const std::string& input, std::size_t pos)
{
    bool first = true;
    while (pos < input.size() && !isspace(input[pos]) && input[pos] != '=')
    {
        if (islower(input[pos]))
        {
            return Error("Invalid key: must be uppercase");
        }

        if (!isalnum(input[pos]) && input[pos] != '_')
        {
            return Error("Invalid key: only alphanumeric and underscore characters are allowed");
        }

        if (first && isdigit(input[pos]))
        {
            return Error("Invalid key: first character must be an uppercase letter");
        }
        first = false;
        ++pos;
    }

    return pos;
}

std::size_t ParseQuotedValue(const std::string& input, std::size_t pos, std::string& value)
{
    // Assuming the first character is a quote
    // pos is always > 1 as we are passing value which must follow a valid key, assignment character and a quote
    assert(pos > 1);
    assert(input[pos] == '\'');
    assert(input[pos - 1] == '=');

    while (++pos < input.size())
    {
        if (input[pos] != '\'')
        {
            // Regular character, move on
            value += input[pos];
            continue;
        }

        // At the quote, look back to see if it is escaped
        if (input[pos - 1] != '\\')
        {
            // Not escaped, end of the value
            return ++pos;
        }

        // Found a backslash before the quote, check if there is another one before it
        if (input[pos - 2] == '\\')
        {
            // Found an escaped backslash, end of the value. Drop the last backslash as it was used to escape the quote
            value.pop_back();
            return ++pos;
        }

        // Found an escaped quote, add it to the value, but in place of the last backslash
        value.pop_back();
        value += input[pos];
    }

    return pos;
}
} // namespace

void Procedure::SetParameter(const std::string& key, std::string value)
{
    mParameters[key] = std::move(value);
}

Optional<Error> Procedure::UpdateUserParameters(const std::string& input)
{
    size_t pos = 0;
    while (pos < input.size())
    {
        // Skip spaces
        pos = SkipSpaces(input, pos);
        if (pos >= input.size())
        {
            break;
        }

        // Parse key
        const size_t keyStart = pos;
        auto result = ParseKey(input, pos);
        if (!result.HasValue())
        {
            return result.Error();
        }

        auto key = input.substr(keyStart, result.Value() - keyStart);
        pos = SkipSpaces(input, result.Value());
        if ((pos >= input.size()) || (input[pos] != '='))
        {
            return Error("Invalid key-value pair: '=' expected");
        }

        // Skip space after assignment character
        pos = SkipSpaces(input, pos + 1);
        if (pos >= input.size())
        {
            return Error("Invalid key-value pair: value expected");
        }

        // Parse value
        std::string value;
        if (input[pos] == '\'')
        {
            pos = ParseQuotedValue(input, pos, value);
            if ((pos >= input.size()) && ((input[pos - 1] != '\'') || (input[pos - 2] == '=')))
            {
                return Error("Invalid key-value pair: missing closing quote");
            }
        }
        else
        {
            // Unquoted value
            const size_t valueStart = pos;
            while (pos < input.size() && !isspace(input[pos]))
            {
                pos++;
            }
            value = input.substr(valueStart, pos - valueStart);
        }

        auto it = mParameters.find(key);
        if (it == mParameters.end())
        {
            return Error(std::string("User parameter '") + key + std::string("' not found"));
        }

        it->second = value;
    }

    return Optional<Error>();
}

Optional<Error> Procedure::SetAudit(const json_value_t* rule)
{
    if (mAuditRule != nullptr)
    {
        return Error("Audit rule already set");
    }

    mAuditRule.reset(json_value_deep_copy(rule));
    return Optional<Error>();
}

const JSON_Object* Procedure::Audit() const noexcept
{
    if (mAuditRule == nullptr)
    {
        return nullptr;
    }
    return json_value_get_object(mAuditRule.get());
}

Optional<Error> Procedure::SetRemediation(const json_value_t* rule)
{
    if (mRemediationRule != nullptr)
    {
        return Error("Remediation rule already set");
    }

    mRemediationRule.reset(json_value_deep_copy(rule));
    return Optional<Error>();
}

const JSON_Object* Procedure::Remediation() const noexcept
{
    if (mRemediationRule == nullptr)
    {
        return nullptr;
    }
    return json_value_get_object(mRemediationRule.get());
}
} // namespace compliance
