CC      := gcc
CFLAGS  := -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS :=

BIN     := minilsof
SRCS    := minilsof.c fdinfo.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BIN) $(OBJS)

run: $(BIN)
	./$(BIN) $$
