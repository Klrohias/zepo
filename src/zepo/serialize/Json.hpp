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
    struct JsonToken {
        using ForEachItemAction = std::function<bool(JsonToken)>;
        using ForEachKeyValueAction = std::function<bool(std::string_view, JsonToken)>;

    private:
        bool mutable_{false};
        union {
            yyjson_val* val_;
            yyjson_mut_val* mutableVal_;
        };

        void checkObjectType(yyjson_type type) const;

        [[nodiscard]] yyjson_type getObjectType() const;

    public:
        explicit JsonToken();

        explicit JsonToken(yyjson_val* val);

        explicit JsonToken(yyjson_mut_val* val);

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

        void forEach(const ForEachItemAction& action) const;

        void forEach(const ForEachKeyValueAction& action) const;
    };

    struct JsonDocument {
        std::shared_ptr<yyjson_doc> doc_{};
        std::shared_ptr<yyjson_mut_doc> mutableDoc_{};
        bool mutable_{false};

        [[nodiscard]] JsonToken getRootToken() const;

        explicit JsonDocument(std::string_view content, bool openMutable = false);

        explicit JsonDocument();
    };

    struct JsonPropertyNameAttribute {
        std::string name;
    };
}

#endif //ZEPO_JSON_HPP
