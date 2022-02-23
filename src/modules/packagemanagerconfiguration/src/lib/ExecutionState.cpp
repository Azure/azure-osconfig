// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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

std::string ExecutionState::GetReportedExecutionState()
{
    std::string executionState;
    std::string stateComponent = StateComponentToString(m_stateComponent);
    std::string subStateComponent = SubStateComponentToString(m_subStateComponent);

    switch (m_stateComponent)
    {
        case StateComponent::Unknown:
            executionState = stateComponent;
            break;
        case StateComponent::Succeeded:
            executionState = stateComponent;
            break;
        default:
            executionState = m_processingArgument.empty()
                           ? stateComponent + "_" + subStateComponent
                           : stateComponent + "_" + subStateComponent + "_{" + m_processingArgument + "}";
    }

    return executionState;
}

std::string ExecutionState::StateComponentToString(StateComponent stateComponent)
{
    switch (stateComponent)
    {
        case StateComponent::Running:
            return "Running";
        case StateComponent::Succeeded:
            return "Succeeded";
        case StateComponent::Failed:
            return "Failed";
        case StateComponent::TimedOut:
            return "TimedOut";
        default:
            return "Unknown";
    }
}

std::string ExecutionState::SubStateComponentToString(SubStateComponent subStateComponent)
{
    switch (subStateComponent)
    {
        case SubStateComponent::DeserializingJsonPayload:
            return "DeserializingJsonPayload";
        case SubStateComponent::DeserializingDesiredState:
            return "DeserializingDesiredState";
        case SubStateComponent::DeserializingPackages:
            return "DeserializingPackages";
        case SubStateComponent::DeserializingSources:
            return "DeserializingSources";
        case SubStateComponent::ModifyingSources:
            return "ModifyingSources";
        case SubStateComponent::UpdatingPackagesSources:
            return "UpdatingPackagesSources";
        case SubStateComponent::UpdatingPackagesLists:
            return "UpdatingPackagesLists";
        case SubStateComponent::InstallingPackages:
            return "InstallingPackages";
        default:
            return "None";
    }
}
