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

    JsonToken::JsonToken(yyjson_val* val) {
        val_ = val;
    }

    JsonToken::JsonToken(yyjson_mut_val* val): mutable_{true} {
        mutableVal_ = val;
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

    void JsonToken::forEach(const ForEachItemAction& action) const {
        checkObjectType(YYJSON_TYPE_ARR);

        if (mutable_) {
            size_t index, max;
            yyjson_mut_val* item;
            yyjson_mut_arr_foreach(mutableVal_, index, max, item) {
                if (action(JsonToken{item})) break;
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
                if (action(yyjson_mut_get_str(key), JsonToken{yyjson_mut_obj_iter_get_val(key)})) break;
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

    JsonToken JsonDocument::getRootToken() const {
        if (mutable_) {
            return JsonToken{yyjson_mut_doc_get_root(mutableDoc_.get())};
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
}
