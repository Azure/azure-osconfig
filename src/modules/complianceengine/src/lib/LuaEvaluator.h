#ifndef COMPLIANCEENGINE_LUAEVALUATOR_H
#define COMPLIANCEENGINE_LUAEVALUATOR_H

#include "ContextInterface.h"
#include "Evaluator.h"
#include "Indicators.h"
#include "Result.h"

#include <map>
#include <string>

struct lua_State;

namespace ComplianceEngine
{

using std::map;
using std::string;

// LuaEvaluator class manages the Lua environment for a single Evaluator instance.
// It provides a secure sandbox for executing Lua scripts with access to compliance
// engine procedures while blocking dangerous system functions.
//
// Procedure calls from Lua scripts:
// - On success: Return (boolean, string) where boolean indicates compliance status
// - On error: Throw a Lua error that propagates to the script execution context
//
// Security features:
// - Restricted environment with only safe Lua functions
// - No access to file I/O, os.execute, or other dangerous operations
// - Action-based permission control (audit vs remediation functions)
class LuaEvaluator
{
private:
    lua_State* L;

public:
    LuaEvaluator();
    ~LuaEvaluator();

    // Non-copyable
    LuaEvaluator(const LuaEvaluator&) = delete;
    LuaEvaluator& operator=(const LuaEvaluator&) = delete;

    // Evaluate a Lua script in the secure environment.
    //
    // The script can:
    // - Return true/false to indicate compliance status
    // - Return (boolean, message) to provide custom status messages
    // - Return error string to indicate script-level errors
    // - Call registered procedures which either return (boolean, message) or throw Lua errors
    //
    // Parameters:
    // - script: Lua script source code
    // - indicators: Indicators tree for logging compliance status
    // - context: Execution context providing system access
    // - action: Controls which procedures are available (Audit vs Remediate)
    //
    // Returns:
    // - Success: Status::Compliant or Status::NonCompliant
    // - Failure: Error with description of compilation or execution failure
    Result<Status> Evaluate(const string& script, IndicatorsTree& indicators, ContextInterface& context, Action action);

private:
    // Register all available compliance procedures as Lua functions
    void RegisterProcedures();

    // Set up the secure Lua environment by removing dangerous functions
    void SecureLuaEnvironment();

    // Lua C function that wraps compliance procedure calls.
    // Returns - on lua stack - (boolean, string) on success or throws Lua error on failure.
    static int LuaProcedureWrapper(lua_State* L);
};

} // namespace ComplianceEngine

#endif // COMPLIANCEENGINE_LUAEVALUATOR_H
