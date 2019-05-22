sieve_mpi: sieve_mpi.c
	mpicc sieve_mpi.c -o sieve_mpi

sieve_luv: sieve_luv.c 
	gcc -g -O2 -luv sieve_luv.c -o sieve_luv

sieve_co_stream: sieve_co_stream.c
	gcc -g -O2 -Ilibaco/ libaco/acosw.S libaco/aco.c sieve_co_stream.c -o sieve_co_stream
  
sieve_co_array: sieve_co_array.c
	gcc sieve_co_array.c -o sieve_co_array

sieve_sub: sieve_sub.c 
	gcc sieve_sub.c -o sieve_sub


all: sieve_co_array sieve_sub sieve_co_stream sieve_luv sieve_mpi

clean: 
	$(RM) *.o sieve_sub sieve_co_array sieve_co_stream sieve_server
