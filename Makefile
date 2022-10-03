SRC := binder.c buf.c transaction.c

CFLAGS += -Wall -Iinclude

TARGET_ARCH ?= x86_64

ifndef NDK_ROOT
  $(error NDK_ROOT is not set)
endif

# Cross-compile for aarch64 if the $ARCH is arm64 or aarch64
ifneq ($(filter arm64 aarch64,$(ARCH)),)
  TARGET_ARCH := aarch64
endif

# Compile for Android platform if $ANDROID is 1
ifeq ($(ANDROID),1)
  # Android NDK settings
  SDK := 30
  HOST_OS := $(shell uname | tr '[:upper:]' '[:lower:]')
  HOST_ARCH := x86_64
  HOST_PLATFORM := $(HOST_OS)-$(HOST_ARCH)
  TARGET_PLATFORM := $(TARGET_ARCH)-linux-android
  NDK_TOOLCHAIN := $(NDK_ROOT)/toolchains/llvm/prebuilt/$(HOST_PLATFORM)

  CC := $(NDK_TOOLCHAIN)/bin/$(TARGET_PLATFORM)$(SDK)-clang
  CFLAGS += -DANDROID
else
  CC := clang
  CFLAGS += --target=aarch64-linux-gnu
endif

SRC := $(addprefix src/, $(SRC))
OBJ := $(SRC:.c=.o)

all: libdevbinder.a examples

$(OBJ): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

libdevbinder.so: $(OBJ)
	$(CC) $(CFLAGS) -fPIC -shared -o libdevbinder.so $(OBJ)

libdevbinder.a: $(OBJ)
	ar rcs $@ $(OBJ)

examples: server client

server: CFLAGS += -static
server: examples/server.c libdevbinder.a
	$(CC) $(CFLAGS) -o $@ $^

client: CFLAGS += -static
client: examples/client.c libdevbinder.a
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f src/*.o libdevbinder.so libdevbinder.a
	rm -f server client
