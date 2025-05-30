#ifndef ENSUREKERNELMODULE_H
#define ENSUREKERNELMODULE_H

#include "ContextInterface.h"
#include "Indicators.h"
#include "Result.h"

#include <string>

namespace compliance
{

Result<bool> SearchFilesystemForModuleName(std::string& moduleName, ContextInterface& context);
Result<bool> IsKernelModuleLoaded(std::string moduleName, ContextInterface& context);
Result<Status> IsKernelModuleBlocked(std::string moduleName, IndicatorsTree& indicators, ContextInterface& context);

} // namespace compliance

#endif // ENSUREKERNELMODULE_H
