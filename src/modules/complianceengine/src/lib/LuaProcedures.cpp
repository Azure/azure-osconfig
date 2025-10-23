// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LuaProcedures.h"

#include "ContextInterface.h"
#include "FilesystemScanner.h"
#include "Result.h"
#include "lauxlib.h"
#include "lua.h"

#include <Indicators.h>
#include <SystemdCatConfig.h>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fnmatch.h>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using ComplianceEngine::FilesystemScanner;
using ComplianceEngine::Result;

namespace ComplianceEngine
{
namespace
{
class ListDirState
{
public:
    struct Frame
    {
        DIR* dir{nullptr};
        std::string relPath; // relative path from base
    };

    std::string basePath;
    std::string pattern; // empty means match all
    bool recursive{false};
    std::vector<Frame> stack; // depth-first traversal
    ~ListDirState()
    {
        for (auto& frame : stack)
        {
            if (frame.dir)
            {
                closedir(frame.dir);
                frame.dir = nullptr;
            }
        }
    }
};

bool PushDir(ListDirState& state, const std::string& relPath)
{
    std::string full = state.basePath;
    if (!relPath.empty())
    {
        full += "/" + relPath;
    }
    DIR* d = opendir(full.c_str());
    if (!d)
    {
        return false;
    }
    ListDirState::Frame frame;
    frame.dir = d;
    frame.relPath = relPath;
    state.stack.push_back(frame);
    return true;
}

// iterator closure upvalue[1] = userdata(ListDirState*)
int ListDirectoryIterator(lua_State* L)
{
    auto statePtr = reinterpret_cast<ListDirState**>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!statePtr || !*statePtr)
    {
        return 0;
    }
    ListDirState* state = *statePtr;

    while (!state->stack.empty())
    {
        auto& frame = state->stack.back();
        errno = 0;
        dirent* entry = readdir(frame.dir);
        if (!entry)
        {
            int status = errno; // if !=0 error
            closedir(frame.dir);
            frame.dir = nullptr;
            state->stack.pop_back();
            if (status != 0)
            {
                lua_pushfstring(L, "ListDirectory iteration error: %s", strerror(status));
                lua_error(L);
                return 0;
            }
            continue; // move to previous frame
        }

        const char* name = entry->d_name;
        if ((0 == strcmp(name, ".")) || (0 == strcmp(name, "..")))
        {
            continue;
        }

        std::string relChild = frame.relPath.empty() ? name : (frame.relPath + "/" + name);
        std::string full = state->basePath + "/" + relChild;

        // Determine if directory for potential recursion
        bool isDir = false;
        if (entry->d_type == DT_DIR)
        {
            isDir = true;
        }
        else if (entry->d_type == DT_UNKNOWN)
        {
            struct stat sb;
            if (0 == lstat(full.c_str(), &sb))
            {
                isDir = S_ISDIR(sb.st_mode);
            }
        }

        if (isDir)
        {
            if (state->recursive)
            {
                if (!PushDir(*state, relChild))
                {
                    lua_pushfstring(L, "ListDirectory failed to open '%s': %s", full.c_str(), strerror(errno));
                    lua_error(L);
                    return 0;
                }
            }
            // Directories are never yielded
            continue;
        }

        bool patternEmpty = state->pattern.empty();
        if (patternEmpty || (0 == fnmatch(state->pattern.c_str(), name, 0)))
        {
            // Yield relative path (basename or relative path when recursive)
            lua_pushlstring(L, relChild.c_str(), relChild.size());
            return 1;
        }
        // else continue loop for next entry
    }

