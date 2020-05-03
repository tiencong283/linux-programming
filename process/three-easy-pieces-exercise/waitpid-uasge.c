#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

char const* question = "\
	Write a slight modification of the previous program (wait-in-child.c), this time using \
waitpid() instead of wait(). When would waitpid() be useful?";

char const* answer= "\n\
	waitpid() differs from wait() in that it can wait for a specific or a group of processes";

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
			// wait until process with pid has exited
			waitpid(pid, NULL, 0);
			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
