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

Indicators::Node::Node(std::string procedureName)
    : procedureName(std::move(procedureName))
{
}

Status Indicators::Compliant(std::string message) &
{
    return PushIndicator(std::move(message), Status::Compliant);
}

Status Indicators::NonCompliant(std::string message) &
{
    return PushIndicator(std::move(message), Status::NonCompliant);
}

Indicators::Node* Indicators::GetRootNode() const noexcept
{
    return mIndicators.get();
}

Status Indicators::PushIndicator(std::string message, Status status) &
{
    assert(!mEvaluationStack.empty());
    mEvaluationStack.back()->indicators.emplace_back(std::move(message), status);
    return mEvaluationStack.back()->indicators.back().status;
}

void Indicators::Push(std::string procedureName) &
{
    if (!mIndicators)
    {
        mIndicators = std::unique_ptr<Node>(new Node(std::move(procedureName)));
        mEvaluationStack.push_back(mIndicators.get());
        return;
    }

    assert(!mEvaluationStack.empty());
    auto* current = mEvaluationStack.back();
    current->children.push_back(std::unique_ptr<Node>(new Node(std::move(procedureName))));
    mEvaluationStack.push_back(current->children.back().get());
}

void Indicators::Pop() &
{
    assert(!mEvaluationStack.empty());
    mEvaluationStack.pop_back();
}

const Indicators::Node& Indicators::Back() const& noexcept
{
    assert(!mEvaluationStack.empty());
    assert(mEvaluationStack.back());
    return *mEvaluationStack.back();
}

Indicators::Node& Indicators::Back() & noexcept
{
    assert(!mEvaluationStack.empty());
    assert(mEvaluationStack.back());
    return *mEvaluationStack.back();
}

} // namespace compliance
