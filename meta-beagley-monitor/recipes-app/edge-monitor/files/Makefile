CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=gnu11
LDFLAGS ?=
TARGET = edge-monitor
SRCS = edge_monitor.c monitor_logic.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)