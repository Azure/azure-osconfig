#include "LuaEvaluator.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <CommonUtils.h>
#include <Result.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace ComplianceEngine
{

using std::map;
using std::string;

namespace
{

using ComplianceEngine::ContextInterface;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

// Structure to hold context data for Lua wrapper functions
struct LuaCallContext
{
    IndicatorsTree& indicators;
    ContextInterface& context;
    const std::string& procedureName;
    Action action;

    LuaCallContext(IndicatorsTree& indicators, ContextInterface& context, const std::string& procedureName, Action action)
        : indicators(indicators),
          context(context),
          procedureName(procedureName),
          action(action){};
};

} // anonymous namespace

// LuaEvaluator implementation
LuaEvaluator::LuaEvaluator()
    : L(luaL_newstate())
{

    if (!L)
    {
        throw std::runtime_error("Failed to create Lua state");
    }

    luaL_openlibs(L);
    SecureLuaEnvironment();
    RegisterProcedures();
}

LuaEvaluator::~LuaEvaluator()
{
    if (L)
    {
        lua_close(L);
    }
}

Result<Status> LuaEvaluator::Evaluate(const string& script, IndicatorsTree& indicators, ContextInterface& context, Action action)
{
    auto log = context.GetLogHandle();

    OsConfigLogInfo(log, "Executing Lua compliance script");

    LuaCallContext callContext(indicators, context, "Lua", action);

    lua_pushstring(L, "lua_call_context");
    lua_pushlightuserdata(L, &callContext);
    lua_settable(L, LUA_REGISTRYINDEX);

    int loadResult = luaL_loadstring(L, script.c_str());
    if (loadResult != LUA_OK)
    {
        std::string error = "Lua script compilation failed: ";
        if (lua_isstring(L, -1))
        {
            error += lua_tostring(L, -1);
        }
        OsConfigLogError(log, "%s", error.c_str());
        lua_pop(L, 1);
        return Error(error);
    }

    lua_getfield(L, LUA_REGISTRYINDEX, "restricted_env");
    if (lua_istable(L, -1))
    {
        const char* upvalueName = lua_setupvalue(L, -2, 1);
        if (!upvalueName)
        {
            OsConfigLogError(log, "Could not set restricted Lua environment");
            lua_pop(L, 1);
            lua_settop(L, 0);
            return Error("Could not set restricted Lua environment");
        }
        else
        {
            OsConfigLogInfo(log, "Restricted environment successfully applied to script");
        }
    }
    else
    {
        lua_pop(L, 1);
        lua_settop(L, 0);
        OsConfigLogError(log, "Restricted Lua environment not found");
        return Error("Restricted Lua environment not found");
    }

    int result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result != LUA_OK)
    {
        std::string error = "Lua script execution failed: ";
        if (lua_isstring(L, -1))
        {
            error += lua_tostring(L, -1);
        }
        OsConfigLogError(log, "%s", error.c_str());
        lua_settop(L, 0);
        return Error(error);
    }

    // lua script must return a single value of either a boolean for compliance
    // or string for error, and optionally a second string for a message.
    int numReturns = lua_gettop(L);
    if (numReturns == 0)
    {
        lua_settop(L, 0);
        OsConfigLogError(log, "Lua script did not return a value");
        return Error("Lua script did not return a value");
    }

    std::string message = "Lua script completed";

    if (lua_isboolean(L, 1))
    {
        bool isCompliant = lua_toboolean(L, 1);
        if ((numReturns >= 2) && lua_isstring(L, 2))
        {
            message = lua_tostring(L, 2);
        }

        lua_settop(L, 0);

        if (isCompliant)
        {
            return indicators.Compliant(message);
        }
        else
        {
            return indicators.NonCompliant(message);
        }
    }

    // For errors return value must be a string explicitly, LUA will implicitly convert a number to string
    if (lua_isstring(L, 1) && !lua_isnumber(L, 1))
    {
        message = lua_tostring(L, 1);
        if (numReturns >= 2 && lua_isstring(L, 2))
        {
            message += std::string(" : ") + lua_tostring(L, 2);
        }
        lua_settop(L, 0);
        return Error(message);
    }
    else
    {
        lua_settop(L, 0);
        return Error("Invalid return type from LUA script");
    }
}

void LuaEvaluator::RegisterProcedures()
{
    lua_getfield(L, LUA_REGISTRYINDEX, "restricted_env");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "restricted_env");
    }

    for (const auto& procedureEntry : Evaluator::mProcedureMap)
    {
        const std::string& procedureName = procedureEntry.first;
        const ProcedureActions& actions = procedureEntry.second;

        if (actions.audit)
        {
            std::string auditFunctionName = "Audit" + procedureName;
            lua_pushstring(L, auditFunctionName.c_str());
            lua_pushlightuserdata(L, reinterpret_cast<void*>(actions.audit));
            lua_pushcclosure(L, LuaEvaluator::LuaProcedureWrapper, 2);
            lua_setfield(L, -2, auditFunctionName.c_str());
        }

        if (actions.remediate)
        {
            std::string remediateFunctionName = "Remediate" + procedureName;
            lua_pushstring(L, procedureName.c_str());
            lua_pushlightuserdata(L, reinterpret_cast<void*>(actions.remediate));
            lua_pushcclosure(L, LuaEvaluator::LuaProcedureWrapper, 2);
            lua_setfield(L, -2, remediateFunctionName.c_str());
        }
    }

    lua_pop(L, 1);
}

