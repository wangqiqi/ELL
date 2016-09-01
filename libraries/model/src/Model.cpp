////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Model.cpp (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Model.h"
#include "Port.h"
#include "Node.h"

// stl
#include <unordered_map>

/// <summary> model namespace </summary>
namespace model
{
    //
    // Model
    //
    Node* Model::GetNode(Node::NodeId id)
    {
        auto it = _idToNodeMap.find(id);
        if (it == _idToNodeMap.end())
        {
            return nullptr; // weak_ptr equivalent of nullptr
        }
        else
        {
            return it->second.get();
        }
    }

    NodeIterator Model::GetNodeIterator(const std::vector<const Node*>& outputNodes) const
    {
        return NodeIterator(this, outputNodes);
    }

    utilities::ObjectDescription Model::GetTypeDescription()
    {
        auto description = utilities::MakeObjectDescription<Model>("Model");
        description.AddProperty<std::vector<const Node*>>("nodes", "The nodes");
        return description;
    }

    utilities::ObjectDescription Model::GetDescription() const
    {
        std::vector<const Node*> nodes;
        auto nodeIter = GetNodeIterator();
        while(nodeIter.IsValid())
        {
            nodes.push_back(nodeIter.Get());
            nodeIter.Next();
        }

        auto description = GetTypeDescription();
        description["nodes"] << nodes;
        return description;
    }

    void Model::SetObjectState(const utilities::ObjectDescription& description, utilities::SerializationContext& context)
    {
        ModelSerializationContext modelContext(context, this);
        
        // Deserialize nodes into big array
        std::vector<const Node*> nodes;
        description["nodes"] >> nodes;

        // Now add them to the model
        for(auto& node: nodes)
        {
            auto sharedNode = std::shared_ptr<Node>(const_cast<Node*>(node));
            sharedNode->RegisterDependencies();
            _idToNodeMap[sharedNode->GetId()] = sharedNode;
        }
    }

    void Model::Deserialize(utilities::Deserializer& serializer) 
    {
        ModelSerializationContext modelContext(serializer.GetContext(), this);
        serializer.PushContext(modelContext);
        // Deserialize nodes into big array
        std::vector<std::unique_ptr<Node>> nodes;
//        serializer.Deserialize("nodes", nodes);
        serializer["nodes"] >> nodes;

        // Now add them to the model
        for(auto& node: nodes)
        {
            std::shared_ptr<Node> sharedNode = std::shared_ptr<Node>(node.release());
            sharedNode->RegisterDependencies();
            _idToNodeMap[sharedNode->GetId()] = sharedNode;
        }
        serializer.PopContext();
    }

    //
    // NodeIterator implementation
    //

    NodeIterator::NodeIterator(const Model* model, const std::vector<const Node*>& outputNodes) : _model(model)
    {
        _currentNode = nullptr;
        _visitFullModel = false;
        if (_model->Size() == 0)
        {
            return;
        }

        // start with output nodes in the stack
        _stack = outputNodes;

        if (_stack.size() == 0) // Visit full model
        {
            // helper function to find a terminal (output) node
            auto IsLeaf = [](const Node* node) { return node->GetDependentNodes().size() == 0; };

            // start with some arbitrary node
            auto iter = _model->_idToNodeMap.begin();
            const Node* anOutputNode = iter->second.get(); // !!! need private access

            // follow dependency chain until we get an output node
            while (!IsLeaf(anOutputNode))
            {
                anOutputNode = anOutputNode->GetDependentNodes()[0];
            }
            _stack.push_back(anOutputNode);
            _visitFullModel = true;
        }

        Next();
    }

    void NodeIterator::Next()
    {
        _currentNode = nullptr;
        while (_stack.size() > 0)
        {
            const Node* node = _stack.back();

            // check if we've already visited this node
            if (_visitedNodes.find(node) != _visitedNodes.end())
            {
                _stack.pop_back();
                continue;
            }

            // we can visit this node only if all its inputs have been visited already
            bool canVisit = true;
            const auto& inputPorts = node->GetInputPorts();
            for (auto inputPort : inputPorts)
            {
                for (const auto& parentNode : inputPort->GetParentNodes())
                {
                    canVisit = canVisit && _visitedNodes.find(parentNode) != _visitedNodes.end();
                }
            }

            if (canVisit)
            {
                _stack.pop_back();
                _visitedNodes.insert(node);

                // In "visit whole model" mode, we want to add dependent nodes, so we can get to parts of the model
                // that the original leaf node doesn't depend on
                if (_visitFullModel)
                {

                    // now add all our children (Note: this part is the only difference between visit-all and visit-active-model
                    const auto& dependentNodes = node->GetDependentNodes();
                    for (const auto& child : ModelImpl::Reverse(dependentNodes)) // Visiting the children in reverse order more closely retains the order the features were originally created
                    {
                        // note: this is kind of inefficient --- we're going to push multiple copies of child on the stack. But we'll check if we've visited it already when we pop it off.
                        // TODO: optimize this if it's a problem
                        _stack.push_back(child);
                    }
                }

                _currentNode = node;
                break;
            }
            else // visit node's inputs
            {
                const auto& inputPorts = node->GetInputPorts();
                for (auto input : ModelImpl::Reverse(inputPorts)) // Visiting the inputs in reverse order more closely retains the order the features were originally created
                {
                    for (const auto& parentNode : input->GetParentNodes())
                    {
                        _stack.push_back(parentNode);
                    }
                }
            }
        }
    }

    //
    // ModelSerializationContext
    //    
    ModelSerializationContext::ModelSerializationContext(utilities::SerializationContext& otherContext, const Model* model) : _originalContext(otherContext), _model(model) 
    {
    }
    
    Node* ModelSerializationContext::GetNodeFromId(const Node::NodeId& id)
    {
        return _oldToNewNodeMap[id];
    }

    void ModelSerializationContext::MapNode(const Node::NodeId& id, Node* node)
    {
        _oldToNewNodeMap[id] = node;
    }
}
