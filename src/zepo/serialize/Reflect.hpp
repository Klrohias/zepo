//
// Created by qingy on 2024/7/14.
//

#pragma once
#ifndef ZEPO_REFLECT_HPP
#define ZEPO_REFLECT_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>

namespace zepo {
    inline void checkTypeMatch(const std::type_index& fieldType, std::string_view fieldName,
                               const std::type_index& requiredType) {
        using namespace std::string_literals;
        if (requiredType != fieldType) {
            throw std::runtime_error(
                "Field type mismatch: \""s + fieldType.name() + " " + std::string(fieldName) + "\" and \"" +
                requiredType.name() + "\"");
        }
    }

    using ValueSetter = std::function<void(void* self, void* value)>;
    using ValueGetter = std::function<void*(void* self)>;
    using AttributeGetter = std::function<void*()>;

    struct FieldInfo;
    struct AttributeInfo;

    struct AttributeInfo {
        std::type_index type{typeid(void)};
        AttributeGetter getter;
    };

    struct FieldInfo {
        std::string name;
        std::type_index type{typeid(void)};
        ValueSetter setter;
        ValueGetter getter;
        std::vector<AttributeInfo> attributes;

        [[nodiscard]] void* findAttribute(std::type_index type) const {
            const auto iter = std::ranges::find_if(attributes, [&type](const AttributeInfo& it) {
                return it.type == type;
            });

            if (iter == attributes.end()) {
                return nullptr;
            }

            return iter->getter();
        }

        template<typename AttributeType>
        AttributeType& findAttribute() const {
            return *static_cast<AttributeType*>(findAttribute(typeid(AttributeType)));
        }
    };

    template<typename Type>
    struct TypeInfo {
        using TargetType = Type;

    private:
        std::vector<FieldInfo> fields_{};

    public:
        template<typename FieldReference>
        FieldInfo& addField(std::string name, FieldReference reference) {
            using FieldType = decltype(TargetType{}.*reference);

            auto setter = [reference](void* self, void* value) {
                *static_cast<TargetType*>(self).*reference = *const_cast<const FieldType*>(static_cast<FieldType*>(
                    value));
            };

            auto getter = [reference](void* self) {
                return static_cast<void*>(&(*static_cast<TargetType*>(self).*reference));
            };

            auto field = FieldInfo{name, typeid(FieldType), setter, getter};
            return fields_.emplace_back(field);
        }

        const FieldInfo& findField(std::string_view name) {
            auto fieldIter = std::find_if(fields_.begin(), fields_.end(),
                                          [&name](const FieldInfo& it) { return it.name == name; });

            if (fieldIter == fields_.end()) {
                throw std::runtime_error("Failed to find field \"" + std::string(name) + "\"");
            }

            return *fieldIter;
        }

        const FieldInfo* tryFindField(std::string_view name) {
            auto fieldIter = std::find_if(fields_.begin(), fields_.end(),
                                          [&name](const FieldInfo& it) { return it.name == name; });

            if (fieldIter == fields_.end()) {
                return nullptr;
            }

            return std::addressof(*fieldIter);
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
    struct MetadataGenerator {
        TypeInfo<Type> metadata{};
        std::vector<AttributeInfo> pendingAttributes{};

        template<auto Name, auto FieldReference>
        void field() {
            auto& fieldInfo = metadata.addField(Name(), FieldReference);
            fieldInfo.attributes = std::move(pendingAttributes);
            pendingAttributes = {};
        }

        template<auto Attribute>
        void attribute() {
            pendingAttributes.push_back(AttributeInfo{
                typeid(decltype(*Attribute())),
                Attribute
            });
        }
    };

    template<typename Type, typename Handler>
    struct TypeDefTraits : Handler {
        using Handler::Handler;

        void execute() {
            throw std::runtime_error("Unknown type: " + std::string(typeid(Type).name()));
        }
    };

    template<typename Type>
    auto metadataOf = TypeInfo<Type>{};
}


#ifndef ZEPO_NO_MACROS

#define ZR_MakeMetadata(TYPE_) template<> \
zepo::TypeInfo<TYPE_> zepo::metadataOf<TYPE_> = []() { \
    static zepo::TypeDefTraits<TYPE_, zepo::MetadataGenerator<TYPE_>> metadataHandler{}; \
    metadataHandler.execute(); \
    return metadataHandler.metadata; \
}();

#define ZR_BeginDef(TYPE_) template<typename Handler> \
    struct zepo::TypeDefTraits<TYPE_, Handler> : Handler { \
        using CurrentType = TYPE_; \
        using Handler::Handler; \
        void execute() {

#define ZR_Attribute(ATTRIBUTE_) this->template attribute<[] { \
    static auto currentAttribute{ATTRIBUTE_}; \
    return &currentAttribute; \
}>()

#define ZR_Field(FIELD_) this->template field<[] { return (#FIELD_); }, &CurrentType::##FIELD_>()

#define ZR_EndDef()     } \
};

#endif //ZEPO_NO_MACROS

#endif //ZEPO_REFLECT_HPP
