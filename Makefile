CC=${CROSS_COMPILE_TARGET}gcc
CFLAGS = -g
LIBS = -lpthread

SRCS = main.c ls.c usage.c lsof.c
OBJS = $(addprefix build/,$(SRCS:.c=.o))
TARGET = build/main.cgi

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r build

