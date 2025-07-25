# Derleyici ve arşivleyici tanımları
CC       := gcc
AR       := ar rcs
CFLAGS   := -Wall -g

# Oluşturulacak statik kütüphane adı ve objeleri
LIB_NAME    := daemon_logger
LIB          := lib$(LIB_NAME).a
LIB_OBJS    := become_daemon.o daemon_logger.o

# Nihai hedefler
TARGETS     := server client

all: $(LIB) $(TARGETS)

# ─── Statik kitaplık ───────────────────────────────────────────────
$(LIB): $(LIB_OBJS)
	$(AR) $@ $^



# ─── server (server.c içindeki main) ───────────────────────────────
server: server.o $(LIB)
	$(CC) $(CFLAGS) -o $@ server.o -L. -l$(LIB_NAME)

# ─── client (bağımsız) ─────────────────────────────────────────────
client: client.o
	$(CC) $(CFLAGS) -o $@ client.o

# ─── Genel .o kuralı ────────────────────────────────────────────────
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# ─── Dosya bağımlılıkları ──────────────────────────────────────────
become_daemon.o: become_daemon.c become_daemon.h
daemon_logger.o: daemon_logger.c become_daemon.h    # log fonksiyon başlığ
server.o: server.c declarations.h headers.h become_daemon.h 
client.o: client.c declarations.h headers.h become_daemon.h 

# ─── Temizlik ─────────────────────────────────────────────────────
clean:
	rm -f *.o $(LIB) $(TARGETS)

