CFLAGS+=-std=c99

all: museum bank
clean:
	@rm -f museum
	@rm -f museum.o

museum: museum.o double_queue.o util.o
bank: bank.o double_queue.o util.o

double_queue.o: double_queue.c double_queue.h debug.h util.h
bank.o: bank.c common.h double_queue.h debug.h
museum.o: museum.c common.h debug.h double_queue.h

util.o: util.c util.h