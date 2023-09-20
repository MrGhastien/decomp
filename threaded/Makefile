CFLAGS = -I$(INC_DIR) -O1

SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
HDRS = $(wildcard $(INC_DIR)/*.h)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = decomp

.PHONY: all clean

all: $(TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(OBJ_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	gcc $(CFLAGS) $^ -o $@

$(OBJ_DIR):
	mkdir $@

clean:
	rm -r $(OBJ_DIR)
	rm $(TARGET)
