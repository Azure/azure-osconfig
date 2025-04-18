// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Result.h"

#include <ostream>

namespace compliance
{
std::ostream& operator<<(std::ostream& os, const compliance::Error& error)
{
    os << "Error: " << error.message << ": code " << error.code;
    return os;
}
} // namespace compliance
