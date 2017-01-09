CC = gcc
TARGET = mydhcps.out
SRCS = mydhcps.c
OBJS = mydhcps.o
DEPS = mydhcps.h packet.h

LDFLAGS =
CFLAGS = -g -O
DFLAGS = -g

RM = rm -f

all: $(TARGET)

$(TARGET): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(DFLAGS) -o $@ $^

.c.o:
	$(CC) -c $(DFLAGS) $<

clean:
	$(RM) $(OBJS)

clean_target:
	$(RM) $(TARGET)

clean_all:
	$(RM) $(TARGET) $(OBJS)
