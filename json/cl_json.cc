/*
 * Copyright (C) 2018 Petr Peringer <peringer@fit.vutbr.cz> (JSON dumper)
 * Copyright (C) 2019 Veronika Sokova <isokova@fit.vutbr.cz>
 *
 * This file is part of code listener.
 *
 * code listener is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * code listener is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with predator.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cl/easy.hh>
#include <cl/cl_msg.hh>
#include <cl/clutil.hh>
#include <cl/storage.hh>
#include <iostream>
#include <cassert>

// required by the gcc plug-in API
extern "C" {
    __attribute__ ((__visibility__ ("default"))) int plugin_is_GPL_compatible;
}

using namespace CodeStorage;

//////////////////////////////////////////////////////////////////
// conversion tables: enum -> C string
// TODO: add quotation later (by std::quote)

/// convert type enum value to string
static inline const char * to_string(enum cl_type_e typ) {
    assert((typ < 13));
    static const char * str[] = {
        /* CL_TYPE_VOID    */ "\"TypeVoid\"",
        /* CL_TYPE_UNKNOWN */ "\"TypeUnknown\"",
        /* CL_TYPE_PTR     */ "\"TypePtr\"",
        /* CL_TYPE_STRUCT  */ "\"TypeStruct\"",
        /* CL_TYPE_UNION   */ "\"TypeUnion\"",
        /* CL_TYPE_ARRAY   */ "\"TypeArray\"",
        /* CL_TYPE_FNC     */ "\"TypeFnc\"",
        /* CL_TYPE_INT     */ "\"TypeInt\"",
        /* CL_TYPE_CHAR    */ "\"TypeChar\"",
        /* CL_TYPE_BOOL    */ "\"TypeBool\"",
        /* CL_TYPE_ENUM    */ "\"TypeEnum\"",
        /* CL_TYPE_REAL    */ "\"TypeReal\"",
        /* CL_TYPE_STRING  */ "\"TypeString\"",
    };
    return str[typ];
}

/// special variant for cst
static inline const char * to_string_cst(enum cl_type_e typ) {
    assert((typ < 13));
    static const char * str[] = {
        /* CL_TYPE_VOID    */ "\"CstVoid\"",
        /* CL_TYPE_UNKNOWN */ "\"CstUnknown\"",
        /* CL_TYPE_PTR     */ "\"CstPtr\"",     // NULL only
        /* CL_TYPE_STRUCT  */ "\"CstStruct\"",
        /* CL_TYPE_UNION   */ "\"CstUnion\"",
        /* CL_TYPE_ARRAY   */ "\"CstArray\"",
        /* CL_TYPE_FNC     */ "\"CstFnc\"",
        /* CL_TYPE_INT     */ "\"CstInt\"",
        /* CL_TYPE_CHAR    */ "\"CstChar\"",
        /* CL_TYPE_BOOL    */ "\"CstBool\"",
        /* CL_TYPE_ENUM    */ "\"CstEnum\"",
        /* CL_TYPE_REAL    */ "\"CstReal\"",
        /* CL_TYPE_STRING  */ "\"CstString\"",
    };
    return str[typ];
}

/// convert scope enum value to string
static inline const char * to_string(enum cl_scope_e scope) {
    assert((scope < 3));
    static const char * str[] = {
        /* CL_SCOPE_GLOBAL   */ "\"CL_SCOPE_GLOBAL\"",
        /* CL_SCOPE_STATIC   */ "\"CL_SCOPE_STATIC\"",
        /* CL_SCOPE_FUNCTION */ "\"CL_SCOPE_FUNCTION\"",
    };
    return str[scope];
}

/// convert operand enum value to string
static inline const char * to_string(enum cl_operand_e op) {
    assert((op < 3));
    static const char * str[] = {
        /* CL_OPERAND_VOID */ "\"OpVoid\"",
        /* CL_OPERAND_CST  */ "\"OpCst\"",
        /* CL_OPERAND_VAR  */ "\"OpVar\"",
    };
    return str[op];
}

