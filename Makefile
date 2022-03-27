CC = clang
LL = llvm-ar

CFLAGS = -Isrc -Werror -Wall -Wextra -pedantic -Wno-unused-parameter -D_FORTIFY_SOURCE=2 -std=c17
EXEFLAGS =

MATIRIA = matiria
SRC_DIR = src

SRC = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/**/**/*.c) $(wildcard $(SRC_DIR)/**/**/**/*.c)
OBJS = $(SRC:%.c=%.o)
JSON = $(SRC:%.c=%.j)

ifndef config
	config=debug
endif

ifeq ($(config), debug)
	CFLAGS += -g -pg
	EXEFLAGS += -g -pg
else
	CFLAGS += -DNDEBUG -m64 -Ofast -ffast-math -flto -O3 -mllvm -polly -mllvm -polly-parallel
	EXEFLAGS += -flto -lgomp -m64 -Ofast -ffast-math -flto -O3
endif

$(MATIRIA): $(OBJS)
	@echo [EXE] $(MATIRIA)
	@$(CC) -o $@ $^ $(EXEFLAGS)

%.o: %.c
	@echo [CC] $<
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	@rm $(OBJS) $(MATIRIA)

vscode_setup: $(JSON)
	@sed -e '1s/^/[\n/' -e '$$s/,$$/\n]/' $(JSON:%.j=%.j.json) > build/compile_commands.json
	@rm $(JSON:%.j=%.j.json)


%.j: %.c
	@$(CC) -MJ $@.json $(CFLAGS) -o $@ -c $<
	@rm $@
