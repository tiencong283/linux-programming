#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

char const* question = "\
	Write a program that creates a child process, and then in the child \
closes standard output (STDOUT FILENO).What happens if the child \
calls printf() to print some output after closing the descriptor?";

char const* answer= "\n\
	When standard output is closed, printf will print nothing just because slot 1 of open file table is empty";

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
				printf("Closing standard output\n");
				close(STDOUT_FILENO);
				printf("After closed stdand output\n");
			}
			break;
		default:
			// wait until process with pid has exited
			waitpid(pid, NULL, 0);
			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
