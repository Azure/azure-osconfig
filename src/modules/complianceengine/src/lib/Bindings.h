// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_BINDINGS_H
#define COMPLIANCEENGINE_BINDINGS_H

#include <BindingParsers.h>
#include <CommonUtils.h>
#include <Evaluator.h>
#include <Optional.h>
#include <ProcedureMap.h>
#include <Regex.h>
#include <Result.h>
#include <Separated.h>
#include <array>
#include <set>
#include <type_traits>

namespace ComplianceEngine
{

// Each procedure provides specialization for this structure
template <typename Params>
struct Bindings;

template <typename Enum>
const std::map<std::string, Enum>& MapEnum();

namespace BindingsImpl
{
template <typename...>
struct make_void
{
    typedef void type;
};

// Missing in C++ 11
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <typename T, typename = void>
struct IsSeparatedType : std::false_type
{
};
template <typename T>
struct IsSeparatedType<T, void_t<decltype(T::separator)>> : std::true_type
{
};

// Parses a single parameter value. The output value must
// be put as output mutable reference to determine the type of
// the parameter at compliation time.
// template <typename T>
// Optional<Error> ParseValue(const std::string& input, T& output);

template <typename T>
constexpr bool IsEnum()
{
    return std::is_enum<T>::value;
}

template <typename T>
constexpr bool IsSeparated()
{
    return IsSeparatedType<T>::value;
}

template <typename T>
constexpr bool IsBuiltin()
{
    return !(IsEnum<T>() || IsSeparated<T>());
}

// Parse an enumeration parameter
template <typename T>
Optional<Error> ParseValue(const std::string& input, typename std::enable_if<IsEnum<T>(), T>::type& output)
{
    static const auto enumMap = MapEnum<T>();
    const auto enumIt = enumMap.find(input);
    if (enumIt == enumMap.end())
    {
        return Error("Invalid value '" + input + "' for enumeration parameter", EINVAL);
    }

    output = enumIt->second;
    return Optional<Error>();
}

// Parse a separated string parameter
template <typename T>
Optional<Error> ParseValue(const std::string& input, typename std::enable_if<IsSeparated<T>(), T>::type& output)
{
    using ItemType = typename T::ItemType;
    static_assert(IsBuiltin<ItemType>(), "Item of Separated<ItemType, Separator> must be a built-in type");

    auto result = T::Parse(input);
    if (!result.HasValue())
    {
        return result.Error();
    }

    output = result.Value();
    return Optional<Error>();
}

template <typename T>
Optional<Error> ParseValue(const std::string& input, typename std::enable_if<IsBuiltin<T>(), T>::type& output)
{
    auto result = BindingParsers::Parse<T>(input);
    if (!result.HasValue())
    {
        return result.Error();
    }

    output = std::move(result.Value());
    return Optional<Error>();
}

template <typename T>
Optional<Error> ParseParameter(const std::map<std::string, std::string>& args, const std::string& key, Optional<T>& output)
{
    const auto it = args.find(key);
    if (it == args.end())
    {
        return Optional<Error>();
    }

    T value = T();
    auto error = ParseValue<T>(it->second, value);
    if (error.HasValue())
    {
        return error;
    }

    output = std::move(value);
    return Optional<Error>();
}

template <typename T>
Optional<Error> ParseParameter(const std::map<std::string, std::string>& args, const std::string& key, T& output)
{
    const auto it = args.find(key);
    if (it == args.end())
    {
        return Error("Missing required '" + key + "' parameter", EINVAL);
    }

    return ParseValue<T>(it->second, output);
}

template <typename Params>
std::set<std::string> GetFieldNames()
{
    using bindings = Bindings<Params>;
    static const auto names = bindings::names;
    return std::set<std::string>(names, names + bindings::size);
}

// Recursion stop handler - handled by Index >= Bindings<Params>::size condition
template <std::size_t Index, typename Params>
Optional<Error> ParseParameterAt(const std::map<std::string, std::string>& args, typename std::enable_if<Index >= Bindings<Params>::size, Params>::type& output)
{
    UNUSED(args);
    UNUSED(output);
    // End of recursion
    return Optional<Error>();
}

// Recursively parse each field in the structure.
// We always parse fields in order they are defined.
template <std::size_t Index, typename Params>
Optional<Error> ParseParameterAt(const std::map<std::string, std::string>& args, typename std::enable_if < Index<Bindings<Params>::size, Params>::type & output)
{
    using bindings = Bindings<Params>;
    const std::string name = bindings::names[Index];
    constexpr auto member = std::get<Index>(bindings::members);

    auto error = ParseParameter(args, name, output.*member);
    if (error.HasValue())
    {
        return error.Value();
    }

    // Recurse to the next index
    return ParseParameterAt<Index + 1, Params>(args, output);
}

// Parse the map<string, string> arguments into specialized structure Params.
template <typename Params>
Result<Params> ParseArguments(const std::map<std::string, std::string>& args)
{
    using bindings = Bindings<Params>;

    static_assert(std::is_default_constructible<Params>::value, "The parameters structure must be default constructible");
    static_assert(bindings::size, "The parameters structure must not be empty");

    // Static as the bindings are immutable, and we don't need to recreate them all the time
    static const auto fields = GetFieldNames<Params>();

    // Find arguments that are unsupported
    if (bindings::size < args.size())
    {
        return Error("Too many arguments provided", EINVAL);
    }
    for (const auto& arg : args)
    {
        if (fields.end() == fields.find(arg.first))
        {
            return Error("Unknown parameter '" + arg.first + "'", EINVAL);
        }
    }

    // T must be default constructible for this feature to work
    Params result;
    // Recursively consume arguments
    auto error = ParseParameterAt<0, Params>(args, result);
    if (error.HasValue())
    {
        return error.Value();
    }

    return result;
}

// Creates a map<string, string> interface for a native procedure
// using specialized parameters structure nad operator() overload.
template <typename Params>
struct ParametrizedProcedureHandler
{
    // The actual procedure to call
    Result<Status> (*mProcedure)(const Params&, IndicatorsTree&, ContextInterface&);
    explicit ParametrizedProcedureHandler(Result<Status> (*procedure)(const Params&, IndicatorsTree&, ContextInterface&))
        : mProcedure{procedure}
    {
    }
    ParametrizedProcedureHandler(const ParametrizedProcedureHandler&) = default;
    ParametrizedProcedureHandler(ParametrizedProcedureHandler&&) = default;
    ParametrizedProcedureHandler& operator=(const ParametrizedProcedureHandler&) = default;
    ParametrizedProcedureHandler& operator=(ParametrizedProcedureHandler&&) = default;
    ~ParametrizedProcedureHandler() = default;

