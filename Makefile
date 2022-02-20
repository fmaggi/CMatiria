CC = clang
LL = llvm-ar

CFLAGS = -Isrc -Werror -D_FORTIFY_SOURCE=2 -std=c17
LFLAGS =

MATIRIA = matiria
SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/**/**/*.c) $(wildcard $(SRC_DIR)/**/**/**/*.c)
OBJS  = $(SRC:%.c=%.o)

ifndef config
	config=debug
endif

ifeq ($(config), debug)
	CFLAGS += -DDEBUG -g -pg
	LFLAGS += -pg -g
else
	CFLAGS += -O2 -m64 -Ofast -ffast-math -flto -mllvm -polly -mllvm -polly-parallel -mllvm -polly-omp-backend=LLVM
	LFLAGS += -flto
endif

$(MATIRIA): $(OBJS)
	@echo [EXE] $(MATIRIA)
	@$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	@echo [CC] $<
	@$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@rm $(OBJS)
