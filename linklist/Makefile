#!make

EXEC   := linklist
SRCS   := $(EXEC).c
OBJS   := $(EXEC).o
LDLIBS := 
CFLAGS :=
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