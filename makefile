# Projeto Etapa 4: Sistemas Distribuídos 2025/2026
#
# Grupo SD-22
#
# Autores:
# - Guilherme Diniz, Nº 61824
# - Henrique Pinto, Nº 61805
# - Maria Leonor, Nº 61779
#
CC = gcc
CFLAGS = -Iinclude -g -Wall -DTHREADED
AR = ar
ARFLAGS = rcs
LIB_DIR = lib
OBJ_DIR = object
BIN_DIR = binary
LIB = $(LIB_DIR)/liblist.a
LDLIBS = -lprotobuf-c -lzookeeper_mt

# Ficheiros base da biblioteca list
SRC = source/data.c source/list.c  
OBJ = $(SRC:source/%.c=$(OBJ_DIR)/%.o)

# Protobuf
PROTO_PROTO = sdmessage.proto
PROTO_SRC = source/sdmessage.pb-c.c
PROTO_HDR = source/sdmessage.pb-c.h

# Cliente
CLIENT_SRC = source/client_stub.c source/list_client.c source/network_client.c $(PROTO_SRC)
CLIENT_OBJ = $(CLIENT_SRC:source/%.c=$(OBJ_DIR)/%.o)

# Servidor
SERVER_SRC = source/list_server.c source/list_skel.c source/network_server.c \
             source/server_log.c source/client_stub.c source/network_client.c $(PROTO_SRC)
SERVER_OBJ = $(SERVER_SRC:source/%.c=$(OBJ_DIR)/%.o)

# Ficheiro message-private.c 
MESSAGE_PRIVATE_SRC = source/message-private.c
MESSAGE_PRIVATE_OBJ = $(OBJ_DIR)/message-private.o

# Alvos principais
.PHONY: all liblist list_client list_server clean

all: liblist list_client list_server

# Biblioteca estática com data.c e list.c
liblist: $(OBJ)
	@mkdir -p $(LIB_DIR)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

# Cliente
list_client: $(CLIENT_OBJ) $(LIB) $(MESSAGE_PRIVATE_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $(CLIENT_OBJ) $(LIB) $(MESSAGE_PRIVATE_OBJ) $(LDLIBS)

# Servidor
list_server: $(SERVER_OBJ) $(LIB) $(MESSAGE_PRIVATE_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $(SERVER_OBJ) $(LIB) $(MESSAGE_PRIVATE_OBJ) $(LDLIBS)

# Compilar
$(OBJ_DIR)/%.o: source/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Limpeza
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/list_client $(BIN_DIR)/list_server $(LIB_DIR)/liblist.a
	rm -f *.log