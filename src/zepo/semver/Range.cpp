//
// Created by qingy on 2024/7/17.
//

#include "Range.hpp"

#include <charconv>
#include <iostream>
#include <locale>
#include <string>

namespace zepo::semver {
    using NodeLevel = Range::NodeLevel;
    using NodeType = Range::NodeType;
    using Token = Range::Token;
    using BaseNode = Range::BaseNode;

    inline bool isVaildVersionCharactor(const int ch) {
        return std::isdigit(ch) || ch == '.' || ch == '-' || isalpha(ch) || ch == '*' || ch == '+';
    }

    inline size_t parseVersionPattern(const std::string_view& view, std::optional<int>& output, size_t begin = 0) {
        auto patternEnd = view.find('.', begin);
        if (patternEnd == -1) {
            patternEnd = view.size();
        }

        const auto patternStr = view.substr(begin, patternEnd);
        if (patternStr != "*" && patternStr != "x" && patternStr != "X") {
            int val{};
            std::from_chars(patternStr.data(), patternStr.data() + patternStr.size(), val);
            output.emplace(val);
        }

        return patternEnd;
    }

    void Range::VersionNode::parse() {
        const std::string_view view{pattern};

        auto majorEnd = parseVersionPattern(view, major_);
        if (majorEnd == view.size()) return;

        auto minorEnd = parseVersionPattern(view, minor_, majorEnd + 1);
        if (minorEnd == view.size()) return;

        parseVersionPattern(view, minor_, minorEnd + 1);
        parsed_ = true;
    }

    NodeLevel Range::VersionNode::getNodeLevel() {
        return NodeLevel::Comparsion;
    }

    NodeType Range::VersionNode::getNodeType() {
        return NodeType::Version;
    }

    bool Range::VersionNode::execute(const Version& version) {
        if (!parsed_) parse();

        if (major_.has_value()) {
            if (major_.value() != version.getMajor()) {
                return false;
            }
        }

        if (minor_.has_value()) {
            if (minor_.value() != version.getMinor()) {
                return false;
            }
        }

        if (patch_.has_value()) {
            if (patch_.value() != version.getPatch()) {
                return false;
            }
        }
        return true;
    }

    Range::HyphenNode::HyphenNode(Version&& from, Version&& to)
        : from{std::move(from)}, to{std::move(to)} {
    }

    NodeLevel Range::HyphenNode::getNodeLevel() {
        return NodeLevel::Comparsion;
    }

    NodeType Range::HyphenNode::getNodeType() {
        return NodeType::Hyphen;
    }

    bool Range::HyphenNode::execute(const Version& version) {
        return version >= from && version <= to;
    }

    NodeLevel Range::AndNode::getNodeLevel() {
        return NodeLevel::Comparsion;
    }

    NodeType Range::AndNode::getNodeType() {
        return NodeType::And;
    }

    bool Range::AndNode::execute(const Version& version) {
        return left->execute(version) && right->execute(version);
    }

    NodeLevel Range::OrNode::getNodeLevel() {
        return NodeLevel::Union;
    }

    NodeType Range::OrNode::getNodeType() {
        return NodeType::Or;
    }

    bool Range::OrNode::execute(const Version& version) {
        return left->execute(version) || right->execute(version);
    }

    Range::CompareNode::CompareNode(TokenType compareType, Version&& target)
        : comparsionType(compareType), target{std::move(target)} {
    }

    NodeLevel Range::CompareNode::getNodeLevel() {
        return NodeLevel::Comparsion;
    }

    NodeType Range::CompareNode::getNodeType() {
        return NodeType::Compare;
    }

    bool Range::CompareNode::execute(const Version& version) {
        if (comparsionType == TokenType::Lt) {
            return version < target;
        }

        if (comparsionType == TokenType::LtEq) {
            return version <= target;
        }

        if (comparsionType == TokenType::Gt) {
            return version > target;
        }

        if (comparsionType == TokenType::GtEq) {
            return version >= target;
        }

        if (comparsionType == TokenType::Eq) {
            return version == target;
        }

        if (comparsionType == TokenType::Caret) {
            return version ^ target;
        }

        if (comparsionType == TokenType::Tilde) {
            return version | target;
        }

        throw std::runtime_error("unknown comparsion operator: " + std::to_string(static_cast<int>(comparsionType)));
    }

