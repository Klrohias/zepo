//
// Created by qingy on 2024/7/14.
//

#pragma once

#ifndef ZEPO_SERIALIZER_HPP
#define ZEPO_SERIALIZER_HPP

#include "zepo/serialize/Reflect.hpp"
#include <string>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <optional>

namespace zepo {
    // declares
    struct SerializerExtensionDataAttribute {
    };

    struct SerializerNameAttribute {
        std::string name;
    };

    template<typename FieldType, typename TokenType>
    concept IsExtensionDataMap = requires(FieldType& it)
    {
        { it.try_emplace(std::string{}, TokenType{}) };
        { it[std::string{}] } -> std::convertible_to<TokenType>;
    };

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

    template<typename Type, typename DocType, typename TokenType>
    struct TokenifyTraits {
        struct SerializeHandler {
            enum Flag {
                kNone = 0,
                kExtensionData = 0b1,
            };

            int flag{Flag::kNone};
            SerializerNameAttribute* customNameAttribute{nullptr};

            TokenType& token;
            const Type& value;
            DocType& doc;

            explicit SerializeHandler(TokenType& token, const Type& value, DocType& doc)
                : token{token}, value{value}, doc{doc} {
            }

            template<auto Name, auto FieldReference>
            void field() {
                using FieldType = decltype(Type{}.*FieldReference);

                if constexpr (IsExtensionDataMap<FieldType, TokenType>) {
                    if (flag & kExtensionData) {
                        flag = kNone;
                        // put extension data
                        for (const auto& [key, value]: value.*FieldReference) {
                            auto resultToken = TokenType::from(doc, value);
                            token.appendChild(key, resultToken);
                        }
                        return;
                    }
                }

                auto resultToken = TokenifyTraits<FieldType, DocType, TokenType>::tokenify(doc, value.*FieldReference);
                if (customNameAttribute) {
                    token.appendChild(customNameAttribute->name, resultToken);
                    customNameAttribute = nullptr;
                } else {
                    token.appendChild(Name(), resultToken);
                }

                flag = kNone;
            }

            template<auto Attribute>
            void attribute() {
                if constexpr (std::is_same_v<decltype(Attribute()), SerializerExtensionDataAttribute*>) {
                    flag |= kExtensionData;
                }

                if constexpr (std::is_same_v<decltype(Attribute()), SerializerNameAttribute*>) {
                    customNameAttribute = Attribute();
                }
            }
        };

