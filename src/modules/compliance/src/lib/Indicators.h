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

// This structure represents a compliance indicator, which is a message that indicates whether a certain condition is met.
// Each indicator has a message and a status (compliant or non-compliant).
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

// This structure represents a tree of indicators, where each node can have multiple children and indicators.
// The indicators are stored in a vector, and each node has its own status (compliant or non-compliant).
// The tree structure allows for a hierarchical representation of the compliance indicators, making it easier to format payload messages.
// The tree is built dynamically as the rule evaluation process progresses, with nodes being pushed and popped from the stack.
struct IndicatorsTree
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

    IndicatorsTree() = default;
    IndicatorsTree(const IndicatorsTree&) = delete;
    IndicatorsTree(IndicatorsTree&&) = default;
    IndicatorsTree& operator=(const IndicatorsTree&) = delete;
    IndicatorsTree& operator=(IndicatorsTree&&) = default;
    ~IndicatorsTree() = default;

    // Add an indicator to the current node in the evaluation stack.
    // The indicator is created with the given message and status.
    Status AddIndicator(std::string message, Status status) &;

    // Add a compliant indicator to the current node in the evaluation stack.
    Status Compliant(std::string message) &;

    // Add a non-compliant indicator to the current node in the evaluation stack.
    Status NonCompliant(std::string message) &;

    // Get the root node of the tree.
    const Node* GetRootNode() const noexcept;

    // Increase the evaluation stack and add a new child node to the tree.
    // The new node is created with the given procedure name.
    void Push(std::string procedureName) &;

    // Decrease the evaluation stack and preserve the current tree structure.
    void Pop() &;

    // Get the last node in the evaluation stack.
    const Node& Back() const& noexcept;

    // Get the last node in the evaluation stack (non-const version).
    Node& Back() & noexcept;

private:
    std::unique_ptr<Node> mTreeRoot;
    std::vector<Node*> mEvaluationStack;
};

} // namespace compliance

#endif // COMPLIANCE_INDICATORS_H
