// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <map>
#include <string>
#include <Evaluator.h>

namespace compliance {
    std::map<std::string, std::pair<action_func_t, action_func_t> > Evaluator::mProcedureMap = {
    };
}
