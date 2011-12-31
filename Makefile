TARGET  := main
CC      := gcc
CFLAGS  := --std=gnu99 -D_GNU_SOURCE -Wall -Wextra -Werror -g -O0 -MMD
CFLAGS  += `pkg-config --cflags sdl`
LDFLAGS := `pkg-config --libs sdl` -lSDL_image -lm
SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)
DEPS    := $(wildcard *.d)

all: $(TARGET)

$(TARGET): $(OBJS)

clean:
	rm -rf $(TARGET) $(OBJS) $(DEPS)

.PHONY: clean

ifneq ($(DEPS),)
include $(DEPS)
endif