    Result<Status> operator()(const std::map<std::string, std::string>& args, IndicatorsTree& indicators, ContextInterface& context) const
    {
        // Parse the arguments into the native Params structure
        auto params = BindingsImpl::ParseArguments<Params>(args);
        if (!params.HasValue())
        {
            return params.Error();
        }

        // Call the procedure
        return mProcedure(params.Value(), indicators, context);
    }
};

// Creates a map<string, string> interface for a native procedure
// using the operator() overload.
struct UnparametrizedProcedureHandler
{
    // The actual procedure to call
    Result<Status> (*mProcedure)(IndicatorsTree&, ContextInterface&);
    explicit UnparametrizedProcedureHandler(Result<Status> (*procedure)(IndicatorsTree&, ContextInterface&))
        : mProcedure{procedure}
    {
    }
    UnparametrizedProcedureHandler(const UnparametrizedProcedureHandler&) = default;
    UnparametrizedProcedureHandler(UnparametrizedProcedureHandler&&) = default;
    UnparametrizedProcedureHandler& operator=(const UnparametrizedProcedureHandler&) = default;
    UnparametrizedProcedureHandler& operator=(UnparametrizedProcedureHandler&&) = default;
    ~UnparametrizedProcedureHandler() = default;

    Result<Status> operator()(const std::map<std::string, std::string>& args, IndicatorsTree& indicators, ContextInterface& context) const
    {
        if (!args.empty())
        {
            return Error("Too many arguments provided", EINVAL);
        }

        // Call the procedure
        return mProcedure(indicators, context);
    }
};
} // namespace BindingsImpl

// Creates an instance of ProcedureHandler for the given procedure function
template <typename T>
BindingsImpl::ParametrizedProcedureHandler<T> MakeHandler(Result<Status> (*fn)(const T&, IndicatorsTree&, ContextInterface&))
{
    return BindingsImpl::ParametrizedProcedureHandler<T>(fn);
}

// Creates an instance of ProcedureHandler for the given procedure function
inline BindingsImpl::UnparametrizedProcedureHandler MakeHandler(Result<Status> (*fn)(IndicatorsTree&, ContextInterface&))
{
    return BindingsImpl::UnparametrizedProcedureHandler(fn);
}
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_BINDINGS_H
