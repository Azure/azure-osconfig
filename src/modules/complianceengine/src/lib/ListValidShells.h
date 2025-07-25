// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCEENGINE_LIST_VALID_SHELLS_H
#define COMPLIANCEENGINE_LIST_VALID_SHELLS_H

#include <ContextInterface.h>
#include <Result.h>
#include <set>

namespace ComplianceEngine
{
Result<std::set<std::string>> ListValidShells(ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_LIST_VALID_SHELLS_H
