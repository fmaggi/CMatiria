#ifndef MTR_MEMORY_H
#define MTR_MEMORY_H

#include "AST/type.h"
#include "engine.h"
#include "object.h"
#include "core/types.h"

void mtr_link_obj(struct mtr_engine* engine, struct mtr_object* object);

void mtr_collect_garbage(struct mtr_engine* engine);

#endif
