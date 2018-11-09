#include "types.hpp"
#include "ast/ast.hpp"
#include "utils/reporting.hpp"
#include "queries/typeresolution.hpp"
#include "analyze/astqueries.hpp"
#include "module/module.hpp"

#include <cstring>
#include <cassert>
#include <algorithm>

using namespace std;

TypeInfo::TypeInfo()
    : baseType{ BaseType::Unknown }
{
}

TypeInfo TypeInfo::makeUnknown()
{
    return TypeInfo(BaseType::Unknown);
}

TypeInfo TypeInfo::makeUndefined()
{
    return TypeInfo(BaseType::Undefined);
}

TypeInfo TypeInfo::makeNull()
{
    return TypeInfo(BaseType::Null);
}

TypeInfo TypeInfo::makeNumber()
{
    return TypeInfo(BaseType::Number);
}

TypeInfo TypeInfo::makeString()
{
    return TypeInfo(BaseType::String);
}

TypeInfo TypeInfo::makeString(const string& value)
{
    return TypeInfo(BaseType::String, make_shared<LiteralTypeInfo>((void*)&value));
}

TypeInfo TypeInfo::makeBoolean()
{
    return TypeInfo(BaseType::Boolean);
}

TypeInfo TypeInfo::makeObject(std::unordered_map<std::string, TypeInfo>&& props, bool strict)
{
    return TypeInfo(BaseType::Object, make_shared<ObjectTypeInfo>(move(props), strict));
}

TypeInfo TypeInfo::makeFunction(Function& decl)
{
    return TypeInfo{BaseType::Function, make_shared<FunctionTypeInfo>(decl)};
}

TypeInfo TypeInfo::makeFunction(std::vector<TypeInfo> &&argumentTypes, TypeInfo returnType, bool variadic)
{
    return TypeInfo{BaseType::Function, make_shared<FunctionTypeInfo>(move(argumentTypes), move(returnType), variadic)};
}

TypeInfo TypeInfo::makeClass(Class &decl)
{
    auto extra = decl.getParentModule().getClassExtraTypeInfo(decl);
    return TypeInfo{BaseType::Class, std::move(extra)};
}

template <>
const FunctionTypeInfo* TypeInfo::getExtra() const { return ((FunctionTypeInfo*)extra.get())->ensureLazyInit(); }
template <>
const PromiseTypeInfo* TypeInfo::getExtra() const { return (PromiseTypeInfo*)extra.get(); }
template <>
const SumTypeInfo* TypeInfo::getExtra() const { return (SumTypeInfo*)extra.get(); }
template <>
const ObjectTypeInfo* TypeInfo::getExtra() const { return (ObjectTypeInfo*)extra.get(); }
template <>
const ClassTypeInfo* TypeInfo::getExtra() const { return ((ClassTypeInfo*)extra.get())->ensureLazyInit(); }
template <>
const string* TypeInfo::getExtra() const { return (const string*)((LiteralTypeInfo*)extra.get())->data; }


TypeInfo::TypeInfo(BaseType baseType, std::shared_ptr<ExtraTypeInfo>&& extra)
    : baseType{baseType}, extra{move(extra)}
{
}

FunctionTypeInfo::FunctionTypeInfo(Function& decl)
    : staticDefinition { &decl }, lazyInitDone{false}
{
    // Actual init is done lazily
}

FunctionTypeInfo::FunctionTypeInfo(std::vector<TypeInfo> &&argumentTypes, TypeInfo returnType, bool variadic)
    : staticDefinition { nullptr }, argumentTypes{move(argumentTypes)}, returnType{returnType}, variadic{variadic}, lazyInitDone{true}
{
    GenericHash gh;
    for (const auto& arg : argumentTypes)
        arg.hash(gh);
    returnType.hash(gh);
    gh.update(&variadic, sizeof(variadic));
    gh.final(hash);
}

FunctionTypeInfo *FunctionTypeInfo::ensureLazyInit()
{
    if (lazyInitDone)
        return this;
    lazyInitDone = true;

    if (staticDefinition) {
        for (auto param : staticDefinition->getParams()) {
            TypeInfo paramType;
            if (param->getType() == AstNodeType::Identifier) {
                auto id = (Identifier*)param;
                if (const auto& typeAnnotation = id->getTypeAnnotation())
                    paramType = resolveAstAnnotationType(*typeAnnotation->getTypeAnnotation());
            } else {
                trace("Cannot handle non-identifier parameter type");
            }

            argumentTypes.push_back(paramType);
        }

        variadic = !staticDefinition->getParams().empty() && staticDefinition->getParams().back()->getType() == AstNodeType::RestElement;

        if (auto typeAnnotation = staticDefinition->getReturnTypeAnnotation()) {
            returnType = resolveAstAnnotationType(*typeAnnotation);
            if (staticDefinition->isAsync())
                returnType = TypeInfo::makePromise(returnType);
        } else {
            returnType = resolveReturnType(*staticDefinition);
        }
    }

    GenericHash gh;
    for (const auto& arg : argumentTypes)
        arg.hash(gh);
    returnType.hash(gh);
    gh.update(&variadic, sizeof(variadic));
    gh.final(hash);

    return this;
}

