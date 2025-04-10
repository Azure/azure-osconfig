// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef COMPLIANCE_INDICATORS_H
#define COMPLIANCE_INDICATORS_H

#include "Logging.h"
#include "MmiResults.h"
#include "Result.h"

#include <cassert>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace compliance
{
struct Indicator
{
    std::string message;
    Status status;

    explicit Indicator(std::string msg, Status stat);
    Indicator(const Indicator&) = default;
    Indicator(Indicator&&) = default;
    Indicator& operator=(const Indicator&) = default;
    Indicator& operator=(Indicator&&) = default;
    ~Indicator() = default;
};

struct Indicators
{
    struct Node
    {
        std::string procedureName;
        Status status = Status::NonCompliant;
        std::vector<std::unique_ptr<Node>> children;
        std::vector<Indicator> indicators;

        explicit Node(std::string procedureName);
        Node(const Node&) = delete;
        Node(Node&&) = delete;
        Node& operator=(const Node&) = delete;
        Node& operator=(Node&&) = delete;
        ~Node() = default;
    };

    Indicators() = default;
    Indicators(const Indicators&) = delete;
    Indicators(Indicators&&) = default;
    Indicators& operator=(const Indicators&) = delete;
    Indicators& operator=(Indicators&&) = default;
    ~Indicators() = default;

    Status Compliant(std::string message) &;
    Status NonCompliant(std::string message) &;
    Node* GetRootNode() const noexcept;
    Status PushIndicator(std::string message, Status status) &;
    void Push(std::string procedureName) &;
    void Pop() &;
    const Node& Back() const& noexcept;
    Node& Back() & noexcept;

private:
    friend class Evaluator;
    std::unique_ptr<Node> mIndicators;
    std::vector<Node*> mEvaluationStack;
};

} // namespace compliance

#endif // COMPLIANCE_INDICATORS_H
