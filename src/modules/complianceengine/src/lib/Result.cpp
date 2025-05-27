// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Result.h"

#include <ostream>

namespace ComplianceEngine
{
std::ostream& operator<<(std::ostream& os, const ComplianceEngine::Error& error)
{
    os << "Error: " << error.message << ": code " << error.code;
    return os;
}
} // namespace ComplianceEngine
