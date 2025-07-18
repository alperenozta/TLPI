CC = gcc
CFLAGS = -Wall -g

OBJS = become_daemon.o
HDRS = become_daemon.h

TARGETS = test_become_daemon daemon_logger

all: $(TARGETS)

# İlk program: test_become_daemon
test_become_daemon: test_become_daemon.c $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ test_become_daemon.c $(OBJS)

# İkinci program: daemon_logger
daemon_logger: daemon_logger.c $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ daemon_logger.c $(OBJS)

# Genel nesne dosyası kuralı
%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(TARGETS)