ClassTypeInfo::ClassTypeInfo(Class &decl)
    : staticDefinition { &decl }, lazyInitDone{false}
{
    // Actual init is done lazily
}

ClassTypeInfo *ClassTypeInfo::ensureLazyInit()
{
    if (lazyInitDone)
        return this;
    lazyInitDone = true;

    strict = false;
    // TODO: Implement resolution of more static properties
    // For classes, we can fill in all the static properties declared as class properties + all the methods
    // For the constructor of classes (like when calling new on a function), we need to look at all the property stores on "this" to gather the other properties
    // (if we have a class without ctor/implement/extends, we cab mark it as strict, otherwise we can just do that later (it's prolly possible for some simple ctors to deduce it's strict))
    // We should also add properties defined through implements/extends if any

    for (auto node : staticDefinition->getBody()->getBody()) {
        if (node->getType() == AstNodeType::ClassMethod) {
            auto method = (ClassMethod*)node;
            if (method->getKind() == ClassMethod::Kind::Method || method->getKind() == ClassMethod::Kind::Constructor) {
                properties[method->getKey()->getName()] = TypeInfo::makeFunction((Function&)*node);
            } else if (method->getKind() == ClassMethod::Kind::Get) {
                auto getterType = TypeInfo::makeFunction((Function&)*node);
                properties[method->getKey()->getName()] = getterType.getExtra<FunctionTypeInfo>()->returnType;
            } else if (method->getKind() == ClassMethod::Kind::Set) {
                auto setterType = TypeInfo::makeFunction((Function&)*node);
                assert(setterType.getExtra<FunctionTypeInfo>()->argumentTypes.size() == 1);
                properties[method->getKey()->getName()] = setterType.getExtra<FunctionTypeInfo>()->argumentTypes[0];
            } else {
                assert(false);
            }
        } else if (node->getType() == AstNodeType::ClassPrivateMethod) {
            auto method = (ClassPrivateMethod*)node;
            if (method->getKind() == ClassPrivateMethod::Kind::Method) {
                properties[method->getKey()->getName()] = TypeInfo::makeFunction((Function&)*node);
            } else if (method->getKind() == ClassPrivateMethod::Kind::Get) {
                auto getterType = TypeInfo::makeFunction((Function&)*node);
                properties[method->getKey()->getName()] = getterType.getExtra<FunctionTypeInfo>()->returnType;
            } else if (method->getKind() == ClassPrivateMethod::Kind::Set) {
                auto setterType = TypeInfo::makeFunction((Function&)*node);
                assert(setterType.getExtra<FunctionTypeInfo>()->argumentTypes.size() == 1);
                properties[method->getKey()->getName()] = setterType.getExtra<FunctionTypeInfo>()->argumentTypes[0];
            } else {
                assert(false);
            }
        } else if (node->getType() == AstNodeType::ClassProperty) {
            auto prop = (ClassProperty*)node;
            auto val = (AstNode*)prop->getValue();
            TypeInfo type;
            if (auto astType = prop->getTypeAnnotation())
                type = resolveAstNodeType(*astType);
            else if (val)
                type = resolveAstNodeType(*val);
            properties[prop->getKey()->getName()] = type;
        }
    }

    GenericHash gh;
    for (const auto& prop : properties) {
        gh.update(prop.first.data(), prop.first.size());
        prop.second.hash(gh);
    }
    gh.update(&strict, sizeof(strict));
    gh.final(hash);

    return this;
}

bool ClassTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    if (memcmp(hash, otherBase.hash, sizeof(hash)) == 0)
        return true;

    const auto& other = (const ClassTypeInfo&)otherBase;
    return properties == other.properties;
}

bool FunctionTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    if (memcmp(hash, otherBase.hash, sizeof(hash)) == 0)
        return true;

    const auto& other = (const FunctionTypeInfo&)otherBase;
    if (staticDefinition == other.staticDefinition)
        return true;
    return argumentTypes == other.argumentTypes && returnType == other.returnType;
}

