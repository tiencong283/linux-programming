#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

char const* question = "\
Write a program that calls fork(). Before calling fork(), have the \
main process access a variable (e.g., x) and set its value to something \
(e.g., 100). What value is the variable in the child process? \
What happens to the variablewhen both the child and parent change \
the value of x?";

char const* answer="\
fork() system call create new process by making almost exact duplicate of the parent \
so both of them have the same virtual memory (text segments, data ...) but \
they just are copies -> make any changes in one process does not reflect in the other.";
 
int num = 100;
int main(int argc, char const** argv){
	int pid;
	printf("\tQ: %s\n\n", question);
	printf("before fork(), num=%d\n", num);
	printf("forking\n");
	switch((pid=fork())){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			printf("in child, num=%d\n", num);
			printf("in child, set num=200\n");
			num = 200;
			break;
		default:
			wait(NULL);
			printf("in parent, child has changed num but still num=%d\n", num);
			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
