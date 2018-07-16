#include "GlobalState.h"

#include "Errors.h"
#include "Types.h"
#include "core/Names/core.h"
#include "core/errors/errors.h"
#include <utility>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"

template class std::vector<std::pair<unsigned int, unsigned int>>;
template class std::shared_ptr<sorbet::core::GlobalState>;
template class std::unique_ptr<sorbet::core::GlobalState>;

using namespace std;

namespace sorbet {
namespace core {

namespace {
const char *top_str = "<any>";
const char *bottom_str = "<impossible>";
const char *untyped_str = "T.untyped";
const char *root_str = "<root>";
const char *object_str = "Object";
const char *string_str = "String";
const char *integer_str = "Integer";
const char *float_str = "Float";
const char *symbol_str = "Symbol";
const char *array_str = "Array";
const char *hash_str = "Hash";
const char *proc_str = "Proc";
const char *trueClass_str = "TrueClass";
const char *falseClass_str = "FalseClass";
const char *nilClass_str = "NilClass";
const char *class_str = "Class";
const char *module_str = "Module";
const char *todo_str = "<todo sym>";
const char *no_symbol_str = "<none>";
const char *opus_str = "Opus";
const char *T_str = "T";
const char *basicObject_str = "BasicObject";
const char *kernel_str = "Kernel";
const char *range_str = "Range";
const char *regexp_str = "Regexp";
const char *standardError_str = "StandardError";
const char *complex_str = "Complex";
const char *rational_str = "Rational";
// A magic non user-creatable class with methods to keep state between passes
const char *magic_str = "<Magic>";
const char *enumerable_str = "Enumerable";
const char *set_str = "Set";
const char *struct_str = "Struct";
const char *file_str = "File";
const char *ruby_typer_str = "RubyTyper";
const char *stub_str = "StubClass";
const char *configatron_str = "Configatron";
const char *store_str = "Store";
const char *root_store_str = "RootStore";
const char *sinatra_str = "Sinatra";
const char *base_str = "Base";
const char *void_str = "Void";
const char *typeAliasTemp_str = "<TypeAlias>";
const char *chalk_str = "Chalk";
const char *tools_str = "Tools";
const char *accessible_str = "Accessible";
const char *generic_str = "Generic";
const char *tuple_str = "Tuple";
const char *subclasses_str = "SUBCLASSES";

// This fills in all the way up to MAX_SYNTHETIC_SYMBOLS
const char *reserved_str = "<<RESERVED>>";
} // namespace

SymbolRef GlobalState::synthesizeClass(absl::string_view name, u4 superclass, bool isModule) {
    NameRef nameId = enterNameConstant(name);

    // This can't use enterClass since there is a chicken and egg problem.
    // These will be added to Symbols::root().members later.
    SymbolRef symRef = SymbolRef(this, symbols.size());
    symbols.emplace_back();
    Symbol &data = symRef.data(*this, true); // allowing noSymbol is needed because this enters noSymbol.
    data.name = nameId;
    data.owner = Symbols::root();
    data.superClass = SymbolRef(this, superclass);
    data.flags = 0;
    data.setClass();
    data.setIsModule(isModule);

    if (symRef._id > Symbols::root()._id) {
        Symbols::root().data(*this, true).members.emplace_back(nameId, symRef);
    }
    return symRef;
}

int globalStateIdCounter = 1;
const int Symbols::MAX_PROC_ARITY;

GlobalState::GlobalState(shared_ptr<ErrorQueue> errorQueue)
    : globalStateId(globalStateIdCounter++), errorQueue(move(errorQueue)) {
    // Empirically determined to be the smallest powers of two larger than the
    // values required by the payload
    unsigned int max_name_count = 8192;
    unsigned int max_symbol_count = 16384;

    names.reserve(max_name_count);
    symbols.reserve(max_symbol_count);
    int names_by_hash_size = 2 * max_name_count;
    names_by_hash.resize(names_by_hash_size);
    ENFORCE((names_by_hash_size & (names_by_hash_size - 1)) == 0, "names_by_hash_size is not a power of 2");
}

void GlobalState::initEmpty() {
    names.emplace_back(); // first name is used in hashes to indicate empty cell
    names[0].kind = NameKind::UTF8;
    names[0].raw.utf8 = absl::string_view();
    Names::registerNames(*this);

    SymbolRef no_symbol_id = synthesizeClass(no_symbol_str, 0);
    SymbolRef top_id = synthesizeClass(top_str, 0);
    SymbolRef bottom_id = synthesizeClass(bottom_str, 0);
    SymbolRef root_id = synthesizeClass(root_str, 0);
    Symbols::root().data(*this, true).members.emplace_back(enterNameConstant(no_symbol_str), no_symbol_id);
    Symbols::root().data(*this, true).members.emplace_back(enterNameConstant(top_str), top_id);
    Symbols::root().data(*this, true).members.emplace_back(enterNameConstant(bottom_str), bottom_id);
    SymbolRef todo_id = synthesizeClass(todo_str, 0);
    SymbolRef object_id = synthesizeClass(object_str, Symbols::BasicObject()._id);
    SymbolRef integer_id = synthesizeClass(integer_str);
    SymbolRef float_id = synthesizeClass(float_str);
    SymbolRef string_id = synthesizeClass(string_str);
    SymbolRef symbol_id = synthesizeClass(symbol_str);
    SymbolRef array_id = synthesizeClass(array_str);
    SymbolRef hash_id = synthesizeClass(hash_str);
    SymbolRef trueClass_id = synthesizeClass(trueClass_str);
    SymbolRef falseClass_id = synthesizeClass(falseClass_str);
    SymbolRef nilClass_id = synthesizeClass(nilClass_str);
    SymbolRef untyped_id = synthesizeClass(untyped_str, 0);
    SymbolRef opus_id = synthesizeClass(opus_str, 0, true);

    SymbolRef T_id = synthesizeClass(T_str, Symbols::todo()._id, true);
    T_id.data(*this).setIsModule(true);

    SymbolRef class_id = synthesizeClass(class_str, 0);
    SymbolRef basicObject_id = synthesizeClass(basicObject_str, 0);
    SymbolRef kernel_id = synthesizeClass(kernel_str, 0, true);
    SymbolRef range_id = synthesizeClass(range_str);
    SymbolRef regexp_id = synthesizeClass(regexp_str);
    SymbolRef magic_id = synthesizeClass(magic_str);
    SymbolRef module_id = synthesizeClass(module_str);
    SymbolRef standardError_id = synthesizeClass(standardError_str);
    SymbolRef complex_id = synthesizeClass(complex_str);
    SymbolRef rational_id = synthesizeClass(rational_str);
    SymbolRef T_Array_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(array_str));
    SymbolRef T_Hash_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(hash_str));
    SymbolRef T_Proc_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(proc_str));
    SymbolRef proc_id = synthesizeClass(proc_str);
    SymbolRef enumerable_id = synthesizeClass(enumerable_str, 0, true);
    SymbolRef set_id = synthesizeClass(set_str);
    SymbolRef struct_id = synthesizeClass(struct_str);
    SymbolRef file_id = synthesizeClass(file_str);
    SymbolRef ruby_typer_id = synthesizeClass(ruby_typer_str, 0, true);
    SymbolRef stub_id = enterClassSymbol(Loc::none(), ruby_typer_id, enterNameConstant(stub_str));
    SymbolRef T_Enumerable_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(enumerable_str));
    SymbolRef T_Range_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(range_str));
    SymbolRef T_Set_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(set_str));
    SymbolRef configatron_id = synthesizeClass(configatron_str);
    SymbolRef configatron_store_id = enterClassSymbol(Loc::none(), configatron_id, enterNameConstant(store_str));
    SymbolRef configatron_rootstore_id =
        enterClassSymbol(Loc::none(), configatron_id, enterNameConstant(root_store_str));
    SymbolRef sinatra_id = synthesizeClass(sinatra_str, 0, true);
    SymbolRef sinatra_base_id = enterClassSymbol(Loc::none(), Symbols::Sinatra(), enterNameConstant(base_str));
    sinatra_base_id.data(*this).setIsModule(false);
    sinatra_base_id.data(*this).superClass = Symbols::Object();
    SymbolRef void_id = enterClassSymbol(Loc::none(), ruby_typer_id, enterNameConstant(void_str));
    SymbolRef typeAliasTemp_id = synthesizeClass(typeAliasTemp_str, 0);
    SymbolRef chalk_id = synthesizeClass(chalk_str, 0, true);
    SymbolRef chalk_tools_id = enterClassSymbol(Loc::none(), chalk_id, enterNameConstant(tools_str));
    SymbolRef chalk_tools_accessible_id =
        enterClassSymbol(Loc::none(), chalk_tools_id, enterNameConstant(accessible_str));
    SymbolRef T_Generic_id = enterClassSymbol(Loc::none(), Symbols::T(), enterNameConstant(generic_str));
    SymbolRef tuple_id = enterClassSymbol(Loc::none(), ruby_typer_id, enterNameConstant(tuple_str));
    SymbolRef SUBCLASSES_id = enterClassSymbol(Loc::none(), ruby_typer_id, enterNameConstant(subclasses_str));

    ENFORCE(no_symbol_id == Symbols::noSymbol());
    ENFORCE(top_id == Symbols::top());
    ENFORCE(bottom_id == Symbols::bottom());
    ENFORCE(root_id == Symbols::root());
    ENFORCE(todo_id == Symbols::todo());
    ENFORCE(object_id == Symbols::Object());
    ENFORCE(integer_id == Symbols::Integer());
    ENFORCE(float_id == Symbols::Float());
    ENFORCE(string_id == Symbols::String());
    ENFORCE(symbol_id == Symbols::Symbol());
    ENFORCE(array_id == Symbols::Array());
    ENFORCE(hash_id == Symbols::Hash());
    ENFORCE(trueClass_id == Symbols::TrueClass());
    ENFORCE(falseClass_id == Symbols::FalseClass());
    ENFORCE(nilClass_id == Symbols::NilClass());
    ENFORCE(untyped_id == Symbols::untyped());
    ENFORCE(opus_id == Symbols::Opus());
    ENFORCE(T_id == Symbols::T());
    ENFORCE(class_id == Symbols::Class());
    ENFORCE(basicObject_id == Symbols::BasicObject());
    ENFORCE(kernel_id == Symbols::Kernel());
    ENFORCE(range_id == Symbols::Range());
    ENFORCE(regexp_id == Symbols::Regexp());
    ENFORCE(magic_id == Symbols::Magic());
    ENFORCE(module_id == Symbols::Module());
    ENFORCE(standardError_id == Symbols::StandardError());
    ENFORCE(complex_id == Symbols::Complex());
    ENFORCE(rational_id == Symbols::Rational());
    ENFORCE(T_Array_id == Symbols::T_Array());
    ENFORCE(T_Hash_id == Symbols::T_Hash());
    ENFORCE(T_Proc_id == Symbols::T_Proc());
    ENFORCE(proc_id == Symbols::Proc());
    ENFORCE(enumerable_id == Symbols::Enumerable());
    ENFORCE(set_id == Symbols::Set());
    ENFORCE(struct_id == Symbols::Struct());
    ENFORCE(file_id == Symbols::File());
    ENFORCE(ruby_typer_id == Symbols::RubyTyper());
    ENFORCE(stub_id == Symbols::StubClass());
    ENFORCE(T_Enumerable_id == Symbols::T_Enumerable());
    ENFORCE(configatron_id == Symbols::Configatron());
    ENFORCE(configatron_store_id == Symbols::Configatron_Store());
    ENFORCE(configatron_rootstore_id == Symbols::Configatron_RootStore());
    ENFORCE(T_Range_id == Symbols::T_Range());
    ENFORCE(T_Set_id == Symbols::T_Set());
    ENFORCE(sinatra_id == Symbols::Sinatra());
    ENFORCE(sinatra_base_id == Symbols::SinatraBase());
    ENFORCE(void_id == Symbols::void_());
    ENFORCE(typeAliasTemp_id == Symbols::typeAliasTemp());
    ENFORCE(chalk_id == Symbols::Chalk());
    ENFORCE(chalk_tools_id == Symbols::Chalk_Tools());
    ENFORCE(chalk_tools_accessible_id == Symbols::Chalk_Tools_Accessible());
    ENFORCE(T_Generic_id == Symbols::T_Generic());
    ENFORCE(tuple_id == Symbols::Tuple());
    ENFORCE(SUBCLASSES_id == Symbols::Subclasses());

    // Synthesize untyped = T.untyped
    Symbols::untyped().data(*this).resultType = Types::untyped();

    // <Magic> has its own type
    Symbols::Magic().data(*this).resultType = make_shared<ClassType>(Symbols::Magic());

    // Synthesize <Magic>#build_hash(*vs : T.untyped) => Hash
    SymbolRef method = enterMethodSymbol(Loc::none(), Symbols::Magic(), Names::buildHash());
    SymbolRef arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg0());
    arg.data(*this).setRepeated();
    arg.data(*this).resultType = Types::untyped();
    method.data(*this).arguments().push_back(arg);
    method.data(*this).resultType = Types::hashOfUntyped();

    // Synthesize <Magic>#build_array(*vs : T.untyped) => Array
    method = enterMethodSymbol(Loc::none(), Symbols::Magic(), Names::buildArray());
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg0());
    arg.data(*this).setRepeated();
    arg.data(*this).resultType = Types::untyped();
    method.data(*this).arguments().push_back(arg);
    method.data(*this).resultType = Types::arrayOfUntyped();

    // Synthesize <Magic>#<splat>(a: Array) => Untyped
    method = enterMethodSymbol(Loc::none(), Symbols::Magic(), Names::splat());
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg0());
    arg.data(*this).resultType = Types::arrayOfUntyped();
    method.data(*this).arguments().push_back(arg);
    method.data(*this).resultType = Types::untyped();

    // Synthesize <Magic>#<defined>(arg0: Object) => Boolean
    method = enterMethodSymbol(Loc::none(), Symbols::Magic(), Names::defined_p());
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg0());
    arg.data(*this).resultType = Types::Object();
    method.data(*this).arguments().push_back(arg);
    method.data(*this).resultType = Types::Boolean();

    // Synthesize <Magic>#<expandSplat>(arg0: T.untyped, arg1: Integer, arg2: Integer) => T.untyped
    method = enterMethodSymbol(Loc::none(), Symbols::Magic(), Names::expandSplat());
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg0());
    arg.data(*this).resultType = Types::untyped();
    method.data(*this).arguments().push_back(arg);
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg1());
    arg.data(*this).resultType = Types::Integer();
    method.data(*this).arguments().push_back(arg);
    arg = enterMethodArgumentSymbol(Loc::none(), method, Names::arg2());
    arg.data(*this).resultType = Types::Integer();
    method.data(*this).arguments().push_back(arg);
    method.data(*this).resultType = Types::untyped();

    int reservedCount = 0;

    // Set the correct resultTypes for all synthesized classes
    // Does it in two passes since the singletonClass will go in the Symbols::root() members which will invalidate the
    // iterator
    vector<SymbolRef> needSingletons;
    for (auto &sym : symbols) {
        auto ref = sym.ref(*this);
        if (ref.exists() && sym.isClass()) {
            needSingletons.emplace_back(ref);
        }
    }
    for (auto sym : needSingletons) {
        sym.data(*this).singletonClass(*this);
    }

    ENFORCE(symbols.size() < Symbols::Proc0()._id);
    while (symbols.size() < Symbols::Proc0()._id) {
        string res(reserved_str);
        res = res + to_string(reservedCount);
        synthesizeClass(res);
        reservedCount++;
    }

    for (int arity = 0; arity <= Symbols::MAX_PROC_ARITY; ++arity) {
        auto id = synthesizeClass(absl::StrCat("Proc", arity), Symbols::Proc()._id);
        ENFORCE(id == Symbols::Proc(arity), "Proc creation failed for arity: ", arity, " got: ", id._id,
                " expected: ", Symbols::Proc(arity)._id);
        id.data(*this).singletonClass(*this);
    }

    ENFORCE(symbols.size() == Symbols::last_synthetic_sym()._id + 1,
            "Too many synthetic symbols? have: ", symbols.size(), " expected: ", Symbols::last_synthetic_sym()._id + 1);

    installIntrinsics();

    // First file is used to indicate absence of a file
    files.emplace_back();
    freezeNameTable();
    freezeSymbolTable();
    freezeFileTable();
    sanityCheck();
}

