#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int find(char* path, char* file);


int main(int argc, char** argv){
	if(argc != 3){
		printf("use: %s <dir> <file>\n", argv[0]);
		return 1;	
	}

	find(argv[1], argv[2]);

	return 0;
}

int find(char* path, char* target){
    int fd;
    struct stat st;
    struct dirent di;
    char buf[512], *p;

    if ((fd = open(path, 0)) < 0) {
        printf("Cannot open %s\n", path);
        return 1;
    }
    if (fstat(fd, &st) < 0) {
        close(fd);
        return 1;
    }
    if (st.type != T_DIR) {
		printf("Cannot find dir %s\n",path);
        close(fd);
        return 1;
    }

    // Copy path into buf
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &di, sizeof(di)) == sizeof(di)) {
        // skip . and ..
        if (di.inum == 0 || strcmp(di.name, ".") == 0 || strcmp(di.name, "..") == 0)
            continue;

        // build full path
        memmove(p, di.name, DIRSIZ);
        p[DIRSIZ] = '\0'; // NULL terminate

        // stat this file/directory
        if (stat(buf, &st) < 0)
            continue;

        // if it's the target file, print it
        if (strcmp(di.name, target) == 0) {
            printf("%s\n", buf);
			close(fd);
			return 0;
        }

        // if it's a directory, recurse
        if (st.type == T_DIR) {
            find(buf, target);
        }
    }
	return 1;
}