/// convert EVar enum value to string
static inline const char * to_string(enum EVar v) {
    assert((v < 4));
    static const char * str[] = {
        /* VAR_VOID    */ "\"VAR_VOID\"",
        /* VAR_GL      */ "\"VAR_GL\"",
        /* VAR_LC      */ "\"VAR_LC\"",
        /* VAR_FNC_ARG */ "\"VAR_FNC_ARG\"",
    };
    return str[v];
}

/// convert instruction type enum value to string (CL_INSN_ prefix removed)
static inline const char * to_string(enum cl_insn_e instruction) {
    assert((instruction < 11));
    static const char * str[] = {
        /* CL_INSN_NOP     */ "\"InsnNOP\"",
        /* CL_INSN_JMP     */ "\"InsnJMP\"",
        /* CL_INSN_COND    */ "\"InsnCOND\"",
        /* CL_INSN_RET     */ "\"InsnRET\"",
        /* CL_INSN_CLOBBER */ "\"InsnCLOBBER\"",
        /* CL_INSN_ABORT   */ "\"InsnABORT\"",
        /* CL_INSN_UNOP    */ "\"InsnUNOP\"",
        /* CL_INSN_BINOP   */ "\"InsnBINOP\"",
        /* CL_INSN_CALL    */ "\"InsnCALL\"",
        /* CL_INSN_SWITCH  */ "\"InsnSWITCH\"",
        /* CL_INSN_LABEL   */ "\"InsnLABEL\"",
    };
    return str[instruction];
}

/// convert unary instruction subtype enum value to string (enum cl_unop_e)
static inline const char * to_string_unop(int subcode) {
    assert((subcode < 6));
    static const char * str[] = {
        /* CL_UNOP_ASSIGN    */ "\"CL_UNOP_ASSIGN\"",
        /* CL_UNOP_TRUTH_NOT */ "\"CL_UNOP_TRUTH_NOT\"",
        /* CL_UNOP_BIT_NOT   */ "\"CL_UNOP_BIT_NOT\"",
        /* CL_UNOP_MINUS     */ "\"CL_UNOP_MINUS\"",
        /* CL_UNOP_ABS       */ "\"CL_UNOP_ABS\"",
        /* CL_UNOP_FLOAT     */ "\"CL_UNOP_FLOAT\"",
    };
    return str[subcode];
}

/// convert binary instruction subtype (enum cl_binop_e) value to string
static inline const char * to_string_binop(int subcode) {
    assert((subcode < 27));
    static const char * str[] = {
     /* CL_BINOP_EQ, */ "\"CL_BINOP_EQ\"",
     /* CL_BINOP_NE, */ "\"CL_BINOP_NE\"",
     /* CL_BINOP_LT, */ "\"CL_BINOP_LT\"",
     /* CL_BINOP_GT, */ "\"CL_BINOP_GT\"",
     /* CL_BINOP_LE, */ "\"CL_BINOP_LE\"",
     /* CL_BINOP_GE, */ "\"CL_BINOP_GE\"",

     /* CL_BINOP_TRUTH_AND, */ "\"CL_BINOP_TRUTH_AND\"",
     /* CL_BINOP_TRUTH_OR, */ "\"CL_BINOP_TRUTH_OR\"",
     /* CL_BINOP_TRUTH_XOR, */ "\"CL_BINOP_TRUTH_XOR\"",

     /* CL_BINOP_PLUS, */ "\"CL_BINOP_PLUS\"",
     /* CL_BINOP_MINUS, */ "\"CL_BINOP_MINUS\"",
     /* CL_BINOP_MULT, */ "\"CL_BINOP_MULT\"",
     /* CL_BINOP_EXACT_DIV, */ "\"CL_BINOP_EXACT_DIV\"",
     /* CL_BINOP_TRUNC_DIV, */ "\"CL_BINOP_TRUNC_DIV\"",
     /* CL_BINOP_TRUNC_MOD, */ "\"CL_BINOP_TRUNC_MOD\"",
     /* CL_BINOP_RDIV, */ "\"CL_BINOP_RDIV\"",
     /* CL_BINOP_MIN, */ "\"CL_BINOP_MIN\"",
     /* CL_BINOP_MAX, */ "\"CL_BINOP_MAX\"",

     /* CL_BINOP_POINTER_PLUS, */ "\"CL_BINOP_POINTER_PLUS\"",

     /* CL_BINOP_BIT_AND, */ "\"CL_BINOP_BIT_AND\"",
     /* CL_BINOP_BIT_IOR, */ "\"CL_BINOP_BIT_IOR\"",
     /* CL_BINOP_BIT_XOR, */ "\"CL_BINOP_BIT_XOR\"",

     /* CL_BINOP_LSHIFT, */ "\"CL_BINOP_LSHIFT\"",
     /* CL_BINOP_RSHIFT, */ "\"CL_BINOP_RSHIFT\"",
     /* CL_BINOP_LROTATE, */ "\"CL_BINOP_LROTATE\"",
     /* CL_BINOP_RROTATE, */ "\"CL_BINOP_RROTATE\"",
     /* CL_BINOP_UNKNOWN */ "\"CL_BINOP_UNKNOWN\"",
    };
    return str[subcode];
}

