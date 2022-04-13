// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include "ExecutionState.h"

ExecutionState::ExecutionState()
{
    m_stateComponent = StateComponent::unknown;
    m_subStateComponent = SubStateComponent::none;
    m_processingArgument = "";
}

void ExecutionState::SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent, std::string processingArgument)
{
    m_stateComponent = stateComponent;
    m_subStateComponent = subStateComponent;
    m_processingArgument = processingArgument;
}

void ExecutionState::SetExecutionState(StateComponent stateComponent, SubStateComponent subStateComponent)
{
    SetExecutionState(stateComponent, subStateComponent, "");
}

bool ExecutionState::IsSuccessful() const
{
    return m_stateComponent == StateComponent::succeeded;
}

ExecutionState::StateComponent ExecutionState::GetExecutionState() const
{
    return m_stateComponent;
}

ExecutionState::SubStateComponent ExecutionState::GetExecutionSubState() const
{
    return m_subStateComponent;
}

std::string ExecutionState::GetExecutionSubStateDetails() const
{
    return m_processingArgument;
}
