#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

char const* question = "\
	Write a program that creates two children, and connects the standard \
output of one to the standard input of the other, using the \
pipe() system call.";

int main(int argc, char const** argv){
	int pid;
	int pipefd[2];

	printf("\tQ: %s\n\n", question);
	printf("\tA: simulate 'ls | wc -l'\n");
	if (pipe(pipefd) == -1){
		perror("In pipefd");
		return 1;
	}

	printf("Create process 'ls'\n");	
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			{
				close(pipefd[0]);	 // close read pipe
				if (dup2(pipefd[1], STDOUT_FILENO) != STDOUT_FILENO){
					perror("In dup2");
				}
				if (dup2(pipefd[1], STDERR_FILENO) != STDERR_FILENO){
					perror("In dup2");
				}
				execlp("ls", "ls", NULL); 
			}
			break;
		default:
			close(pipefd[1]);	// close write pipe
			waitpid(pid, NULL, 0);
	}

	printf("Create process 'wc -l'\n");	
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			{
				if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO){
					perror("In dup2");
				}
				execlp("wc", "wc", "-l", NULL); 
			}
			break;
		default:
			close(pipefd[0]);
			waitpid(pid, NULL, 0);
	}

	return 0;
}
