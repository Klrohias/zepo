//
// Created by qingy on 2024/7/14.
//

#pragma once
#ifndef ZEPO_REFLECT_HPP
#define ZEPO_REFLECT_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>

namespace zepo {
    inline void checkTypeMatch(const std::type_index& fieldType, std::string_view fieldName,
                               const std::type_index& requiredType) {
        if (requiredType != fieldType) {
            std::string exceptionMessage;
            exceptionMessage.append("Field type mismatch: \"");
            exceptionMessage.append(fieldType.name());
            exceptionMessage.append(" ");
            exceptionMessage.append(fieldName);
            exceptionMessage.append("\" and \"");
            exceptionMessage.append(requiredType.name());
            exceptionMessage.append("\"");
            throw std::runtime_error(exceptionMessage);
        }
    }

    template<typename Type>
    struct ReflectMetadata {
        using ValueSetter = std::function<void(void* self, void* value)>;
        using ValueGetter = std::function<void*(void* self)>;
        using TargetType = Type;

        struct Field {
            std::string name;
            std::type_index type{typeid(void)};
            ValueSetter setter;
            ValueGetter getter;
        };

    private:
        std::vector<Field> fields_{};

    public:
        template<typename FieldReference>
        void addField(std::string name, FieldReference reference) {
            using FieldType = decltype(TargetType{}.*reference);

            auto setter = [reference](void* self, void* value) {
                *static_cast<TargetType*>(self).*reference = *const_cast<const FieldType*>(static_cast<FieldType*>(
                    value));
            };

            auto getter = [reference](void* self) {
                return static_cast<void*>(&(*static_cast<TargetType*>(self).*reference));
            };

            fields_.push_back(Field{name, typeid(FieldType), setter, getter});
        }

        const Field& findField(std::string_view name) {
            auto fieldIter = std::find_if(fields_.begin(), fields_.end(),
                                          [&name](const Field& it) { return it.name == name; });

            if (fieldIter == fields_.end()) {
                throw std::runtime_error("Failed to find field \"" + std::string(name) + "\"");
            }

            return *fieldIter;
        }

        template<typename FieldType>
        FieldType getField(Type& instance, std::string_view name) {
            const auto& fieldInfo = findField(name);
            checkTypeMatch(fieldInfo.type, fieldInfo.name, typeid(FieldType));

            return *static_cast<const FieldType*>(fieldInfo.getter(&instance));
        }

        template<typename FieldType>
        void setField(Type& instance, std::string_view name, const FieldType& value) {
            const auto& fieldInfo = findField(name);
            checkTypeMatch(fieldInfo.type, fieldInfo.name, typeid(FieldType));

            fieldInfo.setter(&instance, const_cast<FieldType*>(&value));
        }
    };

    template<typename Type>
    struct MetadataHandler {
        ReflectMetadata<Type> metadata{};

        template<auto Name, auto FieldReference>
        void field() {
            metadata.addField(Name(), FieldReference);
        }
    };

    template<typename Type, typename Handler>
    struct ReflectTraits : Handler {
        using Handler::Handler;

        void handle() {
            throw std::runtime_error("Unknown type: " + std::string(typeid(Type).name()));
        }
    };

    template<typename Type>
    auto MetadataOf = ReflectMetadata<Type>{};
}


#ifndef ZEPO_NO_MACROS

#define ZEPO_REFLECT_ACCESSABLE_(TYPE_) template<> \
zepo::ReflectMetadata<TYPE_> zepo::MetadataOf<TYPE_> = []() { \
    static zepo::ReflectTraits<TYPE_, zepo::MetadataHandler<TYPE_>> metadataHandler{}; \
    metadataHandler.handle(); \
    return metadataHandler.metadata; \
}();

#define ZEPO_REFLECT_INFO_BEGIN_(TYPE_) template<typename Handler> \
    struct zepo::ReflectTraits<TYPE_, Handler> : Handler { \
        using CurrentType = TYPE_; \
        using Handler::Handler; \
        void handle() {

#define ZEPO_REFLECT_FIELD_(FIELD_) this->template field<[] { return (#FIELD_); }, &CurrentType::##FIELD_>()

#define ZEPO_REFLECT_INFO_END_()     } \
};

#endif

#endif //ZEPO_REFLECT_HPP
