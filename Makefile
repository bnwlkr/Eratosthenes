sieve_luv: sieve_luv.c 
	gcc -g -O2 -luv sieve_luv.c -o sieve_luv

sieve_co_stream: sieve_co_stream.c
	gcc -g -O2 -Ilibaco/ libaco/acosw.S libaco/aco.c sieve_co_stream.c -o sieve_co_stream
  
sieve_co_array: sieve_co_array.c utils.o
	gcc sieve_co_array.c utils.o -o sieve_co_array

sieve_sub: sieve_sub.c utils.o
	gcc sieve_sub.c utils.o -o sieve_sub

utils.o: utils.c
	gcc -c utils.c


all: sieve_co_array sieve_sub sieve_co_stream

clean: 
	$(RM) *.o sieve_sub sieve_co_array sieve_co_stream sieve_server
