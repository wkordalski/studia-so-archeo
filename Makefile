CFLAGS+=-std=c99 -pthread -D_GNU_SOURCE
LDFLAGS+=-lpthread

all: museum bank
clean:
	@rm -f museum
	@rm -f bank
	@rm -f ./*.o

museum: museum.o double_queue.o util.o
bank: bank.o double_queue.o util.o company.o

double_queue.o: double_queue.c double_queue.h debug.h util.h
bank.o: bank.c common.h double_queue.h debug.h
museum.o: museum.c common.h debug.h double_queue.h
company.o: company.c company.h

util.o: util.c util.h
