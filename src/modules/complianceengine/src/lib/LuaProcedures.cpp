// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "LuaProcedures.h"

#include "lauxlib.h"
#include "lua.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fnmatch.h>
#include <memory>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

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

    // pop ce and restricted_env
    lua_pop(L, 2);
} // namespace ComplianceEngine
}
