// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include "ExecutionState.h"

ExecutionState::ExecutionState()
{
    m_stateComponent = StateComponent::Unknown;
    m_substateComponent = SubstateComponent::None;
    m_processingArgument = "";
}

void ExecutionState::SetExecutionState(StateComponent stateComponent, SubstateComponent substateComponent, std::string processingArgument)
{
    m_stateComponent = stateComponent;
    m_substateComponent = substateComponent;
    m_processingArgument = processingArgument;
}

void ExecutionState::SetExecutionState(StateComponent stateComponent, SubstateComponent substateComponent)
{
    SetExecutionState(stateComponent, substateComponent, "");
}

bool ExecutionState::IsSuccessful() const
{
    return (m_stateComponent != StateComponent::Failed) && (m_stateComponent != StateComponent::TimedOut);
}

ExecutionState::StateComponent ExecutionState::GetExecutionState() const
{
    return m_stateComponent;
}

ExecutionState::SubstateComponent ExecutionState::GetExecutionSubstate() const
{
    return m_substateComponent;
}

std::string ExecutionState::GetExecutionSubstateDetails() const
{
    return m_processingArgument;
}
