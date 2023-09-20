CFLAGS = -I$(INC_DIR) -g -ldl -Wl,--export-dynamic -lm

SRC_DIR = ./src
TST_DIR = ./tests
INC_DIR = ./include
OBJ_DIR = ./obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
TSTS = $(wildcard $(TST_DIR)/*.c)
HDRS = $(wildcard $(INC_DIR)/*.h)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
TEST_OBJS = $(patsubst $(TST_DIR)/%.c,$(OBJ_DIR)/tests/%.o,$(TSTS))

TARGET = decomp
TEST_TARGET = test-decomp.so

.PHONY: all clean

all: $(TARGET) $(TEST_TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HDRS) | $(OBJ_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/tests/%.o: $(TST_DIR)/%.c $(HDRS) | $(OBJ_DIR)
	gcc $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	gcc $(CFLAGS) $^ -o $@

$(TEST_TARGET): $(TEST_OBJS)
	gcc $(CFLAGS) -fPIC -shared $^ -o $@

$(OBJ_DIR):
	mkdir -p $@/tests

clean:
	rm $(OBJS)
	rm $(TEST_OBJS)
	rm $(TARGET)
	rm $(TEST_TARGET)
