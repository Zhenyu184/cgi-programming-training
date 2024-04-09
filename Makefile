CC := $(TARGET)-gcc
TARGET := build/main.cgi
SRCS := main.c ls.c usage.c lsof.c misc.c
OBJS := $(addprefix build/,$(SRCS:.c=.o))

CFLAGS := \
	-g \
	-Wall \
	-O3

DFLAGS := -D_GNU_SOURCE

LIBS :=  \
	-lpthread \
	-luLinux_cgi

IDFLAGS := -I${NAS_LIB_PATH}/include

LDFLAGS := \
	-L${SYS_TARGET_PREFIX}/lib \
	-Wl,-rpath,${SYS_TARGET_PREFIX}/lib

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CC) $(CFLAGS) $^ -o $@ $(IDFLAGS) $(LDFLAGS) $(LIBS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DFLAGS) -c $< -o $@ $(IDFLAGS) $(LDFLAGS) $(LIBS)

clean:
	$(RM) -r build

