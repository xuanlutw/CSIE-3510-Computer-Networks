EXE = server

SRC_DIR = src
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CPPFLAGS += -Iinclude -Ipthread
CFLAGS += -Wall -g
LDFLAGS += -Llib -pthread
LDLIBS += -lm

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

client: cl/*.c obj/utils.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $< obj/utils.o -o client 

clean:
	$(RM) $(OBJ) server client

