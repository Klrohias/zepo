//
// Created by qingy on 2024/7/14.
//

#ifndef ZEPO_JSON_HPP
#define ZEPO_JSON_HPP
#include <functional>
#include <memory>
#include <string>
#include <yyjson.h>

namespace zepo {
    struct JsonDocument;

    struct JsonToken {
        using ForEachItemAction = std::function<bool(JsonToken)>;
        using ForEachKeyValueAction = std::function<bool(std::string_view, JsonToken)>;

    private:
        bool mutable_{false};

        union {
            yyjson_val* val_;
            yyjson_mut_val* mutableVal_;
        };
        yyjson_mut_doc* mutableDoc_{nullptr};

        void checkObjectType(yyjson_type type) const;

        [[nodiscard]] yyjson_type getObjectType() const;

    public:
        explicit JsonToken();

        explicit JsonToken(JsonDocument& jsonDoc, bool isArray = false);

        explicit JsonToken(yyjson_val* val);

        explicit JsonToken(yyjson_mut_doc* jsonDoc, yyjson_mut_val* val);

        [[nodiscard]] yyjson_mut_val* getRawMutableValue() const;

        [[nodiscard]] yyjson_val* getRawImmutableValue() const;

        [[nodiscard]] std::string toString() const;

        [[nodiscard]] double_t toDouble() const;

        [[nodiscard]] float_t toFloat() const;

        [[nodiscard]] int8_t toInt8() const;

        [[nodiscard]] int16_t toInt16() const;

        [[nodiscard]] int32_t toInt32() const;

        [[nodiscard]] int64_t toInt64() const;

        [[nodiscard]] uint8_t toUint8() const;

        [[nodiscard]] uint16_t toUint16() const;

        [[nodiscard]] uint32_t toUint32() const;

        [[nodiscard]] uint64_t toUint64() const;

        static JsonToken from(JsonDocument& jsonDoc, const std::string& value);

        static JsonToken from(JsonDocument& jsonDoc, double_t value);

        static JsonToken from(JsonDocument& jsonDoc, float_t value);

        static JsonToken from(JsonDocument& jsonDoc, int8_t value);

        static JsonToken from(JsonDocument& jsonDoc, int16_t value);

        static JsonToken from(JsonDocument& jsonDoc, int32_t value);

        static JsonToken from(JsonDocument& jsonDoc, int64_t value);

        static JsonToken from(JsonDocument& jsonDoc, uint8_t value);

        static JsonToken from(JsonDocument& jsonDoc, uint16_t value);

        static JsonToken from(JsonDocument& jsonDoc, uint32_t value);

        static JsonToken from(JsonDocument& jsonDoc, uint64_t value);

        static JsonToken from(JsonDocument& jsonDoc, const JsonToken& value);

        static JsonToken fromNull(JsonDocument& jsonDoc);

        void forEach(const ForEachItemAction& action) const;

        void forEach(const ForEachKeyValueAction& action) const;

        void appendChild(const JsonToken& token);

        void appendChild(std::string_view key, const JsonToken& token);
    };

    struct JsonDocument {
    private:
        std::shared_ptr<yyjson_doc> doc_{};
        std::shared_ptr<yyjson_mut_doc> mutableDoc_{};
        bool mutable_{false};

    public:
        [[nodiscard]] JsonToken getRootToken() const;

        explicit JsonDocument(std::string_view content, bool openMutable = false);

        explicit JsonDocument();

        [[nodiscard]] yyjson_mut_doc* getRawMutableValue() const;

        [[nodiscard]] yyjson_doc* getRawImmutableValue() const;

        void setRoot(const JsonToken& rootToken);

        std::string stringify();
    };
}

#endif //ZEPO_JSON_HPP
