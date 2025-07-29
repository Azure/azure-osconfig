// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_BINDINGS_H
#define COMPLIANCEENGINE_BINDINGS_H

#include <CommonUtils.h>
#include <Optional.h>
#include <Regex.h>
#include <Result.h>
#include <StringTools.h>
#include <iostream>
#include <set>
#include <type_traits>
namespace ComplianceEngine
{
template <typename...>
struct make_void
{
    typedef void type;
};

// Missing in C++ 11
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

// Checks whether type T has a static method GetBindings
template <typename T, typename = void>
struct HasBindings : std::false_type
{
};

// Checks whether type T has a static method GetBindings
template <typename T>
struct HasBindings<T, void_t<decltype(T::GetBindings())>> : std::true_type
{
};

// Parsing specialization for int
inline Optional<Error> ParseValue(const std::map<std::string, std::string>& args, const std::string& key, int& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    auto result = TryStringToInt(it->second);
    if (!result.HasValue())
    {
        return result.Error();
    }

    output = std::move(result).Value();
    return Optional<Error>();
}

// Parsing specialization for std::string
inline Optional<Error> ParseValue(const std::map<std::string, std::string>& args, const std::string& key, std::string& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    output = it->second;
    return Optional<Error>();
}

// Parsing specialization for regex
inline Optional<Error> ParseValue(const std::map<std::string, std::string>& args, const std::string& key, regex& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    try
    {
        output = regex(it->second);
    }
    catch (const std::exception& e)
    {
        return Error("Regular expression '" + it->second + "' compilation failed: " + e.what());
    }

    return Optional<Error>();
}

// Parsing specialization for int
template <typename T>
inline Optional<Error> ParseValue(const std::map<std::string, std::string>& args, const std::string& key, Optional<T>& output)
{
    std::cerr << "[" << __func__ << ":" << __LINE__ << "] "
              << "key: " << key << std::endl;
    auto it = args.find(key);
    if (it == args.end())
    {
        return Optional<Error>();
    }

    T value;
    auto error = ParseValue(args, key, value);
    if (error.HasValue())
    {
        return error;
    }

    output = std::move(value);
    return Optional<Error>();
}

template <typename T, typename... Args>
struct Bindings;

// Primary template for zero arguments
template <typename T>
struct Bindings<T>
{
    // Empty specialization for no parameters
    Optional<Error> ParseArguments(T& result, const std::map<std::string, std::string>& args, std::set<std::string>::const_iterator field) const
    {
        UNUSED(result);
        UNUSED(args);
        UNUSED(field);
        return Optional<Error>();
    }
    static constexpr std::size_t Size()
    {
        return 0;
    }
};

// Partial specialization for one or more arguments
template <typename T, typename Arg1, typename... Args>
struct Bindings<T, Arg1, Args...>
{
    Arg1 T::*member;
    Bindings<T, Args...> other;

    Bindings(Arg1 T::*&& arg1, Args T::*&&... otherArgs)
        : member{std::move(arg1)},
          other{std::move(otherArgs)...}
    {
    }

    Optional<Error> ParseArguments(T& result, const std::map<std::string, std::string>& args, std::set<std::string>::const_iterator field) const
    {
        std::cerr << "[" << __func__ << ":" << __LINE__ << "] " << std::endl;
        auto error = ParseValue(args, *field, result.*member);
        if (error.HasValue())
        {
            return error.Value();
        }

        // Recurse to the rest of the arguments
        error = other.ParseArguments(result, args, ++field);
        if (error.HasValue())
        {
            return error.Value();
        }

        // No error
        return Optional<Error>();
    }

    static constexpr std::size_t Size()
    {
        return 1 + Bindings<T, Args...>::Size();
    }
};

template <typename T, typename... Args>
Result<T> ParseArguments(const std::map<std::string, std::string>& args, const std::set<std::string>& fields)
{
    static_assert(HasBindings<T>::value, "The parameters object must provide a GetBinding() static method");

    static const auto bindings = T::GetBindings();
    if (fields.size() != bindings.Size())
    {
        return Error("Fileds list size mismatch: " + std::to_string(fields.size()) + ", " + std::to_string(sizeof...(Args)), EINVAL);
    }

    for (const auto& arg : args)
    {
        if (fields.end() == fields.find(arg.first))
        {
            return Error("Unknown parameter '" + arg.first + "'", EINVAL);
        }
    }

    T result;
    auto error = bindings.ParseArguments(result, args, fields.cbegin());
    if (error.HasValue())
    {
        return error.Value();
    }

    return result;
}

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_BINDINGS_H
