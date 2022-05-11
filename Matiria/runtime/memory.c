#include "memory.h"

void mtr_link_obj(struct mtr_engine* engine, struct mtr_object* object) {
    object->next = engine->objects;
    engine->objects = object;
}
