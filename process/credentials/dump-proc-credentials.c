#include <stdio.h>
#include <sys/types.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/fsuid.h>
#include <errno.h>

char const* tips = "\
TIPS: make set-user-ID executable\n\
sudo chown root:root dump-proc-credentials\n\
sudo chmod u+s ./dump-proc-credentials";

void dump(){
	int uid, euid, suid;
	int gid, egid, sgid;
	if (getresuid(&uid, &euid, &suid) == -1 || getresgid(&gid, &egid, &sgid) == -1){
		return;
	}
	printf("real userID = %d, groupID = %d\n", getuid(), getgid());
	printf("effective userID = %d, groupID = %d\n", euid, egid);
	printf("saved userID = %d, groupID = %d\n", suid, sgid);
	printf("file-system userID = %d, groupID = %d\n", setfsuid(euid), setfsgid(egid));
	printf("\n");
}

int main(int argc, char const** argv){
	int euid;
	int uid;

	printf("%s\n\n", tips);
	printf("processID = %d\n", getpid());
	dump();
	euid = geteuid();
	uid = getuid();

	if (geteuid() != 0){
		return 0;
	}
	printf("call seteuid(%d) will temporarily drop the root\n", uid);
	if (seteuid(uid) == -1){
		perror("seteuid");
		return 1;
	}
	dump();

	printf("call seteuid(%d) will regain the root\n", euid);
	if (seteuid(euid) == -1){
		perror("seteuid");
		return 1;
	}
	dump();
	
	printf("call setuid(%d) will lose all privileges (one-way trip)\n", uid);
	if (setuid(uid) == -1){
		perror("seteuid");
		return 1;
	}
	dump();

	printf("call seteuid(%d) to regain the root will fail now\n", euid);
	if (seteuid(euid) == -1){
		perror("seteuid");
		return 1;
	}
	return 0;
}
