#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>


// portable reasons
#ifdef _DIRENT_HAVE_D_TYPE
#define HAVE_D_TYPE 1
#else
#define HAVE_D_TYPE 0
#endif

#define BUFSIZE 1024

char const* procMountAt = "/proc";
char const* procStatAtFmt = "/proc/%d/stat";	// all stat information about process (pid, ppid, state ...)
char const* procCmdAtFmt = "/proc/%d/cmdline";	// commandline

// return 1 if is a directory, 0 if regular file, -1 otherwise
int checkFileTypeStat(char const* filePath){
	struct stat fstat;
	memset(&fstat, 0, sizeof(fstat));

	if (stat(filePath,  &fstat)){
		return -1;
	}
	if ((fstat.st_mode&S_IFMT) == S_IFDIR){	// is directory
	       return 1;
	}	       
	if ((fstat.st_mode&S_IFMT) == S_IFREG){	// is normal file
	       return 0;
	}	       
	return -1;
}

// return 1 if is a directory, 0 if regular file, -1 otherwise
int checkFileType(struct dirent* entry){
	if (HAVE_D_TYPE){	// reused 
		switch(entry->d_type){
			case DT_DIR:	// dir
				return 1;
			case DT_REG:	// regular file
				return 0;
			default:
				return -1;
		}
	}
	return checkFileTypeStat(entry -> d_name);
}


// return true if all characters in s are numeric
int isNumericStr(char const* s){
	for (int i=0; i<strlen(s); ++i){
		if (s[i] < '0' || s[i] > '9')
			return 0;
	}
	return 1;
}

// read all file content
char* readAllFile(char const* filePath, int fillNull){
	FILE* of = NULL;
	char* buffer = NULL;
	int nread = 0;
	int bufLen = 0;

	if (checkFileTypeStat(filePath) != 0){	// normal files only 
		return NULL;
	}
	if ((of = fopen(filePath, "rb")) == NULL){	// io errors
		return NULL;
	}
	buffer = (char*)malloc(BUFSIZE);
	memset(buffer, 0, BUFSIZE);

	char* tmp = NULL;
	char* p = buffer;
	while((nread=fread(p, 1, BUFSIZE, of)) == BUFSIZE){
			bufLen += nread;
			if ((tmp = realloc(buffer, bufLen + BUFSIZE)) != NULL){	// realloc fails
				buffer = tmp;
			} else {
				free(buffer);
				return NULL;
			}
			p = buffer + bufLen;	// next reading portion
	}
	if (ferror(of)){
		return NULL;
	}

	bufLen += nread;
	if (buffer[bufLen - 1] == '\x00'){
		bufLen -= 1;
	}
	if (bufLen <= 0){	// empty
		return NULL;
	}
	if (fillNull){	// replace null by whitespace
		// command-line arguments as set of strings seperated by null
		while(strlen(buffer) != bufLen){
			buffer[strlen(buffer)] = ' ';
		}
	}

	fclose(of);
	return buffer;
}

int main(int argc, char const** argv){
	char buffer[BUFSIZE];
	memset(buffer, 0, sizeof(buffer));

	DIR* dirp = opendir(procMountAt);				
	if (dirp == NULL){
		printf("ERROR: cannot open '%s' directory\n", procMountAt);
		return 1;
	}

	while (1){
		errno = 0;
		struct dirent* entry = readdir(dirp);	
		if (entry == NULL){
			if (errno != 0){	// io errors
				perror(NULL);
				closedir(dirp);
				return 1;
			}
			break;	// endof dir
		}
		// processComm
		char const* comm = entry -> d_name;	// at most NAME_MAX
		if (comm[0] == '.' || !isNumericStr(comm) || checkFileType(entry) != 1)	// special files
			continue;
		// processID
		int pid = atoi(comm);
		// processCmd
		sprintf(buffer, procCmdAtFmt, pid);
		char* cmd = readAllFile(buffer, 1);

		// processStat
		sprintf(buffer, procStatAtFmt, pid);
		char* pstat = readAllFile(buffer, 0);

		// parentPID
		int ppid = -1;
		if (pstat != NULL){
			sscanf(strchr(pstat, ')') + 1, "%*s %d", &ppid);	// docs guarantees no error
		}	
		printf("processID   = %d\n", pid);
		printf("parentPID   = %d\n", ppid);
		printf("cmd  	    = %s\n", cmd != NULL ? cmd : "-");
		printf("\n");

		free(cmd);
		free(pstat);
	}
	
	closedir(dirp);
	return 0;
}
