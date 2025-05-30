#ifndef ENSUREKERNELMODULE_H
#define ENSUREKERNELMODULE_H

#include "ContextInterface.h"
#include "Indicators.h"
#include "Result.h"

#include <string>

namespace compliance
{
Result<Status> EnsureKernelModuleUnavailable(std::string moduleName, IndicatorsTree& indicators, ContextInterface& context);
}

#endif // ENSUREKERNELMODULE_H
