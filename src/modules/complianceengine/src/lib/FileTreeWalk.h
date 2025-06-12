// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_FILE_TREE_WALK_H
#define COMPLIANCE_FILE_TREE_WALK_H

#include <ContextInterface.h>
#include <IterationHelpers.h>
#include <MmiResults.h>
#include <Result.h>
#include <functional>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace ComplianceEngine
{
using FtwCallback = std::function<Result<Status>(const std::string&, const std::string&, const struct stat&)>;

// This function walks the file tree starting from the given path and calls the provided callback for each file/directory.
// The callback should return a Result<Status> indicating the compliance status of the file/directory.
// If the callback returns a non-compliant status and breakOnNonCompliant is set to true, the walk will stop.
// The function returns a Result<Status> indicating the overall compliance status of the tree.
// If the walk encounters an error, it will return an Error object with the error message and code.
// The goal is to mimic the nftw function from the C standard library, but with additional context and support for indicators and compliance-specific interface.
Result<Status> FileTreeWalk(const std::string& path, FtwCallback callback, BreakOnNonCompliant breakOnNonCompliant, ContextInterface& context);
} // namespace ComplianceEngine

#endif // COMPLIANCE_FILE_TREE_WALK_H
