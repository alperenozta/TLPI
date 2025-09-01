# --- Minimal Makefile ---

CC       := gcc
AR       := ar rcs
CFLAGS   := -Wall -g
CPPFLAGS := -Iinclude

DAEMON_DIR := daemon
CLI_DIR    := cli
KERNEL_DIR := kernel
LIB_DIR    := lib

LIB        := $(LIB_DIR)/libdaemon_logger.a
LIB_OBJS   := $(LIB_DIR)/become_daemon.o $(LIB_DIR)/daemon_logger.o

SERVER_SRC := $(DAEMON_DIR)/server.c
SERVER_OBJ := $(DAEMON_DIR)/server.o
SERVER     := server

CLIENT_SRC := $(CLI_DIR)/client.c
CLIENT_OBJ := $(CLI_DIR)/client.o
CLIENT     := client

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean user kernel kmod
all: $(LIB) $(SERVER) $(CLIENT) kmod

$(LIB): $(LIB_OBJS)
	@mkdir -p $(LIB_DIR)
	$(AR) $@ $^

$(SERVER): $(SERVER_OBJ) $(LIB)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJ) -L$(LIB_DIR) -ldaemon_logger

$(CLIENT): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJ)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

kmod:
	$(MAKE) -C $(KDIR) M=$(PWD)/$(KERNEL_DIR) modules

clean:
	rm -f $(SERVER) $(CLIENT) $(LIB) $(SERVER_OBJ) $(CLIENT_OBJ) $(LIB_OBJS)
	$(MAKE) -C $(KDIR) M=$(PWD)/$(KERNEL_DIR) clean

user: $(LIB) $(SERVER) $(CLIENT)
kernel: kmod
