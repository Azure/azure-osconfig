#include "LuaEvaluator.h"

#include "LuaProcedures.h"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <CommonUtils.h>
#include <Result.h>
#include <Telemetry.h>
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

// Using unified LuaCallContext from LuaProcedures.h

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

    LuaCallContext callContext{indicators, context, "Lua", action, 0u};

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
        OSConfigTelemetryStatusTrace("luaL_loadstring", -1);
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
            OSConfigTelemetryStatusTrace("lua_setupvalue", -1);
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
        OSConfigTelemetryStatusTrace("lua_getfield", -1);
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
        OSConfigTelemetryStatusTrace("lua_pcall", result);
        luaL_traceback(L, L, NULL, 1);
        const char* traceback = lua_tostring(L, -1);
        if (traceback)
        {
            OsConfigLogError(log, "Lua Traceback: %s", traceback);
        }
        lua_pop(L, 1);
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
        OSConfigTelemetryStatusTrace("lua_gettop", -1);
        return Error("Lua script did not return a value");
    }

    std::string message = "Lua script completed";

    if (lua_isboolean(L, 1))
    {
        if (callContext.indicatorsDepth > 0)
        {
            // If scripts call ce.indicators.push(), we expect them to clean up the stack properly
            lua_settop(L, 0);
            return Error("Indicators stack not cleaned up properly");
        }

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

    // Create or fetch the 'ce' namespace table inside the restricted environment
    lua_getfield(L, -1, "ce");
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);        // pop non-table (nil)
        lua_newtable(L);      // create ce table
        lua_pushvalue(L, -1); // duplicate for setting into parent
        lua_setfield(L, -3, "ce");
    }

    int ceTableIndex = lua_gettop(L); // index of ce table

    for (const auto& procedureEntry : Evaluator::mProcedureMap)
    {
        const std::string& procedureName = procedureEntry.first;
        const ProcedureActions& actions = procedureEntry.second;

        // Audit function exposed as ce.Audit<Procedure>
        if (actions.audit)
        {
            std::string auditFunctionName = "Audit" + procedureName;
            lua_pushstring(L, auditFunctionName.c_str());
            lua_pushlightuserdata(L, const_cast<void*>(reinterpret_cast<const void*>(&actions.audit)));
            lua_pushcclosure(L, LuaEvaluator::LuaProcedureWrapper, 2);
            lua_setfield(L, ceTableIndex, auditFunctionName.c_str());
        }

        // Remediation function exposed as ce.Remediate<Procedure>
        if (actions.remediate)
        {
            std::string remediateFunctionName = "Remediate" + procedureName;
            lua_pushstring(L, procedureName.c_str());
            lua_pushlightuserdata(L, const_cast<void*>(reinterpret_cast<const void*>(&actions.remediate)));
            lua_pushcclosure(L, LuaEvaluator::LuaProcedureWrapper, 2);
            lua_setfield(L, ceTableIndex, remediateFunctionName.c_str());
        }
    }

    // Pop ce table then restricted_env to leave stack clean
    lua_pop(L, 1); // ce table
    lua_pop(L, 1); // restricted_env

    // Register additional helper procedures (e.g., ListDirectory)
    RegisterLuaProcedures(L);
}

void LuaEvaluator::SecureLuaEnvironment()
{
    lua_newtable(L);

    const std::vector<const char*> safeGlobals = {
        "print", "type", "tostring", "tonumber", "pairs", "ipairs", "next", "pcall", "xpcall", "select", "math"};
    const std::map<const char*, std::vector<const char*>> safeModuleFunctions = {
        {"string", {"byte", "char", "find", "format", "gsub", "len", "lower", "match", "gmatch", "rep", "reverse", "sub", "upper"}},
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
    auto log = callContext->ctx.GetLogHandle();
    lua_pushvalue(L, lua_upvalueindex(1));
    if (!lua_isstring(L, -1))
    {
        OsConfigLogError(log, "Failed to get procedure name from upvalue");
        OSConfigTelemetryStatusTrace("lua_upvalueindex", -1);
        lua_pushstring(L, "Failed to get procedure name from upvalue");
        lua_error(L);
        return 0;
    }
    std::string procedureName = lua_tostring(L, -1);
    lua_pop(L, 1);

    if ((callContext->action != ComplianceEngine::Action::Remediate) && (procedureName.substr(0, 9) == "Remediate"))
    {
        OsConfigLogError(log, "Remediation not allowed in audit mode");
        OSConfigTelemetryStatusTrace("action", EPERM);
        lua_pushstring(L, "Remediation not allowed in audit mode");
        lua_error(L);
        return 0;
    }

    lua_pushvalue(L, lua_upvalueindex(2));
    if (!lua_islightuserdata(L, -1))
    {
        OsConfigLogError(log, "Failed to get function pointer from upvalue");
        OSConfigTelemetryStatusTrace("lua_islightuserdata", -1);
        lua_pushstring(L, "Failed to get function pointer from upvalue");
        lua_error(L);
        return 0;
    }
    const ComplianceEngine::action_func_t* actionFunc = reinterpret_cast<const ComplianceEngine::action_func_t*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    if (!actionFunc)
    {
        OsConfigLogError(log, "No function for procedure %s", procedureName.c_str());
        OSConfigTelemetryStatusTrace("actionFunc", ENOENT);
        lua_pushstring(L, ("No function for procedure: " + procedureName).c_str());
        lua_error(L);
        return 0;
    }

    std::map<std::string, std::string> args;

    OsConfigLogInfo(log, "Processing lua procedure %s", procedureName.c_str());

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
                const char* k = lua_tostring(L, -2);
                const char* v = lua_tostring(L, -1);
                std::string errormsg = std::string("Invalid key-value pair '") + (k ? k : "NIL") + "':'" + (v ? v : "NIL") + "'";
                lua_pushstring(L, errormsg.c_str());
                lua_error(L);
                return 0;
            }

            lua_pop(L, 1); // Remove value, keep key for next iteration
        }
    }

    // Call the actual function
    auto result = (*actionFunc)(args, callContext->indicators, callContext->ctx);

    if (result.HasValue())
    {
        OsConfigLogInfo(callContext->ctx.GetLogHandle(), "Lua procedure '%s' executed: %scompliant", procedureName.c_str(),
            (result.Value() == Status::NonCompliant) ? "non-" : "");
        lua_pushboolean(L, result.Value() == ComplianceEngine::Status::Compliant);

        std::string message =
            (result.Value() == ComplianceEngine::Status::Compliant) ? (procedureName + " is compliant") : (procedureName + " is not compliant");
        lua_pushstring(L, message.c_str());
        return 2;
    }
    else
    {
        OsConfigLogWarning(callContext->ctx.GetLogHandle(), "LUA script execution ended with an error: %s", result.Error().message.c_str());
        lua_pushstring(L, result.Error().message.c_str());
        lua_error(L);
        return 0;
    }
    return 0;
}

} // namespace ComplianceEngine
