#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include "hash.hpp"

class AstNode;
class Function;
class Class;

enum class BaseType {
    Unknown = 0,
    Sum,
    Undefined,
    Null,
    Number,
    String,
    Boolean,
    Object,
    Array,
    Function,
    Class,
    Promise,
};

struct ExtraTypeInfo
{
    virtual ~ExtraTypeInfo() = default;
    // We assume that binary operators on ExtraTypeInfo always apply to two objects of the same derived types!
    // TODO: Replace the virtual implemetations by just comparing the hash, it's large/high quality enough to not collide
    virtual bool operator==(const ExtraTypeInfo& otherBase) const = 0;
    virtual bool operator<(const ExtraTypeInfo& other) const;

    uint8_t hash[GenericHash::hashsize]; // Subclasses of ExtraTypeInfo should compute this hash in their constructor!
};

struct FunctionTypeInfo;
struct ClassTypeInfo;
struct PromiseTypeInfo;
struct SumTypeInfo;
struct ObjectTypeInfo;
struct LiteralTypeInfo;

struct TypeInfo
{
public:
    TypeInfo(); // Equivalent to makeUnknown()
    static TypeInfo makeUnknown();
    static TypeInfo makeUndefined();
    static TypeInfo makeNull();
    static TypeInfo makeNumber();
    static TypeInfo makeBoolean();
    static TypeInfo makeString();
    static TypeInfo makeString(const std::string& value); // Value must live as long as the TypeInfo!
    static TypeInfo makeObject(std::unordered_map<std::string, TypeInfo>&& props, bool strict);
    static TypeInfo makeFunction(Function& decl);
    static TypeInfo makeFunction(std::vector<TypeInfo>&& argumentTypes, TypeInfo returnType);
    static TypeInfo makeClass(Class& decl);
    static TypeInfo makePromise(const TypeInfo& nestedType);
    static TypeInfo makeSum(std::vector<TypeInfo>&& types);

    BaseType getBaseType() const;
    const char* baseTypeName() const;
    bool hasExtra() const;
    template <class E> E* getExtra();
    void hash(GenericHash& gh) const; // Udpates gh with the hash of this type

    operator bool() const; // True iff base type is not unknown
    bool operator==(const TypeInfo& other) const;
    bool operator<(const TypeInfo& other) const;

private:
    TypeInfo(BaseType baseType, std::shared_ptr<ExtraTypeInfo>&& extra = {});

private:
    BaseType baseType;
    std::shared_ptr<ExtraTypeInfo> extra;
};

template <>
FunctionTypeInfo* TypeInfo::getExtra();
template <>
PromiseTypeInfo* TypeInfo::getExtra();
template <>
SumTypeInfo* TypeInfo::getExtra();
template <>
ObjectTypeInfo* TypeInfo::getExtra();
template <>
ClassTypeInfo* TypeInfo::getExtra();
template <>
const std::string* TypeInfo::getExtra();

struct FunctionTypeInfo : public ExtraTypeInfo
{
    FunctionTypeInfo(Function& decl);
    FunctionTypeInfo(std::vector<TypeInfo>&& argumentTypes, TypeInfo returnType);
    FunctionTypeInfo* ensureLazyInit();
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    Function* staticDefinition;
    std::vector<TypeInfo> argumentTypes;
    TypeInfo returnType;

private:
    bool lazyInitDone;
};

struct ClassTypeInfo : public ExtraTypeInfo
{
    ClassTypeInfo(Class& decl);
    ClassTypeInfo* ensureLazyInit();
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    Class* staticDefinition;
    std::unordered_map<std::string, TypeInfo> properties;
    bool strict; // If false, the object value may have extra properties not described in the type

private:
    bool lazyInitDone;
};

struct PromiseTypeInfo : public ExtraTypeInfo
{
    PromiseTypeInfo();
    PromiseTypeInfo(const TypeInfo& nestedType);
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    TypeInfo nestedType;
};

struct SumTypeInfo : public ExtraTypeInfo
{
    SumTypeInfo();
    SumTypeInfo(std::vector<TypeInfo>&& elements);
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    std::vector<TypeInfo> elements; // This vector must stay sorted (for comparisons)
};

struct ObjectTypeInfo : public ExtraTypeInfo
{
    ObjectTypeInfo(std::unordered_map<std::string, TypeInfo>&& properties, bool strict);
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    std::unordered_map<std::string, TypeInfo> properties;
    bool strict; // If false, the object value may have extra properties not described in the type
    // TODO: FIXME: If an object escapes or might alias with another object, we need to remove the strict flag, and assume it can change after every function call/aliasing property store!
};

struct LiteralTypeInfo : public ExtraTypeInfo
{
    LiteralTypeInfo(void* data);
    virtual bool operator==(const ExtraTypeInfo& otherBase) const override;

    void* data;
};

#endif // TYPES_HPP
