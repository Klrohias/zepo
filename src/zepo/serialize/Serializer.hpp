//
// Created by qingy on 2024/7/14.
//

#pragma once

#ifndef ZEPO_SERIALIZER_HPP
#define ZEPO_SERIALIZER_HPP
#include <string>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace zepo {
    // declares
    template<typename T, typename TokenType>
    T parse(const TokenType& token);

    template<typename TokenType>
    void forEach(const TokenType& token, const std::function<bool(TokenType)>& handler);

    template<typename TokenType>
    void forEach(const TokenType& token,
                 const std::function<bool(std::string_view, TokenType)>& handler);

    // definitions

    template<typename T, typename TokenType>
    struct ParseTraits {
        static T parse(const TokenType& token) {
            T result{};
            result.parse(token);
            return result;
        }
    };

    template<typename TokenType>
    struct ParseTraits<std::string, TokenType> {
        static std::string parse(const TokenType& token) {
            return token.toString();
        }
    };

    template<typename TokenType>
    struct ParseTraits<float_t, TokenType> {
        static float_t parse(const TokenType& token) {
            return token.toFloat();
        }
    };

    template<typename TokenType>
    struct ParseTraits<double_t, TokenType> {
        static double_t parse(const TokenType& token) {
            return token.toDouble();
        }
    };

    template<typename TokenType>
    struct ParseTraits<uint64_t, TokenType> {
        static uint64_t parse(const TokenType& token) {
            return token.toUint64();
        }
    };

    template<typename TokenType>
    struct ParseTraits<uint32_t, TokenType> {
        static uint32_t parse(const TokenType& token) {
            return token.toUint32();
        }
    };

    template<typename TokenType>
    struct ParseTraits<uint16_t, TokenType> {
        static uint16_t parse(const TokenType& token) {
            return token.toUint16();
        }
    };

    template<typename TokenType>
    struct ParseTraits<uint8_t, TokenType> {
        static uint8_t parse(const TokenType& token) {
            return token.toUint8();
        }
    };

    template<typename TokenType>
    struct ParseTraits<int64_t, TokenType> {
        static int64_t parse(const TokenType& token) {
            return token.toInt64();
        }
    };

    template<typename TokenType>
    struct ParseTraits<int32_t, TokenType> {
        static int32_t parse(const TokenType& token) {
            return token.toInt32();
        }
    };

    template<typename TokenType>
    struct ParseTraits<int16_t, TokenType> {
        static int16_t parse(const TokenType& token) {
            return token.toInt16();
        }
    };

    template<typename TokenType>
    struct ParseTraits<int8_t, TokenType> {
        static int8_t parse(const TokenType& token) {
            return token.toInt8();
        }
    };

    template<typename TokenType>
    struct ParseTraits<TokenType, TokenType> {
        static TokenType parse(const TokenType& token) {
            return token;
        }
    };

    template<typename T, typename TokenType>
    struct ParseTraits<std::vector<T>, TokenType> {
        using Vector = std::vector<T>;

        static Vector parse(const TokenType& token) {
            Vector vectorResult{};
            forEach<TokenType>(token, [&vectorResult](const TokenType& childToken) {
                vectorResult.push_back(ParseTraits<T, TokenType>::parse(childToken));
                return false;
            });
            return vectorResult;
        }
    };

    template<typename T, typename TokenType>
    struct ParseTraits<std::map<std::string, T>, TokenType> {
        using Map = std::map<std::string, T>;

        static Map parse(const TokenType& token) {
            Map mapResult{};

            forEach<TokenType>(token, [&](std::string_view key, const TokenType& childToken) {
                mapResult[std::string(key)] = ParseTraits<T, TokenType>::parse(childToken);
                return false;
            });

            return mapResult;
        }
    };

    template<typename T, typename TokenType>
    struct ParseTraits<std::optional<T>, TokenType> {
        using Optional = std::optional<T>;

        static Optional parse(const TokenType& token) {
            return {ParseTraits<T, TokenType>::parse(token)};
        }
    };

    template<typename T, typename TokenType>
    T parse(const TokenType& token) {
        return ParseTraits<T, TokenType>::parse(token);
    }

    template<typename TokenType>
    void forEach(const TokenType& token, const std::function<bool(TokenType)>& handler) {
        token.forEach(handler);
    }

    template<typename TokenType>
    void forEach(const TokenType& token,
                 const std::function<bool(std::string_view, TokenType)>& handler) {
        token.forEach(handler);
    }

    template<typename TokenType>
    TokenType get(const TokenType& token, std::string_view findKey) {
        TokenType result{};
        forEach(token, [&result, &findKey](auto key, auto value) {
            if (findKey == key) {
                result = std::move(value);
                return true;
            }

            return false;
        });
        return result;
    }

    template<typename Type, typename TokenType>
    struct SerializerHandler {
        Type& target;
        const TokenType& token;
        std::string_view targetName;

        SerializerHandler(Type& target, const TokenType& token, std::string_view targetName)
            : target{target}, token{token}, targetName{targetName} {
        }

        template<auto Name, auto FieldReference>
        void field() {
            if (targetName == Name()) {
                target.*FieldReference = zepo::parse<decltype(Type{}.*FieldReference)>(token);
            }
        }

        template<auto Attribute>
        void attribute() {
        }
    };
}

#ifndef ZEPO_NO_MACROS

#define ZEPO_REFLECT_PARSABLE_(TYPE_) template<typename TokenType> \
struct zepo::ParseTraits<TYPE_, TokenType> { \
    static TYPE_ parse(const TokenType& token) { \
        TYPE_ result; \
        zepo::forEach<TokenType>(token, [&result](std::string_view key, const TokenType& value) { \
            zepo::ReflectTraits<TYPE_, SerializerHandler<TYPE_, TokenType>> reflectHandle{result, value, key}; \
            reflectHandle.handle(); \
            return false; \
        }); \
        return result; \
    } \
}

#endif //ZEPO_NO_MACROS

#endif //ZEPO_SERIALIZER_HPP
