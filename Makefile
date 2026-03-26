CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Icommon -Iclient/include -Iserver/include -Isecurity
LDLIBS ?= -lssl -lcrypto -lpthread

SERVER_SOURCES = \
	server/server.c \
	server/connection_handler.c \
	server/thread_pool.c \
	common/protocol.c \
	common/utils.c \
	security/ssl_setup.c

CLIENT_SOURCES = \
	client/client.c \
	client/client_utils.c \
	common/protocol.c \
	common/utils.c \
	security/ssl_setup.c

.PHONY: all clean

all: server/server_app client/client_app

server/server_app: $(SERVER_SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SERVER_SOURCES) $(LDLIBS)

client/client_app: $(CLIENT_SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(CLIENT_SOURCES) $(LDLIBS)

clean:
	rm -f server/server_app client/client_app
