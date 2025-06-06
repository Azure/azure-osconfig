// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Base64.h"

#include "Result.h"

#include <string>

namespace ComplianceEngine
{
static inline char Base64Char(const unsigned char c)
{
    int cc = c;
    if (c >= 'A' && c <= 'Z')
    {
        return (char)(cc - 'A');
    }
    else if (c >= 'a' && c <= 'z')
    {
        return (char)(cc - 'a' + 26);
    }
    else if (c >= '0' && c <= '9')
    {
        return (char)(c - '0' + 52);
    }
    else if (c == '+')
    {
        return (char)62;
    }
    else if (c == '/')
    {
        return (char)63;
    }
    else
    {
        return 0;
    }
}

static inline bool IsBase64(const unsigned char c)
{
    return (isalnum(c) || (c == '+') || (c == '/') || (c == '='));
}

Result<std::string> Base64Decode(const std::string& input)
{
    if (input.size() % 4 != 0)
    {
        return Error("Invalid base64 length", EINVAL);
    }
    for (const char c : input)
    {
        if (!IsBase64(c))
        {
            return Error("Invalid base64 character", EINVAL);
        }
    }

    std::string ret;
    ret.reserve((input.size() * 3) / 4);

    for (size_t i = 0; i < input.size(); i += 4)
    {
        char enc[4] = {0};

        int j = 0;
        for (j = 0; j < 4; j++)
        {
            if (input[i + j] == '=')
            {
                break;
            }
            enc[j] = Base64Char(input[i + j]);
        }

        if (j == 4)
        {
            ret += (char)((enc[0] << 2) | (enc[1] >> 4));
            ret += (char)(((enc[1] & (char)0x0f) << 4) | (enc[2] >> 2));
            ret += (char)(((enc[2] & (char)0x03) << 6) | enc[3]);
        }
        else if (j == 3)
        {
            ret += (char)((enc[0] << 2) | (enc[1] >> 4));
            ret += (char)(((enc[1] & (char)0x0f) << 4) | (enc[2] >> 2));
        }
        else if (j == 2)
        {
            ret += (char)((enc[0] << 2) | (enc[1] >> 4));
        }
        else
        {
            return Error("Invalid base64", EINVAL);
        }
    }
    return ret;
}
} // namespace ComplianceEngine
