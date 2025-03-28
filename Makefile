CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags libnm)
LDFLAGS = $(shell pkg-config --libs libnm)

TARGET = nm
SRC = main2.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
