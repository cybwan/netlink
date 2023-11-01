#!make

LIB    := nlp.so
EXEC   := nlp
SRCS   := netlink/attr.c netlink/cache.c netlink/cache_mngt.c netlink/genl.c netlink/genl_ctrl.c netlink/genl_family.c netlink/genl_mngt.c netlink/handlers.c netlink/msg.c netlink/nl.c netlink/object.c netlink/socket.c netlink/unl.c netlink/error.c link.c neigh.c addr.c route.c filter.c nlops.c net_api.c main.c
OBJS   := netlink/attr.o netlink/cache.o netlink/cache_mngt.o netlink/genl.o netlink/genl_ctrl.o netlink/genl_family.o netlink/genl_mngt.o netlink/handlers.o netlink/msg.o netlink/nl.o netlink/object.o netlink/socket.o netlink/unl.o netlink/error.o link.o neigh.o addr.o route.o filter.o nlops.o net_api.o main.o
OBJSSO := netlink/attr.c netlink/cache.c netlink/cache_mngt.c netlink/genl.c netlink/genl_ctrl.c netlink/genl_family.c netlink/genl_mngt.c netlink/handlers.c netlink/msg.c netlink/nl.c netlink/object.c netlink/socket.c netlink/unl.c netlink/error.c link.c neigh.c addr.c route.c filter.c nlops.c net_api.c main.c
LDLIBS := 
CFLAGS := 
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
	$(RM) */*.o *.o *.so *~ *.bak
