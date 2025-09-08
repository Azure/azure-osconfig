// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_SEPARATED_H
#define COMPLIANCEENGINE_SEPARATED_H

#include <BindingParsers.h>
#include <initializer_list>
#include <numeric>
#include <type_traits>
#include <vector>

namespace std
{
// Helper to make std::to_string() work for the Separated type when T is std::string
inline string to_string(const string& value) // NOLINT(*-identifier-naming)
{
    return value;
}
} // namespace std

namespace ComplianceEngine
{
// Used to represent a list of items separated by a specific character
template <typename T, char Separator>
struct Separated
{
    static constexpr char separator = Separator;
    using ItemType = T;
    static_assert(std::is_default_constructible<T>::value, "Separated item type must be default constructible");
    std::vector<T> items;

    Separated() = default;
    Separated(const Separated&) = default;
    Separated(Separated&&) = default;
    Separated& operator=(const Separated&) = default;
    Separated& operator=(Separated&&) = default;
    ~Separated() = default;

    Separated(const std::vector<T>& items)
        : items(items)
    {
    }

    Separated(std::vector<T>&& items)
        : items(std::move(items))
    {
    }

    static Result<Separated> Parse(const std::string& input)
    {
        Separated result;
        std::stringstream inputStream(input);
        std::string item;
        while (std::getline(inputStream, item, separator))
        {
            auto parsingResult = BindingParsers::Parse<T>(item);
            if (!parsingResult.HasValue())
            {
                return parsingResult.Error();
            }

            result.items.push_back(std::move(parsingResult.Value()));
        }

        return result;
    }

    std::string ToString() const
    {
        return std::accumulate(items.begin(), items.end(), std::string(),
            [](const std::string& a, const T& b) { return a + (a.length() > 0 ? std::string(1, separator) : "") + std::to_string(b); });
    }
};
} // namespace ComplianceEngine
#endif // COMPLIANCEENGINE_SEPARATED_H
