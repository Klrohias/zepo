//
// Created by qingy on 2024/7/14.
//

#include "Json.hpp"

#include <sstream>
#include <stdexcept>

namespace zepo {
    struct YyjsonDeleter {
        void operator()(yyjson_doc* doc) const {
            yyjson_doc_free(doc);
        }

        void operator()(yyjson_mut_doc* doc) const {
            yyjson_mut_doc_free(doc);
        }
    } yyjsonDeleter;


    void JsonToken::checkObjectType(yyjson_type type) const {
        if (getObjectType() != type) {
            throw std::runtime_error{"Type not match"};
        }
    }

    yyjson_type JsonToken::getObjectType() const {
        return mutable_ ? yyjson_mut_get_type(mutableVal_) : yyjson_get_type(val_);
    }

    JsonToken::JsonToken(): mutableVal_{nullptr} {
    }

    JsonToken::JsonToken(JsonDocument& jsonDoc, bool isArray)
        : JsonToken(
            jsonDoc.getRawMutableValue(),
            isArray
                ? yyjson_mut_arr(jsonDoc.getRawMutableValue())
                : yyjson_mut_obj(jsonDoc.getRawMutableValue())
        ) {
    }

    JsonToken::JsonToken(yyjson_val* val) {
        val_ = val;
    }

    JsonToken::JsonToken(yyjson_mut_doc* jsonDoc, yyjson_mut_val* val)
        : mutableDoc_{jsonDoc}, mutable_{true} {
        mutableVal_ = val;
    }

    yyjson_mut_val* JsonToken::getRawMutableValue() const {
        if (!mutable_) {
            throw std::runtime_error("Token is immutable");
        }
        return mutableVal_;
    }

    yyjson_val* JsonToken::getRawImmutableValue() const {
        if (mutable_) {
            throw std::runtime_error("Token is mutable");
        }
        return val_;
    }

    std::string JsonToken::toString() const {
        auto type = getObjectType();
        if (type == YYJSON_TYPE_NONE) {
            return "Null(Nullptr)";
        }

        if (type == YYJSON_TYPE_NULL) {
            return "Null";
        }

        if (type == YYJSON_TYPE_NUM) {
            return std::to_string(toDouble());
        }

        return mutable_ ? yyjson_mut_get_str(mutableVal_) : yyjson_get_str(val_);
    }

    double_t JsonToken::toDouble() const {
        checkObjectType(YYJSON_TYPE_NUM);
        return mutable_ ? yyjson_mut_get_num(mutableVal_) : yyjson_get_num(val_);
    }

    float_t JsonToken::toFloat() const {
        return static_cast<float_t>(toDouble());
    }

    int8_t JsonToken::toInt8() const {
        return static_cast<int8_t>(toInt64());
    }

    int16_t JsonToken::toInt16() const {
        return static_cast<int16_t>(toInt64());
    }

    int32_t JsonToken::toInt32() const {
        return static_cast<int32_t>(toInt64());
    }

    int64_t JsonToken::toInt64() const {
        checkObjectType(YYJSON_TYPE_NUM);
        return mutable_ ? yyjson_mut_get_int(mutableVal_) : yyjson_get_int(val_);
    }

    uint8_t JsonToken::toUint8() const {
        return static_cast<uint8_t>(toInt64());
    }

    uint16_t JsonToken::toUint16() const {
        return static_cast<uint16_t>(toInt64());
    }

    uint32_t JsonToken::toUint32() const {
        return static_cast<uint32_t>(toInt64());
    }

    uint64_t JsonToken::toUint64() const {
        return static_cast<uint64_t>(toInt64());
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const std::string& value) {
        return JsonToken{jsonDoc.getRawMutableValue(), yyjson_mut_str(jsonDoc.getRawMutableValue(), value.c_str())};
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const double_t& value) {
        return JsonToken{jsonDoc.getRawMutableValue(), yyjson_mut_real(jsonDoc.getRawMutableValue(), value)};
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const float_t& value) {
        return JsonToken{jsonDoc.getRawMutableValue(), yyjson_mut_real(jsonDoc.getRawMutableValue(), (double_t) value)};
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const int64_t& value) {
        return JsonToken{jsonDoc.getRawMutableValue(), yyjson_mut_int(jsonDoc.getRawMutableValue(), value)};
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const uint64_t& value) {
        return from(jsonDoc, static_cast<int64_t>(value));
    }

