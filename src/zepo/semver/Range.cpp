//
// Created by qingy on 2024/7/17.
//

#include "Range.hpp"

#include <iostream>
#include <locale>
#include <string>

namespace zepo::semver {
    using NodeType = Range::NodeType;
    using Token = Range::Token;
    using BaseNode = Range::BaseNode;

    inline bool isVaildVersionCharactor(const int ch) {
        return std::isdigit(ch) || ch == '.' || ch == '-' || isalpha(ch) || ch == '*' || ch == '+';
    }

    NodeType Range::VersionNode::getNodeType() {
        return Comparsion;
    }

    bool Range::VersionNode::execute(const Version& version) {
        return false;
    }

    Range::HyphenNode::HyphenNode(Version&& from, Version&& to)
        : from{std::move(from)}, to{std::move(to)} {
    }

    NodeType Range::HyphenNode::getNodeType() {
        return Comparsion;
    }

    bool Range::HyphenNode::execute(const Version& version) {
        return version >= from && version <= to;
    }

    NodeType Range::AndNode::getNodeType() {
        return Comparsion;
    }

    bool Range::AndNode::execute(const Version& version) {
        return left->execute(version) && right->execute(version);
    }

    NodeType Range::OrNode::getNodeType() {
        return Union;
    }

    bool Range::OrNode::execute(const Version& version) {
        return left->execute(version) || right->execute(version);
    }

    Range::CompareNode::CompareNode(TokenType compareType, Version&& target)
        : comparsionType(compareType), target{std::move(target)} {
    }

    NodeType Range::CompareNode::getNodeType() {
        return Comparsion;
    }

    bool Range::CompareNode::execute(const Version& version) {
        if (comparsionType == Lt) {
            return version < target;
        }

        if (comparsionType == LtEq) {
            return version <= target;
        }

        if (comparsionType == Gt) {
            return version > target;
        }

        if (comparsionType == GtEq) {
            return version >= target;
        }

        if (comparsionType == Eq) {
            return version == target;
        }

        if (comparsionType == Caret) {
            return version ^ target;
        }

        if (comparsionType == Tilde) {
            return version | target;
        }

        throw std::runtime_error("unknown comparsion operator: " + std::to_string(comparsionType));
    }

    Generator<Token> Range::lexer(const std::string_view expression) {
        using namespace std::string_literals;
        for (size_t current = 0; current < expression.size();) {
            auto currentChar{expression[current]};

            if (currentChar == '^') {
                current++;
                co_yield {Caret};
            } else if (currentChar == '~') {
                current++;
                co_yield {Tilde};
            } else if (currentChar == '*' || std::isdigit(currentChar) || currentChar == 'v' || currentChar == 'V') {
                if (currentChar == 'v' || currentChar == 'V') {
                    current++;
                    if (!isVaildVersionCharactor(expression[current])) {
                        throw std::runtime_error("lexer error: invalid version range at :" + std::to_string(current));
                    }
                }

                const auto begin{current};
                auto enterChannel{false};
                while (current < expression.size()) {
                    if (currentChar = expression[current]; !isVaildVersionCharactor(currentChar)) {
                        break;
                    }

                    if (currentChar == '-') {
                        if (enterChannel) {
                            throw std::runtime_error(
                                "lexer error: invalid version range at :" + std::to_string(current));
                        }
                        enterChannel = true;
                    }

                    if (currentChar == '+') {
                        if (!enterChannel) {
                            throw std::runtime_error(
                                "lexer error: invalid version range at :" + std::to_string(current));
                        }
                    }

                    if (currentChar == '*') {
                        if (enterChannel) {
                            throw std::runtime_error(
                                "lexer error: invalid version range at :" + std::to_string(current));
                        }
                    }
                    current++;
                }

                co_yield {VersionLiteral, expression.substr(begin, current - begin)};
            } else if (currentChar == '=') {
                current++;
                co_yield {Eq};
            } else if (isspace(currentChar)) {
                current++;
            } else if (currentChar == '|') {
                // Or
                current++;
                if (expression[current] != '|') {
                    throw std::runtime_error(
                        "lexer error: invalid operator at :" + std::to_string(current));
                }

                current++;
                co_yield {Or};
            } else if (currentChar == '>') {
                // Gt or GtEq
                current++;
                if (expression[current] == '=') {
                    current++;
                    co_yield {GtEq};
                }

                co_yield {Gt};
            } else if (currentChar == '<') {
                // Lt or LtEq
                current++;
                if (expression[current] == '=') {
                    current++;
                    co_yield {LtEq};
                }

                co_yield {LtEq};
            } else if (currentChar == '-') {
                current++;
                co_yield {Hyphen};
            } else {
                throw std::runtime_error("lexer error: invalid charactor at :" + std::to_string(current));
            }
        }
    }

    std::unique_ptr<BaseNode> Range::parser(Generator<Token>& tokenStream) {
        UniqueNode currentNode{nullptr};
        NodeType currentType{Comparsion};

        while (tokenStream.moveNext()) {
            auto* current = &tokenStream.getCurrent();

            // parse basic
            if (currentType <= Comparsion) {
                if (current->type == Caret
                    || current->type == Gt
                    || current->type == GtEq
                    || current->type == Eq
                    || current->type == Lt
                    || current->type == LtEq
                    || current->type == Tilde) {
                    // comparsion version
                    auto comparsionType = current->type;

                    if (!tokenStream.moveNext()) {
                        throw std::runtime_error(
                            "parser error: unexpected eof");
                    }

                    current = &tokenStream.getCurrent();

                    if (current->type != VersionLiteral) {
                        throw std::runtime_error(
                            "parser error: expected VersionLiteral, but found " + std::to_string(current->type));
                    }

                    UniqueNode createdNode{new CompareNode{comparsionType, Version{current->value}}};
                    if (currentNode) {
                        auto* andNode = new AndNode{};
                        andNode->left = std::move(currentNode);
                        andNode->right = std::move(createdNode);
                        currentNode = UniqueNode{andNode};
                    } else {
                        currentNode = std::move(createdNode);
                    }

                    continue;
                }

                if (current->type == VersionLiteral) {
                    UniqueNode createdNode{new VersionNode{}};
                    if (currentNode) {
                        auto* andNode = new AndNode{};
                        andNode->left = std::move(currentNode);
                        andNode->right = std::move(createdNode);
                        currentNode = UniqueNode{andNode};
                    } else {
                        currentNode = std::move(createdNode);
                    }
                    continue;
                }
            }

            // parse ||
            if (currentType <= Union) {
                if (current->type == Or) {
                    currentType = Union;

                    auto* orNode = new OrNode{};
                    orNode->left = std::move(currentNode);
                    orNode->right = std::move(parser(tokenStream));
                    currentNode = UniqueNode{orNode};
                }
            }
        }

        if (!currentNode) {
            throw std::runtime_error(
                "parser error: unexpected eof");
        }

        return currentNode;
    }

    Range::Range(std::string_view expression) {
        auto tokenStream = lexer(expression);
        rootNode = SharedNode{parser(tokenStream).release()};
    }

    bool Range::satisfies(const Version& target) const {
        return rootNode->execute(target);
    }
} // zepo
