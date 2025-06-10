// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef SCOPEGUARD_H
#define SCOPEGUARD_H
#include <functional>

typedef std::function<void()> FunctionType;

class ScopeGuard
{
    public:
        ScopeGuard(const FunctionType& f): func(f), dismiss(false) {}
        ~ScopeGuard()
        {
            if (!dismiss)
            {
                func();
            }
        }
        void Dismiss()
        {
            dismiss = true;
        }
    private:
        FunctionType func;
        bool dismiss;
        ScopeGuard(const ScopeGuard& Rhs);
        ScopeGuard& operator=(const ScopeGuard&);
};

#endif  // SCOPEGUARD_H