/// convert enum cl_accessor_e value to string
static inline const char * to_string(enum cl_accessor_e a) {
    assert((a < 5));
    static const char * str[] = {
        /* CL_ACCESSOR_REF         */ "\"Ref\"",
        /* CL_ACCESSOR_DEREF       */ "\"Deref\"",
        /* CL_ACCESSOR_DEREF_ARRAY */ "\"DerefArray\"",
        /* CL_ACCESSOR_ITEM        */ "\"Item\"",
        /* CL_ACCESSOR_OFFSET      */ "\"Offset\"",
    };
    return str[a];
}

static bool is_valid(struct cl_loc l) {
    if(l.file)
        return true;
    return false;
}

/// convert location info to JSON
static std::string to_json(struct cl_loc l) {
    std::stringstream out;
    out << std::boolalpha;
    out << "( \"" << (l.file?l.file:"") << "\", "
            << l.line             << ", "
            << l.column;
    if(l.sysp)
        out << ", true ";
    out << ")";
    return out.str();
}

/// get type name
inline const char * nameOf(const struct cl_type &pTyp) {
    if(pTyp.name)
        return pTyp.name;
    else
        return "<anonymous>";
}

/// get name of the variable
inline std::string nameOf(const CodeStorage::Var &v) {
    return v.name;
}

// some indentation macros
#if 0
static int indent_json=0;
#define INDENT_UP   do{ ::indent_json+=4; }while(0)
#define INDENT_DOWN do{ if(::indent_json>=4) ::indent_json-=4; }while(0)
#define INDENT      std::string((::indent_json<30)?::indent_json:30,' ')
#else
#define INDENT_UP       do{}while(0)
#define INDENT_DOWN     do{}while(0)
#define INDENT          ""
#endif

/// dump CodeStorage type
static std::string to_json(const struct cl_type *pTyp) {
    std::stringstream out;
    out << std::boolalpha;
    out << "( " << pTyp->uid << ",\n";
    INDENT_UP;
    out << INDENT << "{\n";
    out << INDENT << "\"code\": ";
    // variants:
    switch(pTyp->code) {
    case CL_TYPE_PTR:
            // assert(item_cnt==1);
            out << "< " << to_string(pTyp->code) << ": ";
            out << pTyp->items[0].type->uid;
            out << " >,\n";
            break;
    case CL_TYPE_ARRAY:
            // assert(item_cnt==1);
            out << "< " << to_string(pTyp->code) << ": (";
            out << pTyp->array_size;
            out << ", ";
            out << pTyp->items[0].type->uid;
            out << " ) >,\n";
            break;
    case CL_TYPE_STRUCT:
    case CL_TYPE_UNION:
            //TODO: can be empty
            out << "< " << to_string(pTyp->code) << ": [ ";
            for(int i=0; i < pTyp->item_cnt; ++i) {
                if(i>0) out << ", ";    // JSON does not allow trailing comma
                out << "{ ";
                if(pTyp->items[i].name)
                    out << "\"name\": \"" << pTyp->items[i].name << "\", ";
                out << "\"offset\": " << pTyp->items[i].offset << ", ";
                out << "\"typ\": " << pTyp->items[i].type->uid;
                out << " }";
            }
            out << " ] >,\n";
            break;
    case CL_TYPE_FNC:
            // TODO: name, offset?
            out << "< " << to_string(pTyp->code) << ": [ ";
            for(int i=0; i < pTyp->item_cnt; ++i) {
                if(i>0) out << ", ";    // JSON does not allow trailing comma
                out << "{ ";
                out << "\"typ\": " << pTyp->items[i].type->uid;
                out << " }";
            }
            out << " ] >,\n";
            break;
    default:
            out << to_string(pTyp->code) << ",\n";
            break;
    } // switch

    if(pTyp->name)
        out << INDENT << "\"name\": \"" << nameOf(*pTyp) << "\",\n";

    out << INDENT << "\"size\": " << pTyp->size << ",\n";

    //if(pTyp->code == CL_TYPE_INT)
    if(pTyp->is_unsigned)
        out << INDENT << "\"is_unsigned\": " << pTyp->is_unsigned << ",\n";

    //TODO: mandatory? if(pTyp->is_const)
        out << INDENT << "\"is_const\": " << pTyp->is_const << ",\n";

    out << INDENT << "\"scope\": " << to_string(pTyp->scope) << "\n";
    out << INDENT << "})";
    INDENT_DOWN;
    return out.str();
}

