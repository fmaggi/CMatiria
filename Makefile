CC = clang
LL = llvm-ar

DEPS = dependencies

CFLAGS = -Imatiria -Werror -D_FORTIFY_SOURCE=2 -std=c18 -O2
LFLAGS = -no-pie

MATIRIA = matiria
SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/**/**/*.c) $(wildcard $(SRC_DIR)/**/**/**/*.c)
OBJS  = $(SRC:%.c=%.o)

ifndef config
	config=release
endif

ifeq ($(config), debug)
	CFLAGS += -DDEBUG -g -pg
	LFLAGS += -pg -g

	OBJ = obj/debug
else
	OBJ = obj/release
endif

$(MATIRIA): $(OBJS)
	@echo [EXE] $(MATIRIA)
	@$(CC) -o $@ $^
	@rm $^

%.o: %.c
	@echo [CC] $<
	@$(CC) -o $@ -c $< $(CFLAGS)
