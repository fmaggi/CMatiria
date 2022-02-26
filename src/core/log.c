#include "log.h"

void mtr_print_call_stack_entry(const char** name) {
    MTR_LOG(MTR_BOLD_DARK(MTR_YELLOW) "Callstack: "MTR_RESET "%s" , *name);
}