    JsonToken JsonToken::from(JsonDocument& jsonDoc, const JsonToken& value) {
        // TODO
        throw std::runtime_error("method not impl");
    }

    JsonToken JsonToken::fromNull(JsonDocument& jsonDoc) {
        return JsonToken{jsonDoc.getRawMutableValue(), yyjson_mut_null(jsonDoc.getRawMutableValue())};
    }

    void JsonToken::forEach(const ForEachItemAction& action) const {
        checkObjectType(YYJSON_TYPE_ARR);

        if (mutable_) {
            size_t index, max;
            yyjson_mut_val* item;
            yyjson_mut_arr_foreach(mutableVal_, index, max, item) {
                if (action(JsonToken{mutableDoc_, item})) break;
            }

            return;
        }

        size_t index, max;
        yyjson_val* item;
        yyjson_arr_foreach(val_, index, max, item) {
            if (action(JsonToken{item})) break;
        }
    }

    void JsonToken::forEach(const ForEachKeyValueAction& action) const {
        checkObjectType(YYJSON_TYPE_OBJ);

        if (mutable_) {
            yyjson_mut_obj_iter iter;
            yyjson_mut_obj_iter_init(mutableVal_, &iter);
            yyjson_mut_val* key;
            while ((key = yyjson_mut_obj_iter_next(&iter))) {
                if (action(yyjson_mut_get_str(key), JsonToken{mutableDoc_, yyjson_mut_obj_iter_get_val(key)})) break;
            }

            return;
        }

        yyjson_obj_iter iter;
        yyjson_obj_iter_init(val_, &iter);
        yyjson_val* key;
        while ((key = yyjson_obj_iter_next(&iter))) {
            if (action(yyjson_get_str(key), JsonToken{yyjson_obj_iter_get_val(key)})) break;
        }
    }

    void JsonToken::appendChild(const JsonToken& token) {
        if (!mutable_) {
            throw std::runtime_error("Token is immutable");
        }

        checkObjectType(YYJSON_TYPE_ARR);
        yyjson_mut_arr_add_val(mutableVal_, token.getRawMutableValue());
    }

    void JsonToken::appendChild(std::string_view key, const JsonToken& token) {
        if (!mutable_) {
            throw std::runtime_error("Token is immutable");
        }

        checkObjectType(YYJSON_TYPE_OBJ);
        yyjson_mut_obj_add_val(mutableDoc_, mutableVal_, key.data(), token.getRawMutableValue());
    }

    JsonToken JsonDocument::getRootToken() const {
        if (mutable_) {
            return JsonToken{mutableDoc_.get(), yyjson_mut_doc_get_root(mutableDoc_.get())};
        }

        return JsonToken{yyjson_doc_get_root(doc_.get())};
    }

    JsonDocument::JsonDocument(const std::string_view content, const bool openMutable): mutable_{openMutable} {
        yyjson_read_err error;
        doc_ = {
            yyjson_read_opts(const_cast<char*>(content.data()), content.size(),
                             YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS, nullptr, &error),
            yyjsonDeleter
        };

        if (!doc_) {
            std::stringstream msg;
            msg << "read error: " << error.msg << ", code: " << error.code << ", pos: " << error.pos;
            throw std::runtime_error(msg.str());
        }

        if (openMutable) {
            mutableDoc_ = {yyjson_doc_mut_copy(doc_.get(), nullptr), yyjsonDeleter};
        }
    }

    JsonDocument::JsonDocument()
        : mutableDoc_{yyjson_mut_doc_new(nullptr), yyjsonDeleter}, mutable_{true} {
    }

    yyjson_mut_doc* JsonDocument::getRawMutableValue() const {
        return mutableDoc_.get();
    }

    yyjson_doc* JsonDocument::getRawImmutableValue() const {
        return doc_.get();
    }

    void JsonDocument::setRoot(const JsonToken& rootToken) {
        if (!mutable_) {
            throw std::runtime_error("Document is immutable");
        }

        yyjson_mut_doc_set_root(mutableDoc_.get(), rootToken.getRawMutableValue());
    }

    std::string JsonDocument::stringify() {
        if (!mutable_) {
            return std::string{yyjson_write(doc_.get(), 0, nullptr)};
        }

        return std::string{yyjson_mut_write(mutableDoc_.get(), 0, nullptr)};
    }
}
