//
// Created by qingy on 2024/7/17.
//

#pragma once
#ifndef ZEPO_RANGE_HPP
#define ZEPO_RANGE_HPP

#include <string_view>

#include "Semver.hpp"
#include "zepo/async/Generator.hpp"

namespace zepo::semver {
    class Range;

    class Range {
    public:
        enum TokenType {
            VersionLiteral,
            Lt,
            Gt,
            LtEq,
            GtEq,
            Eq,
            Hyphen,
            Tilde,
            Or,
            Caret,
        };

        enum NodeType {
            Comparsion,
            Union,
        };

        struct Token {
            TokenType type{VersionLiteral};
            std::string_view value;
        };

        struct BaseNode {
            virtual ~BaseNode() = default;

            virtual NodeType getNodeType() = 0;
            virtual bool execute(const Version&) = 0;
        };

        struct VersionNode : BaseNode {
            NodeType getNodeType() override;
            bool execute(const Version& version) override;
        };

        struct HyphenNode : BaseNode {
            Version from;
            Version to;

            HyphenNode(Version&& from, Version&& to);

            NodeType getNodeType() override;
            bool execute(const Version& version) override;
        };

        struct AndNode : BaseNode {
            std::unique_ptr<BaseNode> left;
            std::unique_ptr<BaseNode> right;

            NodeType getNodeType() override;
            bool execute(const Version& version) override;
        };

        struct OrNode : BaseNode {
            std::unique_ptr<BaseNode> left;
            std::unique_ptr<BaseNode> right;

            NodeType getNodeType() override;
            bool execute(const Version& version) override;
        };

        struct CompareNode : BaseNode {
            TokenType comparsionType{};
            Version target;

            CompareNode(TokenType compareType, Version&& target);

            NodeType getNodeType() override;
            bool execute(const Version& version) override;
        };

        using UniqueNode = std::unique_ptr<BaseNode>;
        using SharedNode = std::shared_ptr<BaseNode>;

    private:
        static Generator<Token> lexer(std::string_view expression);

        static UniqueNode parser(Generator<Token>& tokenStream);

        SharedNode rootNode;

    public:
        explicit Range(std::string_view expression);

        [[nodiscard]] bool satisfies(const Version& target) const;
    };
} // zepo

#endif //ZEPO_RANGE_HPP
