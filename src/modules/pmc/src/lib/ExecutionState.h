// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

class ExecutionState
{
public:
    enum StateComponent
    {
        unknown = 0,
        running,
        succeeded,
        failed,
        timedOut
    };

    enum SubStateComponent
    {
        none,
        deserializingJsonPayload,
        deserializingDesiredState,
        deserializingPackages,
        updatingPackageLists,
        installingPackages
    };

    ExecutionState();
    virtual ~ExecutionState() = default;

    void SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent, std::string processingArgument);
    void SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent);
    bool IsSuccessful();
    StateComponent GetExecutionState();
    SubStateComponent GetExecutionSubState();
    std::string GetExecutionSubStateDetails();

private:
    StateComponent m_stateComponent;
    SubStateComponent m_subStateComponent;
    std::string m_processingArgument;
};