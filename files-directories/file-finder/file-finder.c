#include <stdlib.h>
#include <argp.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


/**
 * Argp (arguments parser): https://www.gnu.org/software/libc/manual/html_node/Argp-Examples.html#Argp-Examples
 */

const char *argp_program_version = "file-finder 1.0 (for practicing files and directories in linux)";
const char *argp_program_bug_address = "conght4@viettel.com.vn";

/* Program documentation. */
static char doc[] =
  "just file finder as the name";
/* A description of the arguments we accept. */
static char args_doc[] = "FILE_NAME";

/* The options we understand. */
static struct argp_option options[] = {
  {"dir",'d', "DIRECTORY",      0,  "The root directory where looking for files" },
  {0,    'c', 0,      0,  "Enable case sensitive" },
  {0,    'h', 0,      0,  "Ignore hidden files (speed performance)" },
  {0,    'a', 0,      0,  "Match the whole name" },
  {0}
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
  char *searchFileName;
  int mCase,mWhole,ignoreHidden; 
  char *rootDir;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

	switch (key)
	{
		case 'c':
			arguments->mCase = 1;
			break;
		case 'a':
			arguments->mWhole = 1;
			break;
		case 'h':
			arguments->ignoreHidden = 1;
			break;
		case 'd':
			arguments->rootDir = arg;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num == 0)	// only take first arg, others ignored
				arguments->searchFileName = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				// not enough arg 
				argp_usage (state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


// return 1 if of type directory, 0 if regular files and -1 otherwise 
int checkFileType(char const* filePath){
	struct stat fs;
	memset(&fs, 0, sizeof(fs));

	if(stat(filePath, &fs) != 0){	// io errors
		return -1;
	}
	if ((fs.st_mode&S_IFMT) == S_IFDIR){
		return 1;
	}
	if ((fs.st_mode&S_IFMT) == S_IFREG){
		return 0;
	}
	return -1;
}
static char* buf = NULL;
static int bufSize = 64;	// initial 10 levels

void toLowerStr(char* s){
	for(int i=0; i<strlen(s); ++i){
		s[i] = tolower(s[i]);
	}
}
int traverseAt(struct arguments* args, int isRoot){
	DIR* dirp = NULL;
	struct dirent* entry = NULL;
	char nameBuf[NAME_MAX];

	if ((dirp = opendir(buf)) == NULL){
		if (!isRoot){	// show errors on root
			return 1;
		}
		if (errno == ENOTDIR)
			printf("ERROR: '%s' is not a directory\n", buf);
		else if (errno == ENOENT)
			printf("ERROR: '%s' no such file or directory\n", buf);
		else
			perror(buf);
		return 1;
	}
	int dirLen = strlen(buf);
	while((entry=readdir(dirp)) != NULL){	// null indicates error or no more files
		char const* name = entry->d_name;

		buf[dirLen] = '\x00';	// preserve root
		
		// general filtering
		if (strlen(name) == 1 && name[0] == '.')	// special files
			continue;
		if (strlen(name) == 2 && name[0] == '.' && name[1] == '.')
			continue;
		if (args->ignoreHidden && name[0] == '.'){	// hidden files
			continue;
		}

		// expand
		if ((strlen(buf) + strlen(name) + 1) > bufSize){
			bufSize = bufSize*4;	// double
			buf = realloc(buf, bufSize);
		}
		if (strlen(name) > 0 && buf[strlen(buf) - 1] != '/'){
			strcat(buf, "/");
			strcat(buf, name);
		}
		else 
			strcat(buf, name);


		if (checkFileType(buf) == 1){	// if dir, traverse into
			traverseAt(args, 0);
			continue;
		}
		if (checkFileType(buf) == 0){
			// logic filtering
			strcpy(nameBuf, name);
			if (!args->mCase){
				toLowerStr(args->searchFileName);
				toLowerStr(nameBuf);
			}
			if (args->mWhole){
				if (strcmp(nameBuf, args->searchFileName) != 0)
					continue;		
			} else if(strstr(nameBuf, args->searchFileName) == NULL)
				continue;
			printf("%s\n", buf);
		}	
	}
	closedir(dirp);

}
int main (int argc, char **argv)
{
	// parsing args (defaults: no case-sensitive, match partly
	struct arguments args = {};
	argp_parse (&argp, argc, argv, 0, 0, &args);

	if (args.rootDir == NULL){
		printf("ERROR: You must specify the root directory (-d option)\n");
		return 1;
	}
	buf = (char*)malloc(bufSize);
	if (buf == NULL){
		printf("ERROR: errors on malloc (errno: %d)\n", errno);
		return 1;
	}
	strcpy(buf, args.rootDir);	// null-terminated guarantee
	int ret = traverseAt(&args, 1);
	free(buf);
	return ret;
}
