// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Indicators.h"

namespace compliance
{
Indicator::Indicator(std::string msg, Status stat)
    : message(std::move(msg)),
      status(stat)
{
}

IndicatorsTree::Node::Node(std::string procedureName)
    : procedureName(std::move(procedureName))
{
}

Status IndicatorsTree::Compliant(std::string message) &
{
    return AddIndicator(std::move(message), Status::Compliant);
}

Status IndicatorsTree::NonCompliant(std::string message) &
{
    return AddIndicator(std::move(message), Status::NonCompliant);
}

const IndicatorsTree::Node* IndicatorsTree::GetRootNode() const noexcept
{
    return mTreeRoot.get();
}

Status IndicatorsTree::AddIndicator(std::string message, Status status) &
{
    assert(!mEvaluationStack.empty());
    mEvaluationStack.back()->indicators.emplace_back(std::move(message), status);
    return mEvaluationStack.back()->indicators.back().status;
}

void IndicatorsTree::Push(std::string procedureName) &
{
    if (!mTreeRoot)
    {
        mTreeRoot = std::unique_ptr<Node>(new Node(std::move(procedureName)));
        mEvaluationStack.push_back(mTreeRoot.get());
        return;
    }

    assert(!mEvaluationStack.empty());
    auto* current = mEvaluationStack.back();
    current->children.push_back(std::unique_ptr<Node>(new Node(std::move(procedureName))));
    mEvaluationStack.push_back(current->children.back().get());
}

void IndicatorsTree::Pop() &
{
    assert(!mEvaluationStack.empty());
    mEvaluationStack.pop_back();
}

const IndicatorsTree::Node& IndicatorsTree::Back() const& noexcept
{
    assert(!mEvaluationStack.empty());
    assert(mEvaluationStack.back());
    return *mEvaluationStack.back();
}

IndicatorsTree::Node& IndicatorsTree::Back() & noexcept
{
    assert(!mEvaluationStack.empty());
    assert(mEvaluationStack.back());
    return *mEvaluationStack.back();
}

} // namespace compliance
