all: museum
clean:
	@rm -f museum
	@rm -f museum.o

museum: museum.o
