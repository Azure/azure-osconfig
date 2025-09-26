// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

// Forward declare lua_State from Lua headers to avoid pulling them in everywhere
struct lua_State;

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
void RegisterLuaProcedures(lua_State* L);