static std::string to_json(const Insn &i); // forward

/// dump CodeStorage variable
static std::string to_json(const Var &v) {
    std::stringstream out;
    out << std::boolalpha;
    out << "( " << v.uid << ",\n";
    INDENT_UP;
    out << INDENT << "{\n"
        << INDENT << "\"code\": " << to_string(v.code) << ",\n"
        << INDENT << "\"typ\": " << v.type->uid << ",\n"
    ;
    if(v.name!="")
        out << INDENT << "\"name\": \"" << nameOf(v) << "\",\n";
    if(v.initials.size()>0) {
        // initializer of global var == constructor
        out << INDENT << "\"initials\": [\n";
        int ni=0;
        for(auto instr: v.initials) {
            if(ni++>0) out << ",\n";
            out << to_json(*instr);
        }
        out << "\n";
        out << INDENT << "],\n";
    }

    if(v.isExtern)
        out << "\"is_extern\": " << v.isExtern << ", ";

    if(is_valid(v.loc))
        out << INDENT << "\"loc\": " << to_json(v.loc) << ",\n";

    out << INDENT << "\"initialized\": " << v.initialized << "\n";
    out << INDENT << "})";
    INDENT_DOWN;
    return out.str();
}

#if 0
static std::string to_json(const struct cl_insn &i);    // forward
/// dump cl_var
static std::string to_json(const struct cl_var &v) {
    std::stringstream out;
    out << std::boolalpha;
    out << "( " << v.uid << ",\n";
    INDENT_UP;
    out << INDENT << "{\n";
    if(v.name)
        out << INDENT << "\"name\": \"" << v.name << "\",\n";
    out << INDENT << "\"loc\": " << to_json(v.loc) << ",\n";
    if(v.initial) {
        struct cl_initializer *p = v.initial;
        out << INDENT << "\"initial\": [\n";
        while(p) {
            out << INDENT << to_json(p->insn) << ",\n";
            p=p->next;
        }
        out << INDENT << "],\n";
    }
    out << INDENT << "\"is_extern\": " << v.is_extern << "\n";
    out << INDENT << "})";
    INDENT_DOWN;
    return out.str();
}
#endif

/// quote string literal values (like std::quoted)
static std::string quoted(const char *cstr) {
    // TODO: not UTF-8 compatible, needs testing
    std::string s;
    char c;
    s+="\"";
    while(( c = *cstr++ )) {
        switch(c) {
        case '\a': s+="\\a"; break;
        case '\b': s+="\\b"; break;
        case '\f': s+="\\f"; break;
        case '\n': s+="\\n"; break;
        case '\r': s+="\\r"; break;
        case '\t': s+="\\t"; break;
        case '\v': s+="\\v"; break;
        case '\\': s+="\\\\"; break;
        case '"': s+="\\\""; break;
        default:
            if(std::isprint(c))
                s+=c;
            else {
                s+="\\x";
                s+="0123456789ABCDEF"[((unsigned char)c)/16];
                s+="0123456789ABCDEF"[((unsigned char)c)%16];
            }
            break;
        }
    }
    s+="\"";
    return s;
}

