CC=${CROSS_COMPILE_S}gcc
LIBS = -lpthread -luLinux_cgi
CFLAGS = -g -Wall
DFLAGS = -D_GNU_SOURCE
IDFLAGS = -I${NAS_LIB_PATH}/include
LDFLAGS = -L$(SYS_TARGET_PREFIX)/lib

SRCS = main.c ls.c usage.c lsof.c cgi.c
OBJS = $(addprefix build/,$(SRCS:.c=.o))
TARGET = build/main.cgi

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS) $(IDFLAGS) $(LDFLAGS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ 

clean:
	$(RM) -r build

