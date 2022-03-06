#include "type.h"

#include <stdlib.h>

struct mtr_object* mtr_new_array_obj(struct mtr_type type) {
    struct mtr_array* a = malloc(sizeof(*a));
    a->type = type;
    return (struct mtr_object*) a;
}