/// dump C literal (constant) value
static std::string to_json_cst(const struct cl_cst &v, const struct cl_type *t) {
    enum cl_type_e code = (t)? t->code : v.code;
    if (v.code == CL_TYPE_STRING || v.code == CL_TYPE_FNC)
        code = v.code; // because it is basically pointer
    std::stringstream out;
    out << std::boolalpha;
    out << "{\n";
    out << INDENT << "\"data\": <" << to_string_cst(code);
    switch(v.code) {
    case CL_TYPE_FNC:
        out << INDENT << ": { \"uid\": " << v.data.cst_fnc.uid << ",\n";
        if(v.data.cst_fnc.name)
            out << INDENT << "\"name\": \"" << v.data.cst_fnc.name << "\",\n";
        if(v.data.cst_fnc.is_extern)
            out << INDENT << "\"is_extern\": " << v.data.cst_fnc.is_extern << ",\n";
        out << INDENT << "\"loc\": " << to_json(v.data.cst_fnc.loc) << " }\n";
        break;
    case CL_TYPE_INT:
        // dumped value depends on signedness os integer type
        if(t) {  // FIXME: Not set by CL in some initializers
            if(t->code == CL_TYPE_BOOL) {
                out << INDENT << ": " << ((v.data.cst_int.value)? "true":"false") << "\n";
            } else if(t->is_unsigned)
                out << INDENT << ": " << (v.data.cst_uint.value) << "\n";
            else
                out << INDENT << ": " << (v.data.cst_int.value) << "\n";
        } else { // DEFAULT: signed
            out << INDENT << ": " << (v.data.cst_int.value) << "\n";
        }
        break;
    case CL_TYPE_STRING:
        out << INDENT << ": " << ::quoted(v.data.cst_string.value) << "\n";
        break;
    case CL_TYPE_REAL:
        out << INDENT << ": " << (v.data.cst_real.value) << "\n";
        break;
    default: /* empty */ break;
    } // switch
    out << INDENT << " > }\n";
    return out.str();
}

static std::string to_json(const struct cl_operand &op);//forward


/// dump GCC accessor
static std::string to_json(const struct cl_accessor &a) {
    std::stringstream out;
    out << std::boolalpha;
    out << INDENT << "{\n";
    out << INDENT << "\"typ\": " << a.type->uid << ",\n";
    out << INDENT << "\"data\": <" << to_string(a.code);
    // accessor variants
    switch(a.code) {
    case CL_ACCESSOR_DEREF_ARRAY:
            out << ": ";
            out << to_json(*a.data.array.index);
            break;
    case CL_ACCESSOR_ITEM:
            out << ": ";
            out << a.data.item.id;
            break;
    case CL_ACCESSOR_OFFSET:
            out << ": ";
            out << a.data.offset.off;
            break;
    default: /* empty */ break;
    }
    out << ">\n";
    out << INDENT << "}";
    return out.str();
}

/// dump instruction operand
static std::string to_json(const struct cl_operand &op) {
    std::stringstream out;
    out << std::boolalpha;
    out << "{\n";
    INDENT_UP;
    out << INDENT << "\"data\": <" << to_string(op.code);
    if(op.code==CL_OPERAND_VAR)
        out << ": " << op.data.var->uid;
    if(op.code==CL_OPERAND_CST)
        out << ": " << to_json_cst(op.data.cst,op.type);
    out << " >,\n";

    if(op.type)
        out << INDENT << "\"typ\": " << op.type->uid << ",\n";
    if(op.accessor) {
        out << INDENT << "\"accessor\": [\n";
        INDENT_UP;
        struct cl_accessor *p=op.accessor;
        int n=0;
        while(p) {
            if(n++>0) out << ",\n";
            out << to_json(*p);     // accessor
            p=p->next;
        }
        out << "\n";
        INDENT_DOWN;
        out << INDENT << "],\n";
    }
    out << INDENT << "\"scope\": " << to_string(op.scope) << "\n";
    INDENT_DOWN;
    out << INDENT << "}";
    return out.str();
}

