#pragma once
#include "json.h"
#include <vector>
#include <optional>




namespace json {

class Builder;
class KeyItemContext;
class DictItemContext;
class ArrayItemContext;


class Builder {
public:
    Builder() = default;

    Builder(const Builder& b) = delete;
    Builder(Builder&&) = delete;

    Builder& operator=(const Builder&) = delete;
    Builder& operator=(Builder&&) = delete;

    KeyItemContext Key(std::string key);
    Builder& Value(Node::Value value);

    DictItemContext StartDict();
    Builder& EndDict();

    ArrayItemContext StartArray();
    Builder& EndArray();

    Node Build();

private:
    Node* AddNode(Node node);

    bool IsComplete() const;

    Node root_;

    std::vector<Node*> nodes_stack_;

    std::optional<std::string> key_;

    bool root_initialized_ = false;
};

class BaseContext {
public:
    explicit BaseContext(Builder& builder) : builder_(builder) {}
protected:
    Builder& builder_;
};

class KeyItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    DictItemContext Value(Node::Value value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
};

class DictItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    KeyItemContext Key(std::string key);
    Builder& EndDict();
};

class ArrayItemContext : public BaseContext {
public:
    using BaseContext::BaseContext;
    ArrayItemContext Value(Node::Value value);
    ArrayItemContext StartArray();
    DictItemContext StartDict();
    Builder& EndArray();
};

};