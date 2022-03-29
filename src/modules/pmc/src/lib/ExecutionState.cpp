// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <string>
#include "ExecutionState.h"

ExecutionState::ExecutionState()
{
    m_stateComponent = StateComponent::Unknown;
    m_subStateComponent = SubStateComponent::None;
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

bool ExecutionState::IsSuccessful()
{
    return m_stateComponent == StateComponent::Succeeded;
}

ExecutionState::StateComponent ExecutionState::GetExecutionState()
{
    return m_stateComponent;
}

ExecutionState::SubStateComponent ExecutionState::GetExecutionSubState()
{
    return m_subStateComponent;
}

std::string ExecutionState::GetExecutionSubStateDetails()
{
    return m_processingArgument;
}
