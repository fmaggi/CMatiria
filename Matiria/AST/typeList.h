#ifndef MTR_TYPE_LIST_H
#define MTR_TYPE_LIST_H

#include "type.h"

struct mtr_type_list {
    struct type_entry* types;
    size_t count;
    size_t capacity;
};

void mtr_type_list_init(struct mtr_type_list* list);
void mtr_type_list_delete(struct mtr_type_list* list);

struct mtr_type* mtr_type_list_register_from_token(struct mtr_type_list* list, struct mtr_token token);
struct mtr_type* mtr_type_list_register_array(struct mtr_type_list* list, struct mtr_type* element);
struct mtr_type* mtr_type_list_register_map(struct mtr_type_list* list, struct mtr_type* key, struct mtr_type* value);
struct mtr_type* mtr_type_list_register_function(struct mtr_type_list* list, struct mtr_type* ret, struct mtr_type** argv, u8 argc);
struct mtr_type* mtr_type_list_register_struct_type(struct mtr_type_list* list, struct mtr_token name, struct mtr_symbol** members, u16 count);
struct mtr_type* mtr_type_list_register_union_type(struct mtr_type_list* list, struct mtr_token name, struct mtr_type** members, u16 count);

struct mtr_type* mtr_type_list_get(struct mtr_type_list* list, size_t index);
struct mtr_type* mtr_type_list_get_user_type(struct mtr_type_list* list, struct mtr_token token);
struct mtr_type* mtr_type_list_get_void_type(struct mtr_type_list* list);

struct mtr_type* mtr_type_list_exists(struct mtr_type_list* list, struct mtr_type type);

#endif
