#!make

LIB    := nlp.so
EXEC   := nlp
SRCS   := link.c neigh.c addr.c route.c filter.c net_api.c main.c
OBJS   := link.o neigh.o addr.o route.o filter.o net_api.o main.o
OBJSSO := link.o neigh.o addr.o route.o filter.o
LDLIBS := $(shell pkg-config --libs   libnl-route-3.0 libnl-3.0)
LDLIBS += -lev
CFLAGS := $(shell pkg-config --cflags libnl-route-3.0 libnl-3.0)
CFLAGS += -I./headers
CFLAGS += -g -Og -fPIC -W -Wall -Wextra -Wno-unused-parameter

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

$(LIB): $(OBJSSO)
	$(CC) -shared -o lib$@ $^ $(LDLIBS)

clean:
	$(RM) $(EXEC) lib$(LIB) $(OBJS)

run:
	./$(EXEC)

distclean: clean
	$(RM) *.o *.so *~ *.bak
