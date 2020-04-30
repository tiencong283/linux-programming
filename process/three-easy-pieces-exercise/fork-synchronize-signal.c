#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

// synchronization signal 
#define SYNC_SIG SIGUSR1

char const* question = "\
Write another program using fork(). The child process should \
print “hello”; the parent process should print “goodbye”. You should \
try to ensure that the child process always prints first; can you do \
this without calling wait() in the parent?.";

char const* answer="\
By using Signals, the child can send a notification to the parent that it has done some work.";

void sigHandler(int sig){
}

int main(int argc, char const** argv){
	struct sigaction action;	
	sigset_t blockMask, orgMask, emptyMask;

	printf("\tQ: %s\n\n", question);

	// disable buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	// block SYNC_SIG 
	sigemptyset(&blockMask);
	sigaddset(&blockMask, SYNC_SIG);
	if (sigprocmask(SIG_BLOCK, &blockMask, &orgMask) == -1){
		perror("In sigprocmask");
		return 1;
	}

	// install the signal
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = sigHandler;
	sigemptyset(&action.sa_mask);
	
	if (sigaction(SYNC_SIG, &action, NULL) == -1){
		perror("In sigaction");
		return 1;
	}

	printf("Forking\n");
	switch(fork()){
		case -1:
			fprintf(stderr, "ERROR: cannot fork\n");
			return 1;
		case 0:
			// doing some work
			printf("The child started - doing some work\n");
			sleep(2);
			
			// signal its parent that it's done
			printf("The child is going to signal its parent\n");
			if (kill(getppid(), SYNC_SIG) == -1){
				perror("In kill");
				return 1;
			}			
			break;
		default:
			// wait for the signal
			printf("The parent is going to wait for signal\n");
			sigemptyset(&emptyMask);
			if (sigsuspend(&emptyMask) == -1 && errno!=EINTR){	// always return -1
				perror("In sigsuspend");
				return 1;
			}
			// got signal
			printf("The parent got signal\n");
			// restore to its original state
			if (sigprocmask(SIG_BLOCK, &blockMask, &orgMask) == -1){
				perror("In sigprocmask");
				return 1;
			}
			printf("The parent continues doing its work\n");

			printf("\n\tA: %s\n", answer);
	}
	return 0;
}