    return 0; // done
}

static int ListDirectoryGC(lua_State* L)
{
    auto statePtr = reinterpret_cast<ListDirState**>(lua_touserdata(L, 1));
    if (statePtr && *statePtr)
    {
        delete *statePtr;
        *statePtr = nullptr;
    }
    return 0;
}

// ce.ListDirectory implementation
static int LuaListDirectory(lua_State* L)
{
    const char* path = luaL_checkstring(L, 1);
    std::string pattern;
    if (!lua_isnoneornil(L, 2))
    {
        pattern = luaL_checkstring(L, 2);
    }
    bool recursive = false;
    if (!lua_isnoneornil(L, 3))
    {
        recursive = lua_toboolean(L, 3) != 0;
    }

    auto stateHolder = reinterpret_cast<ListDirState**>(lua_newuserdata(L, sizeof(ListDirState*)));
    *stateHolder = new ListDirState();
    (*stateHolder)->basePath = path;
    (*stateHolder)->pattern = pattern;
    (*stateHolder)->recursive = recursive;

    if (!PushDir(**stateHolder, ""))
    {
        delete *stateHolder;
        *stateHolder = nullptr;
        lua_pushfstring(L, "ListDirectory failed to open '%s': %s", path, strerror(errno));
        lua_error(L);
        return 0;
    }

    // Metatable (once)
    if (luaL_newmetatable(L, "ListDirStateMT"))
    {
        lua_pushcfunction(L, ListDirectoryGC);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    lua_pushcclosure(L, ListDirectoryIterator, 1);
    return 1;
}

// ---------------- Filesystem permission-filter iterator ----------------
struct FSCacheIterState
{
    std::shared_ptr<const FilesystemScanner::FSCache> cache;
    std::map<std::string, FilesystemScanner::FSEntry>::const_iterator it;
    std::map<std::string, FilesystemScanner::FSEntry>::const_iterator end;
    mode_t hasMask{0};
    mode_t noMask{0};
    bool started{false};
};

static int FSCacheIterNext(lua_State* L)
{
    auto holder = reinterpret_cast<FSCacheIterState**>(lua_touserdata(L, lua_upvalueindex(1)));
    if (!holder || !*holder)
    {
        return 0;
    }
    FSCacheIterState* st = *holder;
    while (st->it != st->end)
    {
        const auto& kv = *st->it;
        ++st->it;
        mode_t m = kv.second.st.st_mode;
        if (st->hasMask && (m & st->hasMask) != st->hasMask)
        {
            continue;
        }
        if (st->noMask && (m & st->noMask) != 0)
        {
            continue;
        }
        lua_pushlstring(L, kv.first.c_str(), kv.first.size());
        return 1;
    }
    return 0;
}

static int FSCacheIterGC(lua_State* L)
{
    auto holder = reinterpret_cast<FSCacheIterState**>(lua_touserdata(L, 1));
    if (holder && *holder)
    {
        delete *holder;
        *holder = nullptr;
    }
    return 0;
}

static unsigned ParseMaskArg(lua_State* L, int index)
{
    if (lua_isnoneornil(L, index))
    {
        return 0u;
    }
    const char* value = lua_tostring(L, index);
    if (!value || value[0] != '0')
    {
        luaL_error(L, "expected octal number starting with 0 for permission mask");
        return 0u;
    }
    char* endptr;
    long ln = strtol(value, &endptr, 8);
    if (*endptr != '\0')
    {
        luaL_error(L, "expected number or nil for permission mask");
        return 0u;
    }
    if (ln < 0 || ln > 0xFFFFFFFFu) // conservative range check
    {
        luaL_error(L, "permission mask out of range");
        return 0u;
    }
    unsigned v = static_cast<unsigned>(ln);
    return v;
}

static int LuaGetFilesystemEntriesWithPerms(lua_State* L)
{
    unsigned hasMask = ParseMaskArg(L, 1);
    unsigned noMask = ParseMaskArg(L, 2);

    // Fetch call context placed in registry by evaluator to access ContextInterface
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* cc = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!cc)
    {
        luaL_error(L, "internal error: missing call context");
        return 0;
    }
    auto* view = reinterpret_cast<LuaCallContext*>(cc);
    FilesystemScanner& scanner = view->ctx.GetFilesystemScanner();
    auto full = scanner.GetFullFilesystem();
    if (!full)
    {
        luaL_error(L, "%s", full.Error().message.c_str());
        return 0;
    }
    auto stateHolder = reinterpret_cast<FSCacheIterState**>(lua_newuserdata(L, sizeof(FSCacheIterState*)));
    // stack: userdata
    *stateHolder = new FSCacheIterState();
    (*stateHolder)->cache = full.Value();
    (*stateHolder)->it = (*stateHolder)->cache->entries.begin();
    (*stateHolder)->end = (*stateHolder)->cache->entries.end();
    (*stateHolder)->hasMask = static_cast<mode_t>(hasMask);
    (*stateHolder)->noMask = static_cast<mode_t>(noMask);

    if (luaL_newmetatable(L, "FSCacheIterStateMT"))
    {
        lua_pushcfunction(L, FSCacheIterGC);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);

    lua_pushcclosure(L, FSCacheIterNext, 1);
    return 1;
}

int LuaSystemdCatConfig(lua_State* L)
{
    // Fetch call context placed in registry by evaluator to access ContextInterface
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* cc = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!cc)
    {
        luaL_error(L, "internal error: missing call context");
        return 0;
    }
    auto* view = reinterpret_cast<LuaCallContext*>(cc);

    // Filename parameter
    constexpr int filenameArgIndex = 1;
    if (lua_isnoneornil(L, filenameArgIndex))
    {
        luaL_error(L, "expected a filename");
        return 0u;
    }
    const char* filename = lua_tostring(L, filenameArgIndex);
    if (!filename)
    {
        luaL_error(L, "expected a filename");
        return 0u;
    }

