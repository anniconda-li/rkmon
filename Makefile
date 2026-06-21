CC := gcc
CFLAGS := -Wall -Wextra -O2
TARGET := rkmon
SRC := src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean