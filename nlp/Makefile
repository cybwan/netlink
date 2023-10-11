#!make

EXEC   := nlp
SRCS   := attr.c link.c
OBJS   := attr.o link.o
LDLIBS := $(shell pkg-config --libs   libnl-route-3.0 libnl-3.0)
LDLIBS += -lev
CFLAGS := $(shell pkg-config --cflags libnl-route-3.0 libnl-3.0)
CFLAGS += -I./headers
CFLAGS += -g -Og -W -Wall -Wextra -Wno-unused-parameter

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(EXEC) $(OBJS)

run:
	./$(EXEC)

distclean: clean
	$(RM) *.o *~ *.bak