    auto result = SystemdCatConfig(filename, view->ctx);
    if (!result.HasValue())
    {
        luaL_error(L, "SystemdCatConfigFailed: %s", result.Error().message.c_str());
        return 0;
    }

    lua_pushlstring(L, result.Value().c_str(), result.Value().size());
    return 1;
}

constexpr unsigned int INDICATORS_STACK_LIMIT = 10;
int LuaIndicatorsPush(lua_State* L)
{
    // Fetch call context placed in registry by evaluator to access ContextInterface
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* cc = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!cc)
    {
        luaL_error(L, "internal error: missing call context");
        return 0;
    }
    auto* view = reinterpret_cast<LuaCallContext*>(cc);
    if (view->indicatorsDepth == INDICATORS_STACK_LIMIT)
    {
        luaL_error(L, "indicators stack limit reached");
        return 0;
    }

    // Procedure name parameter
    constexpr int procedureArgIndex = 1;
    if (lua_isnoneornil(L, procedureArgIndex))
    {
        luaL_error(L, "expected a procedure name");
        return 0u;
    }
    const char* procedureName = lua_tostring(L, procedureArgIndex);
    if (!procedureName)
    {
        luaL_error(L, "expected a procedure name");
        return 0u;
    }

    view->indicators.Push(procedureName);
    view->indicatorsDepth++;
    return 0;
}

int LuaIndicatorsPop(lua_State* L)
{
    // Fetch call context placed in registry by evaluator to access ContextInterface
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* cc = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!cc)
    {
        luaL_error(L, "internal error: missing call context");
        return 0;
    }
    auto* view = reinterpret_cast<LuaCallContext*>(cc);
    if (!view->indicatorsDepth)
    {
        luaL_error(L, "indicators stack is empty");
        return 0;
    }

    view->indicators.Pop();
    view->indicatorsDepth--;
    return 0;
}

int LuaIndicatorsAddIndicator(lua_State* L, Status status)
{
    // Fetch call context placed in registry by evaluator to access ContextInterface
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    void* cc = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (!cc)
    {
        luaL_error(L, "internal error: missing call context");
        return 0;
    }
    auto* view = reinterpret_cast<LuaCallContext*>(cc);

    // Message parameter
    constexpr int messageArgIndex = 1;
    if (lua_isnoneornil(L, messageArgIndex))
    {
        luaL_error(L, "expected a message");
        return 0u;
    }
    const char* message = lua_tostring(L, messageArgIndex);
    if (!message)
    {
        luaL_error(L, "expected a message");
        return 0u;
    }

    view->indicators.AddIndicator(message, status);
    if (status == Status::Compliant)
    {
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushboolean(L, 0);
    }
    return 1;
}

int LuaIndicatorsCompliant(lua_State* L)
{
    return LuaIndicatorsAddIndicator(L, Status::Compliant);
}

int LuaIndicatorsNonCompliant(lua_State* L)
{
    return LuaIndicatorsAddIndicator(L, Status::NonCompliant);
}
} // anonymous namespace

void RegisterLuaProcedures(lua_State* L)
{
    // Get restricted_env
    lua_getfield(L, LUA_REGISTRYINDEX, "restricted_env");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return; // nothing we can do
    }

    // Get or create ce table
    lua_getfield(L, -1, "ce");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);   // pop non-table
        lua_newtable(L); // create ce
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "ce");
    }

    // stack: restricted_env, ce

    lua_pushcfunction(L, LuaListDirectory);
    lua_setfield(L, -2, "ListDirectory");

    lua_pushcfunction(L, LuaGetFilesystemEntriesWithPerms);
    lua_setfield(L, -2, "GetFilesystemEntriesWithPerms");

    lua_pushcfunction(L, LuaSystemdCatConfig);
    lua_setfield(L, -2, "SystemdCatConfig");

    // Get or create ce.indicators table
    lua_getfield(L, -1, "indicators");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);   // pop non-table
        lua_newtable(L); // create indicators
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, "indicators");

        lua_pushcfunction(L, LuaIndicatorsPush);
        lua_setfield(L, -2, "push");

        lua_pushcfunction(L, LuaIndicatorsPop);
        lua_setfield(L, -2, "pop");

        lua_pushcfunction(L, LuaIndicatorsCompliant);
        lua_setfield(L, -2, "compliant");

        lua_pushcfunction(L, LuaIndicatorsNonCompliant);
        lua_setfield(L, -2, "noncompliant");
    }

    // pop ce, restricted_env and indicators
    lua_pop(L, 3);
}
} // namespace ComplianceEngine
