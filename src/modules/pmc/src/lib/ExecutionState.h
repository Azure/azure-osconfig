// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

class ExecutionState
{
public:
    enum StateComponent
    {
        Unknown = 0,
        Running,
        Succeeded,
        Failed,
        TimedOut
    };

    enum SubstateComponent
    {
        None,
        DeserializingJsonPayload,
        DeserializingDesiredState,
        DeserializingGpgKeys,
        DeserializingSources,
        DeserializingPackages,
        DownloadingGpgKeys,
        ModifyingSources,
        UpdatingPackageLists,
        InstallingPackages
    };

    ExecutionState();
    virtual ~ExecutionState() = default;

    void SetExecutionState(StateComponent stateComponent, SubstateComponent substateComponent, std::string processingArgument);
    void SetExecutionState(StateComponent stateComponent, SubstateComponent substateComponent);
    bool IsSuccessful() const;
    StateComponent GetExecutionState() const;
    SubstateComponent GetExecutionSubstate() const;
    std::string GetExecutionSubstateDetails() const;

private:
    StateComponent m_stateComponent;
    SubstateComponent m_substateComponent;
    std::string m_processingArgument;
};
