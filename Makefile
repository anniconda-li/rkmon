CC := gcc
CFLAGS := -Wall -Wextra -O2 -std=c11
TARGET := rkmon
SRC := src/main.c src/collect.c src/output.c src/net.c
HEADERS := src/rkmon.h src/collect.h src/output.h src/net.h

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