        static TokenType tokenify(DocType& doc, const Type& value) {
            TokenType token{doc, false};
            TypeDefTraits<Type, SerializeHandler> reflectTraits{token, value, doc};
            reflectTraits.execute();

            return token;
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

    template<typename T, typename... I, typename TokenType>
    struct ParseTraits<std::map<std::string, T, I...>, TokenType> {
        using Map = std::map<std::string, T, I...>;

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

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<std::string, DocType, TokenType> {
        using TargetType = std::string;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<uint8_t, DocType, TokenType> {
        using TargetType = uint8_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<uint16_t, DocType, TokenType> {
        using TargetType = uint16_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<uint32_t, DocType, TokenType> {
        using TargetType = uint32_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<uint64_t, DocType, TokenType> {
        using TargetType = uint64_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<int8_t, DocType, TokenType> {
        using TargetType = int8_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<int16_t, DocType, TokenType> {
        using TargetType = int16_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<int32_t, DocType, TokenType> {
        using TargetType = int32_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<int64_t, DocType, TokenType> {
        using TargetType = int64_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<float_t, DocType, TokenType> {
        using TargetType = float_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<double_t, DocType, TokenType> {
        using TargetType = double_t;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType>
    struct TokenifyTraits<TokenType, DocType, TokenType> {
        static TokenType tokenify(DocType& doc, const TokenType& value) {
            return TokenType::from(doc, value);
        }
    };

    template<typename DocType, typename TokenType, typename WrappedType>
    struct TokenifyTraits<std::optional<WrappedType>, DocType, TokenType> {
        using TargetType = std::optional<WrappedType>;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            if (value.has_value()) {
                return TokenType::from(doc, value.value());
            }

            return TokenType::fromNull(doc);
        }
    };

    template<typename DocType, typename TokenType, typename WrappedType>
    struct TokenifyTraits<std::vector<WrappedType>, DocType, TokenType> {
        using TargetType = std::vector<WrappedType>;

        static TokenType tokenify(DocType& doc, const TargetType& value) {
            TokenType token{doc, true};
            for (const WrappedType& item: value) {
                const auto resultToken = TokenifyTraits<WrappedType, DocType, TokenType>::tokenify(doc, item);
                token.appendChild(resultToken);
            }

            return token;
        }
    };

    template<typename DocType, typename TokenType, typename T, typename... I>
    struct TokenifyTraits<std::map<std::string, T, I...>, DocType, TokenType> {
        using Map = std::map<std::string, T, I...>;

        static TokenType tokenify(DocType& doc, const Map& value) {
            TokenType token{doc, false};

            for (const auto& [key, item]: value) {
                const auto resultToken = TokenifyTraits<T, DocType, TokenType>::tokenify(doc, item);
                token.appendChild(key, resultToken);
            }

            return token;
        }
    };

    template<typename TokenType, typename T, typename DocType>
    TokenType tokenify(DocType& doc, const T& token) {
        return TokenifyTraits<T, DocType, TokenType>::tokenify(doc, token);
    }

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
    struct DeserializeContext {
        enum Flag {
            kNone = 0,
            kFoundExtensionDataInsertor = 0b1,
            kIsNotExtension = 0b10,
        };

        struct DeserializeFieldProcessor {
            DeserializeContext& context;
            std::string_view key;
            const TokenType& value;

            explicit DeserializeFieldProcessor(DeserializeContext& context, std::string_view key,
                                               const TokenType& value)
                : context{context}, key{key}, value{value} {
            }

            template<auto Name, auto FieldReference>
            void field() {
                using FieldType = decltype(Type{}.*FieldReference);

                if ((context.customNameAttribute && key == context.customNameAttribute->name)
                    || (!context.customNameAttribute && Name() == key)) {
                    context.target.*FieldReference = zepo::parse<FieldType>(value);
                }

                context.reset();
            }

            template<auto Attribute>
            void attribute() const {
                using AttributePtrType = decltype(Attribute());

                if constexpr (std::is_same_v<AttributePtrType, SerializerExtensionDataAttribute*>) {
                    context.scanExtensionDataRequired = true;
                }

                if constexpr (std::is_same_v<AttributePtrType, SerializerNameAttribute*>) {
                    context.customNameAttribute = Attribute();
                }
            }
        };

        struct DeserializeExtensionDataProcessor {
            DeserializeContext& context;
            std::string_view key;
            const TokenType& value;

            explicit DeserializeExtensionDataProcessor(
                DeserializeContext& context,
                const std::string_view key,
                const TokenType& value
            ) : context{context}, key{key}, value{value} {
            }

            template<auto Name, auto FieldReference>
            void field() {
                using FieldType = decltype(Type{}.*FieldReference);

                if constexpr (IsExtensionDataMap<FieldType, TokenType>) {
                    if (context.flags & kFoundExtensionDataInsertor) {
                        context.extensionDataInsertor = [](Type* target, const std::string& key, const TokenType& value) {
                            (target->*FieldReference).try_emplace(key, value);
                        };

                        context.flags &= ~kFoundExtensionDataInsertor;
                    }
                }


                if ((context.customNameAttribute && key == context.customNameAttribute->name)
                    || (!context.customNameAttribute && Name() == key)) {
                    context.flags |= kIsNotExtension;
                }
            }

            void postprocess() {
                if (!(context.flags & kIsNotExtension)) {
                    // is extension
                    context.extensionData.try_emplace(std::string{key}, value);
                }
                context.reset();
            }

            template<auto Attribute>
            void attribute() const {
                using AttributePtrType = decltype(Attribute());

                if constexpr (std::is_same_v<AttributePtrType, SerializerExtensionDataAttribute*>) {
                    context.flags |= kFoundExtensionDataInsertor;
                }

                if constexpr (std::is_same_v<AttributePtrType, SerializerNameAttribute*>) {
                    context.customNameAttribute = Attribute();
                }
            }
        };

        bool scanExtensionDataRequired{false};
        int flags{kNone};
        SerializerNameAttribute* customNameAttribute{nullptr};
        std::function<void(Type*, const std::string&, const TokenType&)> extensionDataInsertor;
        std::map<std::string, TokenType> extensionData;

        Type& target;
        const TokenType& token;

        explicit DeserializeContext(Type& target, const TokenType& token)
            : target{target}, token(token) {
        }

        void deserializeExtensionData() {
            zepo::forEach<TokenType>(
                token,
                [&](std::string_view key, const TokenType& value) {
                    TypeDefTraits<Type, DeserializeExtensionDataProcessor> processor{
                        *this,
                        key,
                        value
                    };

                    processor.execute();
                    processor.postprocess();
                    return false;
                }
            );

            for (auto [k, v]: extensionData) {
                extensionDataInsertor(&target, k, v);
            }
        }

        void deserialize() {
            zepo::forEach<TokenType>(token, [this](std::string_view key, const TokenType& value) {
                TypeDefTraits<Type, DeserializeFieldProcessor> processor{*this, key, value};
                processor.execute();
                return false;
            });

            if (scanExtensionDataRequired) {
                deserializeExtensionData();
            }
        }

        void reset() {
            customNameAttribute = nullptr;
            flags = kNone;
        }
    };
}

#ifndef ZEPO_NO_MACROS

#define ZS_MakeParsable(TYPE_) template<typename TokenType> \
struct zepo::ParseTraits<TYPE_, TokenType> { \
    static TYPE_ parse(const TokenType& token) { \
        TYPE_ result; \
        zepo::DeserializeContext<TYPE_, TokenType> context{result, token}; \
        context.deserialize(); \
        return result; \
    } \
}

#define ZS_DefTokenizer(TYPE_, FUNC_) template<typename DocType, typename TokenType> \
struct zepo::TokenifyTraits<TYPE_, DocType, TokenType> { \
    static TokenType tokenify(DocType& forwardedDoc, const TYPE_& forwardedValue) { \
        using TargetType = TYPE_; \
        return FUNC_(forwardedDoc, forwardedValue); \
    } \
}

#endif //ZEPO_NO_MACROS

#endif //ZEPO_SERIALIZER_HPP
