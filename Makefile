CC = gcc
DEPS = libacc.h

.PHONY: all clean test

all: store libacc.o libacc.a client

store: store.c
	@$(CC) $< -pthread -o $@

libacc.o: libacc.c $(DEPS)
	@$(CC) -c $<

libacc.a: libacc.o
	@ar ru $@ $<

client: client.c libacc.a $(DEPS)
	@$(CC) $< -L. -lacc -o $@

perm:
	@chmod +x test.sh
	@chmod +x testsum.sh

clean:
	@rm -f store client libacc.o libacc.a objstore.sock testout.log formtestout.log *~
	@rm -rf data

test:
	@./test.sh
	@./testsum.sh
