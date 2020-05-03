#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

char const* question = "\
	Write a program that uses wait() to wait for the child process \
to finish in the parent. What does wait() return? What happens if \
you use wait() in the child?";

char const* answer= "\n\
	1. wait() returns the pid of just-terminated child process\n\
	2. if using wait() in child -> if the child has no any children -> wait will return immediately";

int main(int argc, char const** argv){
	int pid;

	printf("\tQ: %s\n\n", question);

	printf("Forking\n");	
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			{
				// use one of them
				if (wait(NULL) == -1){
					perror("In wait");
				}
			}
			break;
		default:
			wait(NULL);
			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
