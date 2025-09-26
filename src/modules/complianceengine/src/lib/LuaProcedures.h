// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

// Forward declare lua_State from Lua headers to avoid pulling them in everywhere
struct lua_State;
namespace ComplianceEngine
{

// Forward declare needed types to avoid heavy includes.
class ContextInterface;
struct IndicatorsTree;
enum class Action : int;

struct LuaCallContext
{
    IndicatorsTree& indicators; // reference to indicators root
    ContextInterface& ctx;      // reference to execution context
    const char* procedureName;  // optional procedure name
    Action action;              // current action (Audit/Remediate)
};

// Registers additional custom helper procedures exposed to Lua under the `ce` table.
// Currently provides:
//   ce.ListDirectory(path, pattern, recursive) -> iterator closure
//     path: string (required) directory to list
//     pattern: string (optional) fnmatch(3) pattern applied ONLY to file basenames; empty or nil matches all
//     recursive: boolean (optional, default false) recurse into subdirectories
// Returns a closure suitable for: for rel in ce.ListDirectory("/etc", "*.conf", true) do ... end
// Behavior:
//   - Yields only FILES (never directories)
//   - Yields relative path from the provided base path (basename for top-level files, may include subdir components when recursive)
//   - Pattern never applied to directory names; recursion still explores all subdirectories
//   - Order is depth-first, unspecified
//   - Raises Lua error on IO errors (including mid-iteration)
//
//   ce.GetFilesystemEntriesWithPerms(has_perms, no_perms) -> iterator closure
//     has_perms: integer bitmask (optional/nil => no required bits). All specified bits must be present in st_mode.
//     no_perms: integer bitmask (optional/nil => no excluded bits). None of these bits may be present in st_mode.
// Behavior:
//   - Iterates over cached filesystem entries from FilesystemScanner (full paths as scanned).
//   - Yields only paths whose st_mode satisfies: (mode & has_perms == has_perms) AND (mode & no_perms) == 0.
//   - If has_perms is 0 or nil, no inclusion constraint is applied. If no_perms is 0 or nil, no exclusion filter is applied.
//   - Order reflects underlying cache map order (lexicographic by path).
//   - Raises Lua error if filesystem cache unavailable (propagates underlying error message on first iteration attempt).
// Notes:
//   - Snapshot semantics: iterator holds shared_ptr<const FSCache>; unaffected by background refresh.
//   - Arguments outside unsigned 32-bit range produce Lua error.
void RegisterLuaProcedures(lua_State* L);

} // namespace ComplianceEngine