void GlobalState::installIntrinsics() {
    for (auto &entry : intrinsicMethods) {
        auto symbol = entry.symbol;
        if (entry.singleton) {
            symbol = symbol.data(*this).singletonClass(*this);
        }
        SymbolRef method = enterMethodSymbol(Loc::none(), symbol, entry.method);
        method.data(*this).intrinsic = entry.impl;
    }
}

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
u4 nextPowerOfTwo(u4 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void GlobalState::reserveMemory(u4 kb) {
    u8 allocated = (sizeof(Name) + sizeof(decltype(names_by_hash)::value_type)) * names.capacity() +
                   sizeof(Symbol) * symbols.capacity();
    u8 want = 1024 * kb;
    if (allocated > want) {
        return;
    }
    u4 scale = nextPowerOfTwo(want / allocated);
    symbols.reserve(symbols.capacity() * scale);
    expandNames(scale);
    sanityCheck();

    allocated = (sizeof(Name) + sizeof(decltype(names_by_hash)::value_type)) * names.capacity() +
                sizeof(Symbol) * symbols.capacity();

    trace(absl::StrCat("Reserved ", allocated / 1024, "KiB of memory. symbols=", symbols.capacity(),
                       " names=", names.capacity()));
}

constexpr decltype(GlobalState::STRINGS_PAGE_SIZE) GlobalState::STRINGS_PAGE_SIZE;

SymbolRef GlobalState::enterSymbol(Loc loc, SymbolRef owner, NameRef name, u4 flags) {
    ENFORCE(owner.exists(), "entering symbol in to non-existing owner");
    ENFORCE(name.exists(), "entering symbol with non-existing name");
    Symbol &ownerScope = owner.data(*this, true);
    auto from = ownerScope.members.begin();
    auto to = ownerScope.members.end();
    histogramInc("symbol_enter_by_name", ownerScope.members.size());

    while (from != to) {
        auto &el = *from;
        if (el.first == name) {
            ENFORCE((from->second.data(*this).flags & flags) == flags, "existing symbol has wrong flags");
            counterInc("symbols.hit");
            return from->second;
        }
        from++;
    }
    ENFORCE(!symbolTableFrozen);

    bool reallocate = symbols.size() == symbols.capacity();

    SymbolRef ret = SymbolRef(this, symbols.size());
    symbols.emplace_back();
    Symbol &data = ret.data(*this, true);
    data.name = name;
    data.flags = flags;
    data.owner = owner;
    data.definitionLoc = loc;
    if (data.isBlockSymbol(*this)) {
        categoryCounterInc("symbols", "block");
    } else if (data.isClass()) {
        categoryCounterInc("symbols", "class");
    } else if (data.isMethod()) {
        categoryCounterInc("symbols", "method");
    } else if (data.isField()) {
        categoryCounterInc("symbols", "field");
    } else if (data.isStaticField()) {
        categoryCounterInc("symbols", "static_field");
    } else if (data.isMethodArgument()) {
        categoryCounterInc("symbols", "argument");
    }

    if (!reallocate) {
        ownerScope.members.emplace_back(name, ret);
    } else {
        owner.data(*this, true).members.emplace_back(name, ret);
    }
    wasModified_ = true;
    return ret;
}

SymbolRef GlobalState::enterClassSymbol(Loc loc, SymbolRef owner, NameRef name) {
    ENFORCE(name.data(*this).isClassName(*this));
    return enterSymbol(loc, owner, name, Symbol::Flags::CLASS);
}

SymbolRef GlobalState::enterTypeMember(Loc loc, SymbolRef owner, NameRef name, Variance variance) {
    u4 flags;
    ENFORCE(owner.data(*this).isClass());
    if (variance == Variance::Invariant) {
        flags = Symbol::Flags::TYPE_INVARIANT;
    } else if (variance == Variance::CoVariant) {
        flags = Symbol::Flags::TYPE_COVARIANT;
    } else if (variance == Variance::ContraVariant) {
        flags = Symbol::Flags::TYPE_CONTRAVARIANT;
    } else {
        Error::notImplemented();
    }

    flags = flags | Symbol::Flags::TYPE_MEMBER;
    SymbolRef result = enterSymbol(loc, owner, name, flags);
    auto &members = owner.data(*this).typeMembers();
    if (find(members.begin(), members.end(), result) == members.end()) {
        members.emplace_back(result);
    }
    return result;
}

SymbolRef GlobalState::enterTypeArgument(Loc loc, SymbolRef owner, NameRef name, Variance variance) {
    u4 flags;
    if (variance == Variance::Invariant) {
        flags = Symbol::Flags::TYPE_INVARIANT;
    } else if (variance == Variance::CoVariant) {
        flags = Symbol::Flags::TYPE_COVARIANT;
    } else if (variance == Variance::ContraVariant) {
        flags = Symbol::Flags::TYPE_CONTRAVARIANT;
    } else {
        Error::notImplemented();
    }

    flags = flags | Symbol::Flags::TYPE_ARGUMENT;
    SymbolRef result = enterSymbol(loc, owner, name, flags);
    owner.data(*this).typeArguments().emplace_back(result);
    return result;
}

SymbolRef GlobalState::enterMethodSymbol(Loc loc, SymbolRef owner, NameRef name) {
    bool isBlock = name.data(*this).kind == NameKind::UNIQUE && name.data(*this).unique.original == Names::blockTemp();
    ENFORCE(isBlock || owner.data(*this).isClass(), "entering method symbol into not-a-class");
    return enterSymbol(loc, owner, name, Symbol::Flags::METHOD);
}

SymbolRef GlobalState::enterNewMethodOverload(Loc loc, SymbolRef original, u2 num) {
    NameRef name = freshNameUnique(UniqueNameKind::Overload, original.data(*this).name, num);
    SymbolRef res = enterMethodSymbol(loc, original.data(*this).owner, name);
    res.data(*this).arguments().reserve(original.data(*this).arguments().size());
    for (auto &arg : original.data(*this).arguments()) {
        Loc loc = arg.data(*this).definitionLoc;
        NameRef nm = arg.data(*this).name;
        SymbolRef newArg = enterMethodArgumentSymbol(loc, res, nm);
        newArg.data(*this).flags = arg.data(*this).flags;
        res.data(*this).arguments().push_back(newArg);
    }
    return res;
}

SymbolRef GlobalState::enterFieldSymbol(Loc loc, SymbolRef owner, NameRef name) {
    ENFORCE(owner.data(*this).isClass(), "entering field symbol into not-a-class");
    return enterSymbol(loc, owner, name, Symbol::Flags::FIELD);
}

SymbolRef GlobalState::enterStaticFieldSymbol(Loc loc, SymbolRef owner, NameRef name) {
    return enterSymbol(loc, owner, name, Symbol::Flags::STATIC_FIELD);
}

SymbolRef GlobalState::enterMethodArgumentSymbol(Loc loc, SymbolRef owner, NameRef name) {
    ENFORCE(owner.data(*this).isMethod(), "entering method argument symbol into not-a-method");
    return enterSymbol(loc, owner, name, Symbol::Flags::METHOD_ARGUMENT);
}

absl::string_view GlobalState::enterString(absl::string_view nm) {
    char *from = nullptr;
    if (nm.size() > GlobalState::STRINGS_PAGE_SIZE) {
        strings.push_back(make_unique<vector<char>>(nm.size()));
        from = strings.back()->data();
        if (strings.size() > 1) {
            swap(*(strings.end() - 1), *(strings.end() - 2));
        }
    } else {
        if (strings_last_page_used + nm.size() > GlobalState::STRINGS_PAGE_SIZE) {
            strings.push_back(make_unique<vector<char>>(GlobalState::STRINGS_PAGE_SIZE));
            // printf("Wasted %i space\n", STRINGS_PAGE_SIZE - strings_last_page_used);
            strings_last_page_used = 0;
        }
        from = strings.back()->data() + strings_last_page_used;
    }

    counterInc("strings");
    memcpy(from, nm.data(), nm.size());
    strings_last_page_used += nm.size();
    return absl::string_view(from, nm.size());
}

NameRef GlobalState::enterNameUTF8(absl::string_view nm) {
    const auto hs = _hash(nm);
    unsigned int hashTableSize = names_by_hash.size();
    unsigned int mask = hashTableSize - 1;
    auto bucketId = hs & mask;
    unsigned int probe_count = 1;

    while (names_by_hash[bucketId].second != 0u) {
        auto &bucket = names_by_hash[bucketId];
        if (bucket.first == hs) {
            auto name_id = bucket.second;
            auto &nm2 = names[name_id];
            if (nm2.kind == NameKind::UTF8 && nm2.raw.utf8 == nm) {
                counterInc("names.utf8.hit");
                return nm2.ref(*this);
            } else {
                counterInc("names.hash_collision.utf8");
            }
        }
        bucketId = (bucketId + probe_count) & mask;
        probe_count++;
    }
    ENFORCE(!nameTableFrozen);

    ENFORCE(probe_count != hashTableSize, "Full table?");

    if (names.size() == names.capacity()) {
        expandNames();
        hashTableSize = names_by_hash.size();
        mask = hashTableSize - 1;
        bucketId = hs & mask; // look for place in the new size
        probe_count = 1;
        while (names_by_hash[bucketId].second != 0) {
            bucketId = (bucketId + probe_count) & mask;
            probe_count++;
        }
    }

    auto idx = names.size();
    auto &bucket = names_by_hash[bucketId];
    bucket.first = hs;
    bucket.second = idx;
    names.emplace_back();

    names[idx].kind = NameKind::UTF8;
    names[idx].raw.utf8 = enterString(nm);
    ENFORCE(names[idx].hash(*this) == hs);
    categoryCounterInc("names", "utf8");

    wasModified_ = true;
    return NameRef(*this, idx);
}

NameRef GlobalState::enterNameConstant(NameRef original) {
    ENFORCE(original.exists(), "making a constant name over non-exiting name");
    ENFORCE(original.data(*this).kind == UTF8, "making a constant name over wrong name kind");

    const auto hs = _hash_mix_constant(CONSTANT, original.id());
    unsigned int hashTableSize = names_by_hash.size();
    unsigned int mask = hashTableSize - 1;
    auto bucketId = hs & mask;
    unsigned int probe_count = 1;

    while (names_by_hash[bucketId].second != 0 && probe_count < hashTableSize) {
        auto &bucket = names_by_hash[bucketId];
        if (bucket.first == hs) {
            auto &nm2 = names[bucket.second];
            if (nm2.kind == CONSTANT && nm2.cnst.original == original) {
                counterInc("names.constant.hit");
                return nm2.ref(*this);
            } else {
                counterInc("names.hash_collision.constant");
            }
        }
        bucketId = (bucketId + probe_count) & mask;
        probe_count++;
    }
    if (probe_count == hashTableSize) {
        Error::raise("Full table?");
    }
    ENFORCE(!nameTableFrozen);

    if (names.size() == names.capacity()) {
        expandNames();
        hashTableSize = names_by_hash.size();
        mask = hashTableSize - 1;

        bucketId = hs & mask; // look for place in the new size
        probe_count = 1;
        while (names_by_hash[bucketId].second != 0) {
            bucketId = (bucketId + probe_count) & mask;
            probe_count++;
        }
    }

    auto &bucket = names_by_hash[bucketId];
    bucket.first = hs;
    bucket.second = names.size();

    auto idx = names.size();
    names.emplace_back();

    names[idx].kind = CONSTANT;
    names[idx].cnst.original = original;
    ENFORCE(names[idx].hash(*this) == hs);
    wasModified_ = true;
    categoryCounterInc("names", "constant");
    return NameRef(*this, idx);
}

NameRef GlobalState::enterNameConstant(absl::string_view original) {
    return enterNameConstant(enterNameUTF8(original));
}

void moveNames(pair<unsigned int, unsigned int> *from, pair<unsigned int, unsigned int> *to, unsigned int szFrom,
               unsigned int szTo) {
    // printf("\nResizing name hash table from %u to %u\n", szFrom, szTo);
    ENFORCE((szTo & (szTo - 1)) == 0, "name hash table size corruption");
    ENFORCE((szFrom & (szFrom - 1)) == 0, "name hash table size corruption");
    unsigned int mask = szTo - 1;
    for (unsigned int orig = 0; orig < szFrom; orig++) {
        if (from[orig].second != 0u) {
            auto hs = from[orig].first;
            unsigned int probe = 1;
            auto bucketId = hs & mask;
            while (to[bucketId].second != 0) {
                bucketId = (bucketId + probe) & mask;
                probe++;
            }
            to[bucketId] = from[orig];
        }
    }
}

void GlobalState::expandNames(int growBy) {
    sanityCheck();

    names.reserve(names.capacity() * growBy);
    vector<pair<unsigned int, unsigned int>> new_names_by_hash(names_by_hash.capacity() * growBy);
    moveNames(names_by_hash.data(), new_names_by_hash.data(), names_by_hash.size(), new_names_by_hash.capacity());
    names_by_hash.swap(new_names_by_hash);
}

NameRef GlobalState::getNameUnique(UniqueNameKind uniqueNameKind, NameRef original, u2 num) const {
    ENFORCE(num > 0, "num == 0, name overflow");
    const auto hs = _hash_mix_unique((u2)uniqueNameKind, UNIQUE, num, original.id());
    unsigned int hashTableSize = names_by_hash.size();
    unsigned int mask = hashTableSize - 1;
    auto bucketId = hs & mask;
    unsigned int probe_count = 1;

    while (names_by_hash[bucketId].second != 0 && probe_count < hashTableSize) {
        auto &bucket = names_by_hash[bucketId];
        if (bucket.first == hs) {
            auto &nm2 = names[bucket.second];
            if (nm2.kind == UNIQUE && nm2.unique.uniqueNameKind == uniqueNameKind && nm2.unique.num == num &&
                nm2.unique.original == original) {
                counterInc("names.unique.hit");
                return nm2.ref(*this);
            } else {
                counterInc("names.hash_collision.unique");
            }
        }
        bucketId = (bucketId + probe_count) & mask;
        probe_count++;
    }
    Error::raise("should never happen");
}

NameRef GlobalState::freshNameUnique(UniqueNameKind uniqueNameKind, NameRef original, u2 num) {
    ENFORCE(num > 0, "num == 0, name overflow");
    const auto hs = _hash_mix_unique((u2)uniqueNameKind, UNIQUE, num, original.id());
    unsigned int hashTableSize = names_by_hash.size();
    unsigned int mask = hashTableSize - 1;
    auto bucketId = hs & mask;
    unsigned int probe_count = 1;

    while (names_by_hash[bucketId].second != 0 && probe_count < hashTableSize) {
        auto &bucket = names_by_hash[bucketId];
        if (bucket.first == hs) {
            auto &nm2 = names[bucket.second];
            if (nm2.kind == UNIQUE && nm2.unique.uniqueNameKind == uniqueNameKind && nm2.unique.num == num &&
                nm2.unique.original == original) {
                counterInc("names.unique.hit");
                return nm2.ref(*this);
            } else {
                counterInc("names.hash_collision.unique");
            }
        }
        bucketId = (bucketId + probe_count) & mask;
        probe_count++;
    }
    if (probe_count == hashTableSize) {
        Error::raise("Full table?");
    }
    ENFORCE(!nameTableFrozen);

    if (names.size() == names.capacity()) {
        expandNames();
        hashTableSize = names_by_hash.size();
        mask = hashTableSize - 1;

        bucketId = hs & mask; // look for place in the new size
        probe_count = 1;
        while (names_by_hash[bucketId].second != 0) {
            bucketId = (bucketId + probe_count) & mask;
            probe_count++;
        }
    }

    auto &bucket = names_by_hash[bucketId];
    bucket.first = hs;
    bucket.second = names.size();

    auto idx = names.size();
    names.emplace_back();

    names[idx].kind = UNIQUE;
    names[idx].unique.num = num;
    names[idx].unique.uniqueNameKind = uniqueNameKind;
    names[idx].unique.original = original;
    ENFORCE(names[idx].hash(*this) == hs);
    wasModified_ = true;
    categoryCounterInc("names", "unique");
    return NameRef(*this, idx);
}

FileRef GlobalState::enterFile(shared_ptr<File> file) {
    ENFORCE(!fileTableFrozen);

    DEBUG_ONLY(for (auto &f
                    : this->files) {
        if (f) {
            if (f->path() == file->path()) {
                Error::raise("should never happen");
            }
        }
    })

    files.emplace_back(file);
    auto ret = FileRef(filesUsed() - 1);
    fileRefByPath[(string)file->path()] = ret;
    return ret;
}

FileRef GlobalState::enterFile(absl::string_view path, absl::string_view source) {
    return GlobalState::enterFile(
        make_shared<File>(string(path.begin(), path.end()), string(source.begin(), source.end()), File::Type::Normal));
}

FileRef GlobalState::enterFileAt(absl::string_view path, absl::string_view source, FileRef id) {
    if (this->files[id.id()] && this->files[id.id()]->source_type != File::Type::TombStone) {
        Error::raise("should never happen");
    }

    auto ret = GlobalState::enterNewFileAt(
        make_shared<File>(string(path.begin(), path.end()), string(source.begin(), source.end()), File::Type::Normal),
        id);
    ENFORCE(ret == id);
    return ret;
}

FileRef GlobalState::enterNewFileAt(shared_ptr<File> file, FileRef id) {
    ENFORCE(!fileTableFrozen);
    ENFORCE(id.id() < this->files.size());
    ENFORCE(this->files[id.id()]->source_type == File::Type::TombStone);
    ENFORCE(this->files[id.id()]->path() == file->path());

    // was a tombstone before.
    this->files[id.id()] = file;
    FileRef result(id);
    return result;
}

FileRef GlobalState::reserveFileRef(std::string path) {
    return GlobalState::enterFile(make_shared<File>(move(path), "", File::Type::TombStone));
}

void GlobalState::mangleRenameSymbol(SymbolRef what, NameRef origName, UniqueNameKind kind) {
    auto &owner = what.data(*this).owner;
    auto &members = owner.data(*this).members;
    for (auto &mem : members) {
        if (mem.first == origName) {
            int collisionCount = 1;
            NameRef name;
            do {
                name = freshNameUnique(kind, origName, collisionCount++);
            } while (owner.data(*this).findMember(*this, name).exists());
            mem.first = name;
            mem.second.data(*this).name = mem.first;
            break;
        }
    }
}

LocalVariable GlobalState::newTemporary(NameRef name, SymbolRef owner) const {
    const Symbol &data = owner.data(*this);
    ENFORCE(data.isMethod(), "entering temporary outside of a method");
    int id = ++(data.uniqueCounter);

    LocalVariable ret(name, id);
    return ret;
}

unsigned int GlobalState::symbolsUsed() const {
    return symbols.size();
}

unsigned int GlobalState::filesUsed() const {
    return files.size();
}

unsigned int GlobalState::namesUsed() const {
    return names.size();
}

string GlobalState::toString(bool showHidden) const {
    return Symbols::root().toString(*this, 0, showHidden);
}

void GlobalState::sanityCheck() const {
    if (!debug_mode) {
        return;
    }
    ENFORCE(!names.empty(), "empty name table size");
    ENFORCE(!strings.empty(), "empty string table size");
    ENFORCE(!names_by_hash.empty(), "empty name hash table size");
    ENFORCE((names_by_hash.size() & (names_by_hash.size() - 1)) == 0, "name hash table size is not a power of two");
    ENFORCE(names.capacity() * 2 == names_by_hash.capacity(), "name table and hash name table sizes out of sync",
            " names.capacity=", names.capacity(), " names_by_hash.capacity=", names_by_hash.capacity());
    ENFORCE(names_by_hash.size() == names_by_hash.capacity(), "hash name table not at full capacity");
    int i = -1;
    for (auto &nm : names) {
        i++;
        if (i != 0) {
            nm.sanityCheck(*this);
        }
    }

    i = -1;
    for (auto &sym : symbols) {
        i++;
        if (i != 0) {
            sym.sanityCheck(*this);
        }
    }
    for (auto &ent : names_by_hash) {
        if (ent.second == 0) {
            continue;
        }
        const Name &nm = names[ent.second];
        ENFORCE(ent.first == nm.hash(*this), "name hash table corruption");
    }
}

bool GlobalState::freezeNameTable() {
    bool old = this->nameTableFrozen;
    this->nameTableFrozen = true;
    return old;
}

bool GlobalState::freezeFileTable() {
    bool old = this->fileTableFrozen;
    this->fileTableFrozen = true;
    return old;
}

bool GlobalState::freezeSymbolTable() {
    bool old = this->symbolTableFrozen;
    this->symbolTableFrozen = true;
    return old;
}

bool GlobalState::unfreezeNameTable() {
    bool old = this->nameTableFrozen;
    this->nameTableFrozen = false;
    return old;
}

bool GlobalState::unfreezeFileTable() {
    bool old = this->fileTableFrozen;
    this->fileTableFrozen = false;
    return old;
}

bool GlobalState::unfreezeSymbolTable() {
    bool old = this->symbolTableFrozen;
    this->symbolTableFrozen = false;
    return old;
}

unique_ptr<GlobalState> GlobalState::deepCopy(bool keepId) const {
    this->sanityCheck();
    auto result = make_unique<GlobalState>(this->errorQueue);
    result->silenceErrors = this->silenceErrors;

    if (keepId) {
        result->globalStateId = this->globalStateId;
    }
    result->parentGlobalStateId = this->globalStateId;
    result->lastNameKnownByParentGlobalState = namesUsed();

    result->strings = this->strings;
    result->strings_last_page_used = STRINGS_PAGE_SIZE;
    result->files = this->files;
    result->fileRefByPath = this->fileRefByPath;
    result->lspInfoQueryLoc = this->lspInfoQueryLoc;

    result->names.reserve(this->names.capacity());
    for (auto &nm : this->names) {
        result->names.emplace_back(nm.deepCopy(*result));
    }

    result->names_by_hash.reserve(this->names_by_hash.size());
    result->names_by_hash = this->names_by_hash;

    result->symbols.reserve(this->symbols.size());
    for (auto &sym : this->symbols) {
        result->symbols.emplace_back(sym.deepCopy(*result));
    }
    result->sanityCheck();
    return result;
}

void GlobalState::addAnnotation(Loc loc, string str, AnnotationPos pos) const {
    unique_lock<mutex> lk(annotations_mtx);
    annotations.emplace_back(loc, str, pos);
}

string GlobalState::showAnnotatedSource(FileRef file) const {
    if (annotations.empty()) {
        return "";
    }

    // Sort the locs backwards
    auto compare = [](Annotation left, Annotation right) {
        if (left.loc.file != right.loc.file) {
            return left.loc.file.id() > right.loc.file.id();
        }

        auto a = left.pos == GlobalState::AnnotationPos::BEFORE ? left.loc.begin_pos : left.loc.end_pos;
        auto b = right.pos == GlobalState::AnnotationPos::BEFORE ? right.loc.begin_pos : right.loc.end_pos;

        if (a != b) {
            return a > b;
        }
        if (left.pos != right.pos) {
            if (left.pos == GlobalState::AnnotationPos::BEFORE && right.pos == GlobalState::AnnotationPos::AFTER) {
                return false;
            }
            if (left.pos == GlobalState::AnnotationPos::AFTER && right.pos == GlobalState::AnnotationPos::BEFORE) {
                return true;
            }
        }
        return false;
    };
    auto sorted = annotations;
    sort(sorted.begin(), sorted.end(), compare);

    auto source = file.data(*this).source();
    string outline(source.begin(), source.end());
    for (auto annotation : sorted) {
        if (annotation.loc.file != file) {
            continue;
        }
        stringstream buf;

        auto pos = annotation.loc.position(*this);
        vector<string> lines = absl::StrSplit(annotation.str, "\n");
        while (!lines.empty() && lines.back().empty()) {
            lines.pop_back();
        }
        if (!lines.empty()) {
            buf << '\n';
            for (auto line : lines) {
                for (int p = 1; p < pos.first.column; p++) {
                    buf << " ";
                }
                if (line.empty()) {
                    // Avoid the trailing space
                    buf << "#";
                } else {
                    buf << "# " << line;
                }
                buf << '\n';
            }
        }
        auto out = buf.str();
        out = out.substr(0, out.size() - 1); // Remove the last newline that the buf always has

        size_t start_of_line;
        switch (annotation.pos) {
            case GlobalState::AnnotationPos::BEFORE:
                start_of_line = annotation.loc.begin_pos;
                start_of_line = outline.find_last_of('\n', start_of_line);
                if (start_of_line == string::npos) {
                    start_of_line = 0;
                }
                break;
            case GlobalState::AnnotationPos::AFTER:
                start_of_line = annotation.loc.end_pos;
                start_of_line = outline.find_first_of('\n', start_of_line);
                if (start_of_line == string::npos) {
                    start_of_line = outline.end() - outline.begin();
                }
                break;
        }
        outline = outline.substr(0, start_of_line) + out + outline.substr(start_of_line);
    }
    return outline;
}

int GlobalState::totalErrors() const {
    return errorQueue->errorCount.load();
}

void GlobalState::_error(unique_ptr<BasicError> error) const {
    ENFORCE(shouldReportErrorOn(error->loc, error->what));
    if (error->isCritical) {
        errorQueue->hadCritical = true;
    }
    errorQueue->pushError(*this, move(error));
}

bool GlobalState::hadCriticalError() const {
    return errorQueue->hadCritical;
}

void GlobalState::flushErrors() {
    this->errorQueue->flushErrors();
}

void GlobalState::flushErrorCount() {
    this->errorQueue->flushErrorCount();
}

ErrorBuilder GlobalState::beginError(Loc loc, ErrorClass what) const {
    if (loc.file.exists()) {
        loc.file.data(*this).hadErrors_ = true;
    }
    bool report = shouldReportErrorOn(loc, what);
    if (report) {
        histogramAdd("error", what.code, 1);
    }
    report = (what == errors::Internal::InternalError) || (report && !this->silenceErrors);
    return ErrorBuilder(*this, report, loc, what);
}

bool GlobalState::shouldReportErrorOn(Loc loc, ErrorClass what) const {
    StrictLevel level = StrictLevel::Strong;
    if (loc.file.exists()) {
        level = loc.file.data(*this).strict;
    }
    if (what.code == errors::Internal::InternalError.code) {
        return true;
    }

    return level >= what.minLevel;
}

bool GlobalState::wasModified() const {
    return wasModified_;
}

void GlobalState::trace(const string &msg) const {
    errorQueue->tracer.trace(msg);
}

void GlobalState::markAsPayload() {
    bool seenEmpty = false;
    for (auto &f : files) {
        if (!seenEmpty) {
            ENFORCE(!f);
            seenEmpty = true;
            continue;
        }
        f->source_type = File::Type::Payload;
    }
}

std::unique_ptr<GlobalState> GlobalState::replaceFile(std::unique_ptr<GlobalState> inWhat, FileRef whatFile,
                                                      std::shared_ptr<File> withWhat) {
    ENFORCE(whatFile.id() < inWhat->filesUsed());
    ENFORCE(whatFile.data(*inWhat, true).path() == withWhat->path());
    inWhat->files[whatFile.id()] = move(withWhat);
    return inWhat;
}

FileRef GlobalState::findFileByPath(absl::string_view path) {
    auto fnd = fileRefByPath.find((string)path);
    if (fnd != fileRefByPath.end()) {
        return fnd->second;
    }
    return FileRef();
}

std::unique_ptr<GlobalState> GlobalState::markFileAsTombStone(std::unique_ptr<GlobalState> what, FileRef fref) {
    ENFORCE(fref.id() < what->filesUsed());
    what->files[fref.id()]->source_type = File::Type::TombStone;
    return what;
}

unsigned int GlobalState::hash() const {
    constexpr bool DEBUG_HASHING_TAIL = false;
    unsigned int result = 0;
    int counter = 0;
    for (const auto &name : this->names) {
        counter++;
        result = mix(result, name.hash(*this));
        if (DEBUG_HASHING_TAIL && counter > namesUsed() - 15) {
            errorQueue->logger.info("Hashing names: {}, {}", result, name.show(*this));
        }
    }
    result = result + 1;
    for (const auto &sym : this->symbols) {
        result = mix(result, sym.hash(*this));
    }
    return result;
}

std::vector<std::shared_ptr<File>> GlobalState::getFiles() const {
    return files;
}

} // namespace core
} // namespace sorbet
