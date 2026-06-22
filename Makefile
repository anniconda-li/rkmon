CC := gcc
CFLAGS := -Wall -Wextra -O2
TARGET := rkmon
SRC := src/main.c src/rkmon.c
HEADERS := src/rkmon.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
