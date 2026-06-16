#include "json_builder.h"


namespace json {

Node* Builder::AddNode(Node node) {
    if (root_initialized_ && nodes_stack_.empty()) {
        throw std::logic_error("JSON уже закончен");
    }
    if (!root_initialized_) {
        root_ = std::move(node);
        root_initialized_ = true;
        return &root_;
    }

    Node* current_node = nodes_stack_.back();

    if (current_node->IsArray()) {
        Array& array = current_node->AsArray();
        array.emplace_back(std::move(node));
        return &array.back();
    }

    if (current_node->IsMap()) {
        if (!key_) {
            throw std::logic_error("Для значения словаря сначала нужно задать ключ");
        }

        Dict& dict = current_node->AsMap();
        auto result = dict.emplace(std::move(*key_), std::move(node));
        key_.reset();
        return &result.first->second;
    }

    throw std::logic_error("Неправильный контекст для добавления значения");
}

Builder& Builder::Value(Node::Value value) {
    AddNode(Node{std::move(value)});
    return *this;
}

DictItemContext Builder::StartDict() {
    Node* node = AddNode(Dict{});
    nodes_stack_.push_back(node);
    return DictItemContext(*this);
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
        throw std::logic_error("Последний эелемент не словарь");
    }
    if (key_) {
        throw std::logic_error("Для ключа не задано значение");
    }
    nodes_stack_.pop_back();
    return *this;
}


ArrayItemContext Builder::StartArray() {
    Node* node = AddNode(Array{});
    nodes_stack_.push_back(node);
    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty() || !nodes_stack_.back()->IsArray()) {
        throw std::logic_error("Последний эелемент не массив");
    }
    nodes_stack_.pop_back();
    return *this;
}


KeyItemContext Builder::Key(std::string key) {
    if (root_initialized_ && nodes_stack_.empty()) {
        throw std::logic_error("JSON уже закончен");
    }

    if (nodes_stack_.empty() || !nodes_stack_.back()->IsMap()) {
        throw std::logic_error("Key можно вызвать только внутри словаря");
    }

    if (key_) {
        throw std::logic_error("Для предыдущего ключа не задано значение");
    }

    key_ = std::move(key);
    return KeyItemContext(*this);
}

Node Builder::Build() {
    if (!root_initialized_) {
        throw std::logic_error("JSON не инициализирован");
    }
    if (!nodes_stack_.empty()) {
        throw std::logic_error("JSON не закончен");
    }
    if (key_) {
        throw std::logic_error("Для предыдущего ключа не задано значение");
    }
    return root_;
}

DictItemContext KeyItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext{builder_};
}

DictItemContext KeyItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext KeyItemContext::StartArray() {
    return builder_.StartArray();
}

KeyItemContext DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& DictItemContext::EndDict() {
    return builder_.EndDict();
}

ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return ArrayItemContext{builder_};
}

DictItemContext ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayItemContext::EndArray() {
    return builder_.EndArray();
}

};