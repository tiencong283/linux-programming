#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>	// file io headers
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>	// rlimit
#include <sys/resource.h>
#include <assert.h>	// assert
#include <unistd.h>	// read

#define BUFSIZE 1024

static int max_nfd = 0;	// soft_limit

void set_max_nfd(){
	struct rlimit nfd_limit;	// see getrlimit
	memset(&nfd_limit, 0, sizeof(nfd_limit));

	if (getrlimit(RLIMIT_NOFILE, &nfd_limit) == -1){
		printf("ERROR: cannot get num_fd_limit info\n");
		exit(1);	
	}
	max_nfd = nfd_limit.rlim_cur;
}

int is_open_fd(int fd){
	return fcntl(fd, F_GETFD) != -1;
}

// return true if is in valid outbound
int is_valid_fd(int fd){
	if (fd < 0){
		return 0;
	}
	if (fd > max_nfd){
		return 0;
	}
	return 1;
}

int custom_dup2(int fd, int new_fd){

/*
If oldfd is not a valid file descriptor, then the call fails, and newfd is not closed.
If oldfd is a valid file descriptor, and newfd has the same value as oldfd, then dup2() does nothing, and returns newfd.
otherwise If the file descriptor newfd was previously open, it is silently closed before being reused.

*/
	if (!is_valid_fd(fd) || !is_valid_fd(new_fd)){
		printf("ERROR: invalid file descriptor parameter\n");
		return -1;
	}
	if (!is_open_fd(fd)){
		printf("ERROR: dup an closed fd\n");
		return -1;
	}
	if (fd == new_fd){
		return new_fd;
	}
	if (is_open_fd(new_fd)){	// If the file descriptor newfd was previously open
		close(new_fd);
	}
	int tfds[max_nfd];
	int i = 0;
	int tfd = -1;
	memset(tfds, 0, sizeof(int) * max_nfd);
	while((tfd = dup(fd)) != new_fd && tfd != -1){	// guarantee that at some point the equation will be equal
		tfds[i] = tfd;
		i += 1;
	}
	while(i >= 0){	// close all the unused fds
		i -= 1;
		close(tfds[i]);
	}
	return new_fd;
}

int main(int argc, char const** argv){
	char buf[BUFSIZE];

	if (argc < 2){
		printf("Usage: %s <file_to_open>\n", argv[0]);
		return 0;
	}
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1){
		printf("ERROR: cannot open file '%s'\n", argv[1]);
		return 1;
	}
	set_max_nfd();
	// dup open file
	int new_fd = custom_dup2(fd, 10);
	// dup stdout
	int new_stdout = custom_dup2(STDOUT_FILENO, 11);

	// confirm opening
	assert(is_open_fd(new_fd) == 1);

	// confirm reading
	memset(buf, 0, BUFSIZE);
	int nread = read(new_fd, buf, BUFSIZE - 1);

	// confirm writing
	buf[nread-1] = '\n';
	write(new_stdout, buf, BUFSIZE - 1);
	return 0;
}
