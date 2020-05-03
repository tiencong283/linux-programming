#define _GNU_SOURCE 1
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

char const* question = "\
	Write a program that calls fork() and then calls some form of \
exec() to run the program /bin/ls. See if you can try all of the \
variants of exec(), including (on Linux) execl(), execle(), \
execlp(), execv(), execvp(), and execvpe(). Why do \
you think there are so many variants of the same basic call?";

char const* answer= "\
	All functions start with exec and do the same functionality (here is to load another program, replace current one) \
but offers different ways to pass arguments. Three components: path to ELF file (1), arguments (2) and environments (3) \n\
1. required, by default it must be an absolute filepath, if 'p' (PATH) in function name -> it may be a filename (exec will search in $PATH env) \n\
2. required, specify arguments in two ways like main ('v' -> vector) or a list ('l' -> list) \n\
3. optional, if needed environment use functions with letter 'e'";

int main(int argc, char const** argv){
	int pid;
	// environment vector
	char* env[] = {
		"PWD=/home",
		NULL
	};
	// argument vector
	char* args[] = {
		"ls",
		NULL
	};

	printf("\tQ: %s\n\n", question);

	printf("Forking\n");	
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			{
				// use one of them
				execl("/bin/ls", "ls", NULL);	// just 'ls' will fail
				execle("/bin/ls", "ls", NULL, env);
				execlp("ls", "ls", NULL);
				execv("/bin/ls", args);
				execvp("ls", args);
				execvpe("ls", args, env);
			}
			break;
		default:
			wait(NULL);
			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
