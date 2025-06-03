
#ifndef KERNELMODULETOOLS_H
#define KERNELMODULETOOLS_H

#include "ContextInterface.h"
#include "Indicators.h"
#include "Result.h"

#include <string>

namespace ComplianceEngine
{

Result<bool> SearchFilesystemForModuleName(std::string& moduleName, ContextInterface& context);
Result<bool> IsKernelModuleLoaded(std::string moduleName, ContextInterface& context);
Result<Status> IsKernelModuleBlocked(std::string moduleName, IndicatorsTree& indicators, ContextInterface& context);

} // namespace ComplianceEngine

#endif // KERNELMODULETOOLS_H