/// dump basic block ID
static std::string to_json(const Block *bb) {
    std::stringstream out;
    const char *label = bb->name().c_str();      // L###
    if (*label != 'L')
        CL_ERROR("FIXME: Label name does not start with 'L'");
    out << std::strtoul(label+1,NULL,10);   // FIXME fragile
    return out.str();
}

/// dump basic block ID list
static std::string to_json(const TTargetList &lst) {
    std::stringstream out;
    out << std::boolalpha;
    out << "[ ";
    int n=0;
    for(const Block *bb: lst) {
        if(n++>0) out << ", ";
        out << to_json(bb);
    }
    out << " ]";
    return out.str();
}

/// dump CodeStorage instruction
static std::string to_json(const Insn &i) {
    std::stringstream out;
    out << std::boolalpha;
    INDENT_UP;
    out << INDENT << "{\n";
    out << INDENT << "\"code\": ";
    out << "<" << to_string(i.code);
    INDENT_UP;
    switch(i.code) {
    case CL_INSN_JMP:   // 0    no operands
            out << ": ";
            out << to_json(i.targets[0]);        // target BB id
            break;
    case CL_INSN_COND:  // 1    if(op[0])
            if(i.operands.size()==1) {
                out << ": ( ";
                out << to_json(i.operands[0]);
                out << ", ";
                out << to_json(i.targets[0]);
                out << ", ";
                out << to_json(i.targets[1]);
                out << " ) ";
            }
            // else CL_ERROR?
            break;
    case CL_INSN_RET:   // 1    return(op[0])
            if(i.operands.size()==1) {
                out << ": \n";
                out << INDENT << to_json(i.operands[0]) << "\n";
            }
            // else CL_ERROR?
            break;
    case CL_INSN_CLOBBER:
            if(i.operands.size()==1) {
                out << ": \n";
                out << INDENT << to_json(i.operands[0]) << "\n";
            }
            break;
    case CL_INSN_ABORT: // 0    no operands
            break;
    case CL_INSN_UNOP:  // 2    op[0] <- f(op[1])
            out << ": \n";
            out << INDENT << "(<" << to_string_unop(i.subCode) << ">,\n";
            for(unsigned o=0; o<i.operands.size(); ++o) {
                if(o>0) out << ",\n";
                out << INDENT << to_json(i.operands[o]);
            }
            out << "\n";
            out << INDENT << ")\n";
            break;
    case CL_INSN_BINOP: // 3    op[0] <- f(op[1],op[2])
            out << ": \n";
            out << INDENT << "(<" << to_string_binop(i.subCode) << ">,\n";
            for(unsigned o=0; o<i.operands.size(); ++o) {
                if(o>0) out << ",\n";
                out << INDENT << to_json(i.operands[o]);
            }
            out << "\n";
            out << INDENT << ")\n";
            break;
    case CL_INSN_CALL:  // >=2  op[0] <- op[1](op[2],...)
            out << ": \n";
            out << INDENT << "[\n";
            for(unsigned o=0; o<i.operands.size(); ++o) {
                if(o>0) out << ",\n";
                out << INDENT << to_json(i.operands[o]);
            }
            out << "\n";
            out << INDENT << "]\n";
            break;
    case CL_INSN_SWITCH: // >=1, should be eliminated by CL
            CL_ERROR("switch instruction");
            break;
    case CL_INSN_LABEL:  // 1
            out << ": \n";
            if(i.operands.size()==1) {
                out << INDENT << "\"";
                if (i.operands[0].code==CL_OPERAND_VOID) {
                    out << i.operands[0].data.cst.data.cst_string.value;
                } else {
                    assert((i.operands[0].code==CL_OPERAND_CST &&
                            i.operands[0].data.cst.code==CL_TYPE_STRING));
                    out << "<anon_label>";
                }
                out << "\"\n";
            } else
                CL_ERROR("wrong label");
            break;
    default:
            CL_ERROR("unknown instruction");
            break;
    }
    INDENT_DOWN;
    out << INDENT << ">,\n";

    // target basic blocs of terminal jump instructions
    if(i.targets.size()>0)
        out << INDENT << "\"targets\": " << to_json(i.targets) << ",\n";
#if 1
    // jump to which of previous "targets" is closing a loop
    // TODO: mandatory?
    if(i.loopClosingTargets.size()>0) {
        out << INDENT << "\"loop_closing_targets\": [ ";
        int nlt=0;
        for(unsigned idx: i.loopClosingTargets) {
            if(nlt++>0) out << ", ";
            out << idx;
        }
        out << " ],\n";
    }
#endif
    // location of command/instruction in source
    out << INDENT << "\"loc\": " << to_json(i.loc) << "\n";
    out << INDENT << "}";
    INDENT_DOWN;
    return out.str();
}

