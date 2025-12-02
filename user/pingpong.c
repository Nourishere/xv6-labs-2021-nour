#include "kernel/types.h"
#include "kernel/syscall.h"
#include "kernel/stat.h"
#include "user/user.h"
#define BUFF_SIZE 64
/* pingpong.c: send a pair of bytes between two processes over two pipes. */

int p1[2]; // parent writes 
int p2[2]; // child writes

int stin, stout;

char buff[1];
char buff2[1];
/*   child (rd)(p1[0]) ------------------------------------ (wr)(p1[1]) parent            */
/*         (wr)(p2[1]) ------------------------------------ (rd)(p2[0])                   */
/*                         												                  */
int main(void){
	if(pipe(p1) < 0 || pipe(p2) < 0) /* Creates two pipes */
		return -1;
	int pid = fork();
	int paid = getpid();
	char par[1] = {'1' + (char) paid};
	char chi[1] = {'1' + (char) pid};
	if(pid == 0){ /* Child */
		stin = dup(0);
		stout = dup(1);
		if(close(p1[1]) < 0) // close the unused side of the first pipe
			return -1; 
		if(close(0) < 0)
			return -1; 
		dup(p1[0]); // stdin comes from the read side of the first pipe	
		if(close(p1[0]) < 0)
			return -1; 
		if(close(p2[0]) < 0) // close the unused side of the second pipe
			return -1; 
		if(close(1) < 0)
			return -1; 
		dup(p2[1]); // stdout goes to the write end of the second pipe
		if(close(p2[1]) < 0) // close the unused side of the second pipe
			return -1; 
		/* Read from pipe */
		if(read(0,buff2,1) < 0)
			return -1;
		if(buff2[0] == 'a'){
			write(stout,chi,1);
			write(stout,": ",2);
			write(stout, "received ping\n",14);
		}	
		/* Write to pipe */
		if(write(1,"b",1) < 0)
			return -1;
		exit(0);
	}
	else if (pid > 0){ /* Parent */
		stin = dup(0);
		stout = dup(1);
		if(close(p1[0]) < 0 || close(p2[1]) < 0) // close the unused ends of the pipes
			return -1;
		if(close(0) < 0)
			return -1;
		dup(p2[0]);
		if(close(p2[0]) < 0) 
			return -1; 
		if(close(1) < 0)
			return -1;
		dup(p1[1]);	
		if(close(p1[1]) < 0) 
			return -1; 
		/* Write to pipe */
		if(write(1,"a",1) < 0)
			return -1;
		/* Read from pipe */
		if(read(0,buff,1) < 0)
			return -1;
		if(buff[0] == 'b'){
			write(stout,par,1);
			write(stout,": ",2);
			write(stout, "received pong\n",14);
		}	
		else
			return -1;
		exit(0);
	}
	else{ /* Error */
		return -1;
	}
}
