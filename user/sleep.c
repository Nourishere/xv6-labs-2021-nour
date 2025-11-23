#include "kernel/types.h"
#include "kernel/syscall.h"
#include "kernel/stat.h"
#include "user/user.h"

/* sleep.c: delay for some time */
int main(int argc, char* argv[]){
	if(argc < 2){
		printf("use: sleep <time>\n");
		return -1;
	}
	else{
		int ret = fork();
		if (ret == 0){ /* Child */
			int t = atoi(argv[1]);
			if(!t)
				return -1;
			if(sleep(t) < 0)
				exit(1);
			else
				exit(0);
		}
		else if(ret > 0){ /* Parent */
			wait((int*)0);
		}
		else{ /* error */
			printf("Error forking\n");
			return -1;
		}
	}
}
