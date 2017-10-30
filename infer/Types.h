#ifndef SRUBY_TYPES_H
#define SRUBY_TYPES_H

#include "../ast/ast.h"
#include <memory>
#include <string>

namespace ruby_typer {
namespace infer {
/** Dmitry: unlike in Dotty, those types are always dealiased. For now */
class Type;
class Types {
public:
    /** Greater lower bound: the widest type that is subtype of both t1 and t2 */
    static std::shared_ptr<Type> glb(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &t2);
    inline static std::shared_ptr<Type> glb(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &&t2) {
        return glb(ctx, t1, t2);
    }
    inline static std::shared_ptr<Type> glb(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &t2) {
        return glb(ctx, t1, t2);
    }
    inline static std::shared_ptr<Type> glb(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &&t2) {
        return glb(ctx, t1, t2);
    }

    /** Lower upper bound: the narrowest type that is supper type of both t1 and t2 */
    static std::shared_ptr<Type> lub(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &t2);
    inline static std::shared_ptr<Type> lub(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &&t2) {
        return lub(ctx, t1, t2);
    }
    inline static std::shared_ptr<Type> lub(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &t2) {
        return lub(ctx, t1, t2);
    }
    inline static std::shared_ptr<Type> lub(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &&t2) {
        return lub(ctx, t1, t2);
    }

    /** is every instance of  t1 an  instance of t2? */
    static bool isSubType(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &t2);
    inline static bool isSubType(ast::Context ctx, std::shared_ptr<Type> &t1, std::shared_ptr<Type> &&t2) {
        return isSubType(ctx, t1, t2);
    }
    inline static bool isSubType(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &t2) {
        return isSubType(ctx, t1, t2);
    }
    inline static bool isSubType(ast::Context ctx, std::shared_ptr<Type> &&t1, std::shared_ptr<Type> &&t2) {
        return isSubType(ctx, t1, t2);
    }
};

class Type {
public:
    Type() = default;
    Type(const Type &obj) = delete;
    virtual ~Type() = default;
    virtual std::string toString(ast::Context ctx, int tabs = 0) = 0;
    virtual std::string typeName() = 0;
};

class GroundType : public Type {
public:
    virtual int kind() = 0;
};

class ProxyType : public Type {
public:
    // TODO: use shared pointers that use inline counter
    std::shared_ptr<Type> underlying;
    ProxyType(std::shared_ptr<Type> underlying);
};

class ClassType : public GroundType {
public:
    ast::SymbolRef symbol;
    ClassType(ast::SymbolRef symbol);
    virtual int kind();

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class MethodType : public GroundType {
public:
    ast::SymbolRef symbol;
    virtual int kind();
    MethodType(ast::SymbolRef symbol);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class OrType : public GroundType {
public:
    std::shared_ptr<Type> left;
    std::shared_ptr<Type> right;
    virtual int kind();
    OrType(std::shared_ptr<Type> left, std::shared_ptr<Type> right);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class AndType : public GroundType {
public:
    std::shared_ptr<Type> left;
    std::shared_ptr<Type> right;
    virtual int kind();
    AndType(std::shared_ptr<Type> left, std::shared_ptr<Type> right);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class Literal : public ProxyType {
public:
    int value;
    Literal(int val);
    Literal(float val);
    Literal(ast::NameRef val);
    Literal(bool val);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class HashType : public ProxyType {
public:
    std::vector<std::shared_ptr<Literal>> keys; // TODO: store sorted by whatever
    std::vector<std::shared_ptr<Type>> values;
    HashType(std::vector<std::shared_ptr<Literal>> keys, std::vector<std::shared_ptr<Type>> values);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

class ArrayType : public ProxyType {
public:
    std::vector<std::shared_ptr<Type>> elems;
    ArrayType(std::vector<std::shared_ptr<Type>> elems);

    virtual std::string toString(ast::Context ctx, int tabs);
    virtual std::string typeName();
};

} // namespace infer
} // namespace ruby_typer
#endif // SRUBY_TYPES_H
