#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

char const* question = "\
Write a program that opens a file (with the open() system call) \
and then calls fork() to create a new process. Can both the child \
and parent access the file descriptor returned by open()? What \
happens when they are writing to the file concurrently, i.e., at the \
same time?";

char const* answer= "open files among one of many properties of the parent are inherited by the child, \
they are made in the manner of dup() syscall -> corresponding file descriptors on the child and parent both refer to the same \
open file description (thus attributes like flag and offset shared between them). When both writing to the file concurrently, it's guaranteed that not overwriting each other(because offset updated and write operation is atomic) but not the order (the output is randomly intermingled).";

char const* fileName = "fork-file-sharing-output.txt";
#define OFLAG (O_RDWR | O_CREAT | O_TRUNC)
// 0x755 permission
#define PER (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

#define WRITE_TIMES 10000
int writeFile(int fd, char const* val){
	int len = strlen(val);
	for (int i=0; i<WRITE_TIMES; ++i){
		write(fd, val, len); 
	}
}

int main(int argc, char const** argv){
	int pid;
	int fd;
	printf("\tQ: %s\n\n", question);
	
	if ((fd=open(fileName, OFLAG, PER)) == -1){
		perror("open");
		return 1;
	}
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			{
				int flag = fcntl(fd, F_GETFL);
				if (flag == -1){
					printf("the child does not inherit the open file descriptor\n");
				} else if (flag&OFLAG){	// preserve flag
					printf("the child does inherit the open file descriptor\n");
				}
				printf("in child, start writing 1 (%d times)\n", WRITE_TIMES);
				writeFile(fd, "1\n");
			}
			break;
		default:
			printf("in parent, start writing 0 (%d times)\n", WRITE_TIMES);
			writeFile(fd, "0\n");
			wait(NULL);
			printf("\n\tA: %s\n", answer);
	}
	close(fd);
	return 0;
}
