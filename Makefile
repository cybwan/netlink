#!make

EXEC   := linklist
SRCS   := $(EXEC).c
OBJS   := $(EXEC).o
LDLIBS := $(shell pkg-config --libs   libnl-route-3.0 libnl-3.0)
LDLIBS += -lev
CFLAGS := $(shell pkg-config --cflags libnl-route-3.0 libnl-3.0)
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


.PHONY: nlp-build
nlp-build:
	@CGO_ENABLED=1 go build -v -o ./bin/nlp ./cmd/nlp/*

.PHONY: nlp-run
nlp-run:
	@./bin/nlp

.PHONY: nlp
nlp: nlp-build nlp-run