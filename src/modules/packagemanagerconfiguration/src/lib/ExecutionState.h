// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>

enum StateComponent
{
    Unknown = 0,
    Running,
    Succeeded,
    Failed,
    TimedOut
};

enum SubStateComponent
{
    None,
    DeserializingJsonPayload,
    DeserializingDesiredState,
    DeserializingPackages,
    DeserializingSources,
    ModifyingSources,
    UpdatingPackagesSources,
    UpdatingPackagesLists,
    InstallingPackages
};

class ExecutionState
{
public:
    ExecutionState();
    virtual ~ExecutionState() = default;

    void SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent, std::string processingArgument);
    void SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent);
    std::string GetReportedExecutionState();

private:
    StateComponent m_stateComponent;
    SubStateComponent m_subStateComponent;
    std::string m_processingArgument;

    static std::string StateComponentToString(StateComponent stateComponent);
    static std::string SubStateComponentToString(SubStateComponent subStateComponent);
};