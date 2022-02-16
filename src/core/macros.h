#ifndef _MTR_MACROS_H
#define _MTR_MACROS_H

// I dont know how safe is this but it is certainly cool
#define mtr_container_of(ptr, type, member) (type*) ((char*)ptr - (char*)&((type*)0)->member)

#endif
