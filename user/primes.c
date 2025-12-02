#include "kernel/types.h"
#include "kernel/syscall.h"
#include "kernel/stat.h"
#include "user/user.h"

int sieve(int fd);

int p1[2];

int main(void){
	pipe(p1);

	if(fork() == 0){ // first child
		close(p1[1]);
		sieve(p1[0]);
	}
	else{ // parent
		close(p1[0]);
		// write numbers to pipe
		for(int i=2; i < 36; i++) 	
			write(p1[1],&i,sizeof(i));
		close(p1[1]);
		wait(0);
	}
}

// Sieve prime numbers using forks and pipes
int sieve(int fd){
	int prime;
	// read and print the first number from the previous process
	// exit if there is nothing to read
	if(read(fd, &prime, sizeof(prime)) != sizeof(prime)){
		close(fd);
		return 0;
	}
	printf("prime %d\n",prime);

	// create a new pipe for the new process
	int p[2];
	pipe(p);

	if(fork() == 0){ // next child
		// child does not want to write
		close(p[1]);
		// takes the input from the current pipe
		// which the parent wrote the sieved numbers to
		sieve(p[0]);
		exit(0);
	}
	else{
		// Parent does not want to read 
		close(p[0]);
		int num;
		// parent passes the numbers that are not multiples of the current 
		// highest number to the next process by writing to the pipe
		while(read(fd, &num, sizeof(int)) == sizeof(int)){
			if(num % prime != 0)
				write(p[1], &num, sizeof(prime));
		}
		close(fd);
		close(p[1]);
		wait(0);	
	}
	return 1;
}
