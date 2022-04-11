#include "type.h"

#include "core/log.h"
#include "debug/dump.h"
#include "scanner/token.h"

#include <stdlib.h>
#include <string.h>

const struct mtr_type invalid_type = {
    .type = MTR_DATA_INVALID,
    .obj = NULL
};

struct mtr_type mtr_get_data_type(struct mtr_token type) {
    struct mtr_type t = invalid_type;
    t.assignable = true;
    switch (type.type)
    {
    case MTR_TOKEN_INT_LITERAL:
    case MTR_TOKEN_INT: {
        t.type = MTR_DATA_INT;
        break;
    }

    case MTR_TOKEN_FLOAT_LITERAL:
    case MTR_TOKEN_FLOAT: {
        t.type = MTR_DATA_FLOAT;
        break;
    }

    case MTR_TOKEN_BOOL:
    case MTR_TOKEN_TRUE:
    case MTR_TOKEN_FALSE: {
        t.type = MTR_DATA_BOOL;
        break;
    }

    case MTR_TOKEN_STRING_LITERAL:
    case MTR_TOKEN_STRING: {
        t.type = MTR_DATA_STRING;
        break;
    }

    case MTR_TOKEN_ANY: {
        t.type = MTR_DATA_ANY;
        break;
    }

    default:
        MTR_LOG_DEBUG("Invalid data type  %s", mtr_token_type_to_str(type.type));
        break;
    }
    return t;
}

struct mtr_type mtr_copy_type(struct mtr_type type) {
    switch (type.type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type.obj;
        return mtr_new_array_type(a->type);
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type.obj;
        return mtr_new_map_type(m->key, m->value);
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type.obj;
        return mtr_new_function_type(f->return_, f->argc, f->argv);
    }
    case MTR_DATA_USER: {
        struct mtr_user_type* s = (struct mtr_user_type*) type.obj;
        return mtr_new_user_type(s->name);
    }
    case MTR_DATA_STRUCT: {
        struct mtr_struct_type* s = (struct mtr_struct_type*) type.obj;
        return mtr_new_struct_type(s->name.name, s->members, s->argc);
    }
    default:
        return type; // no need to copy anything;
    }

}

static void delete_object_type(mtr_object_type* obj, enum mtr_data_type type) {
    switch (type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) obj;
        mtr_delete_type(a->type);
        free(a);
        return;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) obj;
        mtr_delete_type(m->key);
        mtr_delete_type(m->value);
        free(m);
        return;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) obj;
        mtr_delete_type(f->return_);
        for (u8 i = 0; i < f->argc; ++i) {
            mtr_delete_type(f->argv[i]);
        }
        free(f->argv);
        free(f);
        return;
    }
    case MTR_DATA_STRUCT: {
        struct mtr_struct_type* s = (struct mtr_struct_type*) obj;
        free(s->members);
        free(s);
        return;
    }
    case MTR_DATA_UNION: {
        struct mtr_union_type* u = (struct mtr_union_type*) obj;
        for (u8 i = 0; i < u->argc; ++i ) {
            mtr_delete_type(u->types[i]);
        }
        free(u->types);
        free(u);
        return;
    }
    case MTR_DATA_USER: {
        struct mtr_user_type* u = (struct mtr_user_type*) obj;
        free(u);
        return;
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid type.");
}

void mtr_delete_type(struct mtr_type type) {
    if (type.obj) {
        delete_object_type(type.obj, type.type);
    }
}

static bool are_user_types(enum mtr_data_type lhs, enum mtr_data_type rhs) {
    return (lhs == MTR_DATA_USER && (rhs == MTR_DATA_STRUCT || rhs == MTR_DATA_UNION))
        || (rhs == MTR_DATA_USER && (lhs == MTR_DATA_STRUCT || lhs == MTR_DATA_UNION));
}

