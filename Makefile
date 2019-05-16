sieve_co_array: sieve_co_array.o utils.o
	gcc -o sieve_co_array sieve_co_array.o utils.o

sieve_sub: sieve_sub.o 
	gcc -o sieve_sub sieve_sub.o utils.o

sieve_sub.o: sieve_sub.c
	gcc -c sieve_sub.c

sieve_co_array.o: sieve_co_array.c 
	gcc -c sieve_co_array.c

utils.o: utils.c
	gcc -c utils.c

all: sieve_co_array sieve_sub

clean: 
	$(RM) *.o sieve_sub sieve_co_array