    Generator<Token> Range::lexer(const std::string_view expression) {
        using namespace std::string_literals;

        // parse token and co_yield them simply
        for (size_t current = 0; current < expression.size();) {
            auto currentChar{expression[current]};

            if (currentChar == '^') {
                current++;
                co_yield {TokenType::Caret};
            } else if (currentChar == '~') {
                current++;
                co_yield {TokenType::Tilde};
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

                co_yield {TokenType::Version, expression.substr(begin, current - begin)};
            } else if (currentChar == '=') {
                current++;
                co_yield {TokenType::Eq};
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
                co_yield {TokenType::Or};
            } else if (currentChar == '>') {
                // Gt or GtEq
                current++;
                if (expression[current] == '=') {
                    current++;
                    co_yield {TokenType::GtEq};
                }

                co_yield {TokenType::Gt};
            } else if (currentChar == '<') {
                // Lt or LtEq
                current++;
                if (expression[current] == '=') {
                    current++;
                    co_yield {TokenType::LtEq};
                }

                co_yield {TokenType::LtEq};
            } else if (currentChar == '-') {
                current++;
                co_yield {TokenType::Hyphen};
            } else {
                throw std::runtime_error("lexer error: invalid charactor at :" + std::to_string(current));
            }
        }
    }

    std::unique_ptr<BaseNode> Range::parser(Generator<Token>& tokenStream) {
        UniqueNode currentNode{nullptr};
        NodeLevel currentLevel{NodeLevel::Comparsion};

        while (tokenStream.moveNext()) {
            auto* currentToken = &tokenStream.getCurrent();

            // comparsion or matches expr
            if (currentLevel <= NodeLevel::Comparsion) {
                // comparsion
                if (currentToken->type == TokenType::Caret
                    || currentToken->type == TokenType::Gt
                    || currentToken->type == TokenType::GtEq
                    || currentToken->type == TokenType::Eq
                    || currentToken->type == TokenType::Lt
                    || currentToken->type == TokenType::LtEq
                    || currentToken->type == TokenType::Tilde) {
                    // comparsion version
                    const auto comparsionType = currentToken->type;

                    if (!tokenStream.moveNext()) {
                        throw std::runtime_error(
                            "parser error: unexpected eof");
                    }

                    currentToken = &tokenStream.getCurrent();

                    if (currentToken->type != TokenType::Version) {
                        throw std::runtime_error(
                            "parser error: expected VersionLiteral, but found "
                            + std::to_string(static_cast<int>(currentToken->type)));
                    }

                    UniqueNode createdNode{new CompareNode{comparsionType, Version{currentToken->value}}};
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

                // version matching (x-range etc.)
                if (currentToken->type == TokenType::Version) {
                    const auto createdNode = new VersionNode{};
                    createdNode->pattern = currentToken->value;

                    if (currentNode) {
                        auto* andNode = new AndNode{};
                        andNode->left = std::move(currentNode);
                        andNode->right = UniqueNode{createdNode};
                        currentNode = UniqueNode{andNode};
                    } else {
                        currentNode = UniqueNode{createdNode};
                    }
                    continue;
                }

                // from some version to another version
                if (currentToken->type == TokenType::Hyphen) {
                    if (auto nodeType = currentNode->getNodeType(); nodeType != NodeType::Version) {
                        throw std::runtime_error("parser error: expected VersionLiteral, but found "
                                                 + std::to_string(static_cast<int>(nodeType)));
                    }

                    Version from{dynamic_cast<VersionNode*>(currentNode.get())->pattern};

                    if (!tokenStream.moveNext()) {
                        throw std::runtime_error(
                            "parser error: unexpected eof");
                    }

                    currentToken = &tokenStream.getCurrent();

                    if (currentToken->type != TokenType::Version) {
                        throw std::runtime_error(
                            "parser error: expected VersionLiteral, but found "
                            + std::to_string(static_cast<int>(currentToken->type)));
                    }

                    Version to{currentToken->value};
                    currentNode = UniqueNode{new HyphenNode{std::move(from), std::move(to)}};
                }
            }

            // parse ||
            // if (currentType <= Union) { // FIXME: always true
            {
                if (currentToken->type == TokenType::Or) {
                    currentLevel = NodeLevel::Union;

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
