CC = gcc
TARGET = mydhcpc.out
SRCS = mydhcpc.c
OBJS = mydhcpc.o
DEPS = mydhcpc.h packet.h

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
