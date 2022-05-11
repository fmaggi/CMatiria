#include "type.h"

#include "core/log.h"
#include "debug/dump.h"
#include "scanner/token.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct mtr_type mtr_get_data_type(struct mtr_token type) {
    struct mtr_type t;
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
        t.type = MTR_DATA_INVALID;
        MTR_LOG_DEBUG("Invalid data type  %s", mtr_token_type_to_str(type.type));
        break;
    }
    return t;
}

bool mtr_is_compound_type(const struct mtr_type* type) {
    return type->type > MTR_DATA_STRING;
}

static void delete_object_type(struct mtr_type* obj) {
    switch (obj->type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) obj;
        return;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) obj;
        return;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) obj;
        free(f->argv);
        return;
    }
    case MTR_DATA_STRUCT: {
        struct mtr_struct_type* s = (struct mtr_struct_type*) obj;
        free(s->members);
        return;
    }
    case MTR_DATA_UNION: {
        struct mtr_union_type* u = (struct mtr_union_type*) obj;
        free(u->types);
        return;
    }
    case MTR_DATA_USER: {
        struct mtr_user_type* u = (struct mtr_user_type*) obj;
        return;
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid type.");
}

void mtr_delete_type(struct mtr_type* type) {
    if (mtr_is_compound_type(type)) {
        delete_object_type(type);
    }
}

static bool are_user_types(enum mtr_data_type lhs, enum mtr_data_type rhs) {
    return (lhs == MTR_DATA_USER && (rhs == MTR_DATA_STRUCT || rhs == MTR_DATA_UNION))
        || (rhs == MTR_DATA_USER && (lhs == MTR_DATA_STRUCT || lhs == MTR_DATA_UNION));
}

static bool object_type_match(const struct mtr_type* lhs, const struct mtr_type* rhs) {
    switch (lhs->type) {
    case MTR_DATA_INVALID: return false;
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* l = (struct mtr_array_type*) lhs;
        struct mtr_array_type* r = (struct mtr_array_type*) rhs;
        return mtr_type_match(l->element, r->element);
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* l = (struct mtr_map_type*) lhs;
        struct mtr_map_type* r = (struct mtr_map_type*) rhs;
        return mtr_type_match(l->key, r->key) && mtr_type_match(l->value, r->value);
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* l = (struct mtr_function_type*) lhs;
        struct mtr_function_type* r = (struct mtr_function_type*) rhs;
        if (l->argc != r->argc) {
            return false;
        }

        for (u8 i = 0; i < l->argc; ++i) {
            if (!mtr_type_match(l->argv[i], r->argv[i])) {
                return false;
            }
        }

        return l->return_ && r->return_ && mtr_type_match(l->return_, r->return_);
    }
    case MTR_DATA_USER:
    case MTR_DATA_UNION:
    case MTR_DATA_STRUCT: {
        struct mtr_user_type* l = (struct mtr_user_type*) lhs;
        struct mtr_user_type* r = (struct mtr_user_type*) rhs;
        return mtr_token_compare(l->name, r->name);
    }
    default:
        break;
    }
    MTR_ASSERT(false, "Invalid data type");
    return false;
}

bool mtr_type_match(const struct mtr_type* lhs, const struct mtr_type* rhs) {
    if (!lhs || !rhs) return false;
    bool invalid = lhs->type == MTR_DATA_INVALID || rhs->type == MTR_DATA_INVALID;
    bool any = lhs->type == MTR_DATA_ANY || rhs->type == MTR_DATA_ANY;
    bool match = (lhs->type == rhs->type)
            && (
                (!mtr_is_compound_type(lhs)) || object_type_match(lhs, rhs)
            // relying on short circuiting to decide whether to call object_type_match or not
            );
    bool user_match = are_user_types(lhs->type, rhs->type) && object_type_match(lhs, rhs);
    return !invalid && (any || match || user_match);
}


struct mtr_type* mtr_get_underlying_type(const struct mtr_type* type) {
    switch (type->type) {
    case MTR_DATA_ARRAY: {
        struct mtr_array_type* a = (struct mtr_array_type*) type;
        return a->element;
    }
    case MTR_DATA_MAP: {
        struct mtr_map_type* m = (struct mtr_map_type*) type;
        return m->value;
    }
    case MTR_DATA_FN: {
        struct mtr_function_type* f = (struct mtr_function_type*) type;
        return f->return_;
    }
    default:
        return NULL;
    }
}
