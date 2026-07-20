CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=gnu11
LDFLAGS ?=
TARGET = edge-monitor

all: $(TARGET)

$(TARGET): edge_monitor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) edge_monitor.c

clean:
	rm -f $(TARGET)