TypeInfo TypeInfo::makePromise(const TypeInfo &nestedType)
{
    return TypeInfo{BaseType::Promise, make_shared<PromiseTypeInfo>(nestedType)};
}

TypeInfo TypeInfo::makeSum(std::vector<TypeInfo> &&types)
{
    return TypeInfo{BaseType::Sum, make_shared<SumTypeInfo>(move(types))};
}

PromiseTypeInfo::PromiseTypeInfo()
{
    GenericHash gh;
    gh.final(hash);
}

PromiseTypeInfo::PromiseTypeInfo(const TypeInfo &nestedType)
    : nestedType{nestedType}
{
    GenericHash gh;
    nestedType.hash(gh);
    gh.final(hash);
}

bool PromiseTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    if (memcmp(hash, otherBase.hash, sizeof(hash)) == 0)
        return true;

    const auto& other = (const PromiseTypeInfo&)otherBase;
    return nestedType == other.nestedType;
}

ObjectTypeInfo::ObjectTypeInfo(std::unordered_map<std::string, TypeInfo> &&properties, bool strict)
    : properties{move(properties)}, strict{strict}
{
    GenericHash gh;
    for (const auto& prop : properties) {
        gh.update(prop.first.data(), prop.first.size());
        prop.second.hash(gh);
    }
    gh.update(&strict, sizeof(strict));
    gh.final(hash);
}

bool ObjectTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    if (memcmp(hash, otherBase.hash, sizeof(hash)) == 0)
        return true;

    const auto& other = (const ObjectTypeInfo&)otherBase;
    return properties == other.properties;
}

BaseType TypeInfo::getBaseType() const
{
    return baseType;
}

const char* TypeInfo::baseTypeName() const
{
    switch (baseType) {
    case BaseType::Unknown:
        return "unknown";
    case BaseType::Sum:
        return "sum type";
    case BaseType::Undefined:
        return "undefined";
    case BaseType::Null:
        return "null";
    case BaseType::Number:
        return "number";
    case BaseType::String:
        return "string";
    case BaseType::Boolean:
        return "boolean";
    case BaseType::Object:
        return "object";
    case BaseType::Array:
        return "array";
    case BaseType::Function:
        return "function";
    case BaseType::Class:
        return "class";
    case BaseType::Promise:
        return "promise";
    }

    __builtin_unreachable();
}

bool TypeInfo::hasExtra() const
{
    return (bool)extra;
}

void TypeInfo::hash(GenericHash &gh) const
{
    gh.update(&baseType, sizeof(baseType));
    if (extra)
        gh.update(extra->hash, sizeof(ExtraTypeInfo::hash));
}

TypeInfo::operator bool() const
{
    return baseType != BaseType::Unknown;
}

bool TypeInfo::operator==(const TypeInfo &other) const
{
    if (baseType != other.baseType)
        return false;
    if (extra == other.extra)
        return true;
    if (!extra || !other.extra)
        return false;
    return *extra == *other.extra;
}

bool TypeInfo::operator<(const TypeInfo &other) const
{
    if (baseType < other.baseType)
        return true;
    if (baseType > other.baseType)
        return false;
    if (!extra)
        return true;
    if (!other.extra)
        return false;
    return *extra < *other.extra;
}

bool ExtraTypeInfo::operator<(const ExtraTypeInfo &other) const
{
    return memcmp(hash, other.hash, sizeof(ExtraTypeInfo::hash)) < 0;
}

SumTypeInfo::SumTypeInfo()
{
    GenericHash gh;
    gh.final(hash);
}

SumTypeInfo::SumTypeInfo(std::vector<TypeInfo> &&elements)
    : elements{elements}
{
    sort(begin(elements), end(elements));

    GenericHash gh;
    for (const auto& prop : elements) {
        prop.hash(gh);
    }
    gh.final(hash);
}

bool SumTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    if (memcmp(hash, otherBase.hash, sizeof(hash)) == 0)
        return true;

    const auto& other = (const SumTypeInfo&)otherBase;
    return elements == other.elements;
}

LiteralTypeInfo::LiteralTypeInfo(void *data)
    : data{data}
{
    GenericHash gh;
    // We don't hash the data, since it's a value, not a part of the type
    gh.final(hash);
}

bool LiteralTypeInfo::operator==(const ExtraTypeInfo &otherBase) const
{
    // If two literal base types are the same, the literal type info isn't going to change that!
    return true;
}
