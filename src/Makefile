CC      ?= gcc
CFLAGS  ?= -pthread -Wall -O2 -D_GNU_SOURCE
LDFLAGS ?= -lgpiod -lrt

SRCS = main.c gpio_handler.c gpio_config.c
OBJS = $(SRCS:.c=.o)

TARGET = rt-handler

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGET)