static bool object_type_match(mtr_object_type* lhs, mtr_object_type* rhs, enum mtr_data_type type) {
    switch (type) {
    case MTR_DATA_INVALID: return false;
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* l = (struct mtr_array_type*) lhs;
        struct mtr_array_type* r = (struct mtr_array_type*) rhs;
        return mtr_type_match(l->type, r->type);
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* l = (struct mtr_map_type*) lhs;
        struct mtr_map_type* r = (struct mtr_map_type*) rhs;
        return mtr_type_match(l->key, r->key) && mtr_type_match(l->value, r->value);
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* l = (struct mtr_function_type*) lhs;
        struct mtr_function_type* r = (struct mtr_function_type*) rhs;
        return mtr_type_match(l->return_, r->return_);
    }
    case MTR_DATA_USER:
    case MTR_DATA_UNION:
    case MTR_DATA_STRUCT: {
        struct mtr_user_type* l = (struct mtr_user_type*) lhs;
        struct mtr_user_type* r = (struct mtr_user_type*) rhs;
        return l->name.length == r->name.length && memcmp(l->name.start, r->name.start, l->name.length) == 0;
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid data type");
    return false;
}

bool mtr_type_match(struct mtr_type lhs, struct mtr_type rhs) {
    bool any = lhs.type == MTR_DATA_ANY || rhs.type == MTR_DATA_ANY;
    bool trivial_type = lhs.obj == NULL;
    bool match = ((lhs.type == rhs.type) || are_user_types(lhs.type, rhs.type))
            && (
                trivial_type || object_type_match(lhs.obj, rhs.obj, lhs.type)
            // relying on short circuiting to decide whether to call object_type_match or not
            );
    return any || match;
}


struct mtr_type mtr_get_underlying_type(struct mtr_type type) {
    switch (type.type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type.obj;
        return a->type;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type.obj;
        return m->value;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type.obj;
        return f->return_;
    }
    default:
        return invalid_type;
    }
}

struct mtr_type mtr_new_array_type(struct mtr_type type) {
    struct mtr_array_type* a = malloc(sizeof(*a));
    a->type = type;

    struct mtr_type t;
    t.type = MTR_DATA_ARRAY;
    t.obj = a;
    t.assignable = true;
    return t;
}


struct mtr_type mtr_new_map_type(struct mtr_type key, struct mtr_type value) {
    struct mtr_map_type* m = malloc(sizeof(*m));
    m->key = key;
    m->value = value;

    struct mtr_type t;
    t.type = MTR_DATA_MAP;
    t.obj = m;
    t.assignable = true;
    return t;
}

struct mtr_type mtr_new_function_type(struct mtr_type return_, u8 argc, struct mtr_type* argv) {
    struct mtr_function_type* f = malloc(sizeof(*f));
    f->return_ = return_;
    f->argc = argc;
    f->argv = NULL;
    if (argc > 0) {
        f->argv = malloc(sizeof(struct mtr_type) * argc);
        memcpy(f->argv, argv, sizeof(struct mtr_type) * argc);
    }

    struct mtr_type t;
    t.type = MTR_DATA_FN;
    t.obj = f;
    t.assignable = false;
    return t;
}

struct mtr_type mtr_new_union_type(struct mtr_token token, struct mtr_type* types, u8 argc) {
    struct mtr_type t;
    t.type = MTR_DATA_UNION;
    t.assignable = false;
    t.obj = NULL;
    if (argc > 255 || argc < 0) {
        goto ret;
    }

    struct mtr_union_type* u = malloc(sizeof(*u));
    u->types = malloc(sizeof(struct mtr_type) * argc);
    memcpy(u->types, types, sizeof(struct mtr_type) * argc);
    u->argc = argc;
    u->name.name = token;

    t.obj = u;
ret:
    return t;
}

struct mtr_type mtr_new_struct_type(struct mtr_token token, struct mtr_symbol** symbols, u8 argc) {
    struct mtr_type t;

    struct mtr_struct_type* s = malloc(sizeof(*s));
    s->name.name = token;
    s->members = malloc(sizeof(struct mtr_symbol*) * argc);
    memcpy(s->members, symbols, sizeof(struct mtr_symbol*) * argc);
    s->argc = argc;

    t.type = MTR_DATA_STRUCT;
    t.assignable = false;
    t.obj = s;
    return t;
}

struct mtr_type mtr_new_user_type(struct mtr_token token) {
    struct mtr_type t;

    struct mtr_user_type* u = malloc(sizeof(*u));
    u->name = token;

    t.assignable = true;
    t.obj = u;
    t.type = MTR_DATA_USER;
    return t;
}
