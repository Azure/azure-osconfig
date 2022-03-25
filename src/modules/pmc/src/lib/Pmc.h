// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PmcBase.h>
#include <string>

class Pmc : public PmcBase
{
public:
    Pmc(unsigned int maxPayloadSizeBytes);
    ~Pmc() = default;

    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;
};