/// dump basic block to JSON
static std::string to_json(const Block &bb) {
    std::stringstream out;
    out << std::boolalpha;
    INDENT_UP;
    out << INDENT << "{\n";
    const char *label = bb.name().c_str();      // L###
    out << INDENT << "\"uid\": " << std::strtoul(label+1,NULL,10) << ",\n"; // FIXME fragile
    out << INDENT << "\"name\": \"" << bb.name() << "\",\n";
    out << INDENT << "\"insns\": [\n";
    int ni=0;
    for(auto instr: bb) {
        if(ni++>0) out << ",\n";
        out << to_json(*instr);
    }
    out << "\n";
    out << INDENT << "],\n";
    //TODO: can be empty:
    out << INDENT << "\"targets\": " << to_json(bb.targets()) << ",\n";
    out << INDENT << "\"inbounds\": " << to_json(bb.inbound()) << "\n";
    out << INDENT << "}";
    INDENT_DOWN;
    return out.str();
}

/// dump function to JSON
static std::string to_json(const Fnc &f) {
    std::stringstream out;
    out << std::boolalpha;
    out << "( " << uidOf(f) << ",\n";
    INDENT_UP;
    out << INDENT << "{\n";
    out << INDENT << "\"def\": " << to_json(f.def) << ",\n";

    out << INDENT << "\"vars\": [ ";
    int nv=0;
    for(auto var_uid: f.vars) {
        if(nv++>0) out << ", ";
        out << var_uid;
    }
    out << " ],\n";

    out << INDENT << "\"args\": [ ";
    int na=0;
    for(auto a_uid: f.args) {
        if(na++>0) out << ", ";
        out << a_uid;
    }
    out << " ],\n";

    out << INDENT << "\"loc\": " << to_json(*locationOf(f)) << ",\n";
    out << INDENT << "\"cfg\": [\n";
    INDENT_UP;
    int nb=0;
    for(auto bb: f.cfg) {
        if(nb++>0) out << ",\n";
        out << to_json(*bb);
    }
    out << "\n";
    out << INDENT << "]\n";
    INDENT_DOWN;
    out << INDENT << "})";
    INDENT_DOWN;
    return out.str();
}


///////////////////////////////////////////////////////////////////////////////
// see easy.hh for details
void clEasyRun(const CodeStorage::Storage &stor, const char *)
{
    try {

    std::stringstream out;
    out << std::boolalpha;
    out << "{\n";

    // dump array of types
    out << "\"types\": [\n";
    unsigned nt=0;
    for(const struct cl_type *pTyp : stor.types ) {
        if(nt>0) out << ",\n";
        out << to_json(pTyp);
        nt++;
    }
    out << "\n],\n";
    //std::cout << "=Total: "<< nt << " types\n";

    // dump array of variables
    out << "\"vars\": [\n";
    unsigned nv=0;
    for(const Var &v : stor.vars ) {
        if(nv>0) out << ",\n";
        out << to_json(v);
        nv++;
    }
    out << "\n],\n";
    //std::cout << "=Total: "<< nv << " variables\n";

    // dump array of functions
    out << "\"fncs\": [\n";
    unsigned nf=0;
    for(const Fnc *pFnc : stor.fncs ) {
        // for each function
        const Fnc &fnc = *pFnc;
        if (!isDefined(fnc))
            continue;

        if(nf>0) out << ",\n";
        out << to_json(fnc);
        nf++;

    }
    out << "\n]\n";
    //std::cout << "=Total: "<< nf << " functions\n";

    out << "}\n";
    std::cout << out.str();

    } catch(...) {
        CL_ERROR("exception in JSON dump");
    }
}
