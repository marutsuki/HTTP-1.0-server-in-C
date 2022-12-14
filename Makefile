CC=gcc
CFLAGS=-Wall -pthread
EXE=server
EXE_DEPS=server.o socket.o http.o http_domain.o socket_worker.o socket_worker_adaptor_http.o util.o

HEADER_DIR = ./header
BUILD_DIR = ./build
SRC_DIR = ./src

OBJ = $(patsubst %, $(BUILD_DIR)/%, $(EXE_DEPS))

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADER_DIR)/%.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BUILD_DIR)/*.o $(EXE)