void LuaEvaluator::SecureLuaEnvironment()
{
    lua_newtable(L);

    const std::vector<const char*> safeGlobals = {
        "print", "type", "tostring", "tonumber", "pairs", "ipairs", "next", "pcall", "xpcall", "select", "math"};
    const std::map<const char*, std::vector<const char*>> safeModuleFunctions = {
        {"string", {"byte", "char", "find", "format", "gsub", "len", "lower", "match", "rep", "reverse", "sub", "upper"}},
        {"table", {"concat", "insert", "remove", "sort"}}, {"io", {"lines"}}, {"os", {"time", "date", "clock", "difftime"}}};

    for (const auto& global : safeGlobals)
    {
        lua_getglobal(L, global);
        lua_setfield(L, -2, global);
    }

    for (const auto& moduleFunctions : safeModuleFunctions)
    {
        lua_getglobal(L, moduleFunctions.first);
        if (lua_istable(L, -1))
        {
            lua_newtable(L);
            for (const auto& func : moduleFunctions.second)
            {
                lua_getfield(L, -2, func);
                lua_setfield(L, -2, func);
            }

            lua_setfield(L, -3, moduleFunctions.first);
        }
        lua_pop(L, 1);
    }

    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "restricted_env");

    lua_pop(L, 1);
}

int LuaEvaluator::LuaProcedureWrapper(lua_State* L)
{
    lua_pushstring(L, "lua_call_context");
    lua_gettable(L, LUA_REGISTRYINDEX);
    LuaCallContext* callContext = static_cast<LuaCallContext*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!callContext)
    {
        lua_pushstring(L, "Failed to get call context");
        // lua_error doesn't return (throws)
        lua_error(L);
        return 0;
    }
    auto log = callContext->context.GetLogHandle();
    lua_pushvalue(L, lua_upvalueindex(1));
    if (!lua_isstring(L, -1))
    {
        OsConfigLogError(log, "Failed to get procedure name from upvalue");
        lua_pushstring(L, "Failed to get procedure name from upvalue");
        lua_error(L);
        return 0;
    }
    std::string procedureName = lua_tostring(L, -1);
    lua_pop(L, 1);

    if ((callContext->action != ComplianceEngine::Action::Remediate) && (procedureName.substr(0, 9) == "Remediate"))
    {
        OsConfigLogError(log, "Remediation not allowed in audit mode");
        lua_pushstring(L, "Remediation not allowed in audit mode");
        lua_error(L);
        return 0;
    }

    lua_pushvalue(L, lua_upvalueindex(2));
    if (!lua_islightuserdata(L, -1))
    {
        OsConfigLogError(log, "Failed to get function pointer from upvalue");
        lua_pushstring(L, "Failed to get function pointer from upvalue");
        lua_error(L);
        return 0;
    }
    ComplianceEngine::action_func_t actionFunc = reinterpret_cast<ComplianceEngine::action_func_t>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!actionFunc)
    {
        OsConfigLogError(log, "No function for procedure %s", procedureName.c_str());
        lua_pushstring(L, ("No function for procedure: " + procedureName).c_str());
        lua_error(L);
        return 0;
    }

    std::map<std::string, std::string> args;

    if ((lua_gettop(L) >= 1) && (lua_istable(L, 1)))
    {
        lua_pushnil(L); // First key
        while (lua_next(L, 1) != 0)
        {
            // Key is at index -2, value is at index -1
            if (lua_isstring(L, -2) && lua_isstring(L, -1))
            {
                std::string key = lua_tostring(L, -2);
                std::string value = lua_tostring(L, -1);
                args[key] = value;
            }
            else
            {
                lua_pushstring(L, "Invalid key-value pair");
                lua_error(L);
                return 0;
            }

            lua_pop(L, 1); // Remove value, keep key for next iteration
        }
    }

    // Call the actual function
    auto result = actionFunc(args, callContext->indicators, callContext->context);

    if (result.HasValue())
    {
        OsConfigLogInfo(callContext->context.GetLogHandle(), "Successful call of lua function, result %scompliant",
            (result.Value() == Status::NonCompliant) ? "non-" : "");
        lua_pushboolean(L, result.Value() == ComplianceEngine::Status::Compliant);

        std::string message =
            (result.Value() == ComplianceEngine::Status::Compliant) ? (procedureName + " is compliant") : (procedureName + " is not compliant");
        lua_pushstring(L, message.c_str());
        return 2;
    }
    else
    {
        OsConfigLogWarning(callContext->context.GetLogHandle(), "LUA script execution ended with an error: %s", result.Error().message.c_str());
        lua_pushstring(L, result.Error().message.c_str());
        lua_error(L);
        return 0;
    }
}

} // namespace ComplianceEngine
