#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void parse(char *par, int offset);

char *new_argv[64];

int main(int argc, char **argv)
{
    char buf[1024];
    int t = 0, i = 0;

    // copy fixed arguments
    for(int j = 1; j < argc; j++){
        new_argv[i++] = argv[j];
    }

    // Read all input from stdin into buf
    int n;
	while((n = read(0, buf+t, sizeof(buf))) > 0)
		t+= n;

    buf[t] = 0;  // null-terminate

    // Parse buf and append tokens after the fixed args
    parse(buf, i);

    if(fork() == 0) {
		exec(argv[1], new_argv);
		exit(1); // exec failed
    } 
	else{
        wait(0);
        exit(0);
    }
}

void
parse(char *par, int offset)
{
    int i = offset;

    // skip initial whitespace
    while (*par == ' ' || *par == '\n')
        par++;
    while (*par != 0) {
        // start of a token
        char *start = par;
        // move until whitespace or null
        while (*par != ' ' && *par != '\n' && *par != 0)
            par++;
        // terminate token by setting ' ' or '\n' to 0 
        if (*par != 0) {
            *par = 0;
            par++;
        }
		// new token start
        new_argv[i++] = start;
        // skip whitespace before next token
        while (*par == ' ' || *par == '\n')
            par++;
    }
    new_argv[i] = 0;
}
