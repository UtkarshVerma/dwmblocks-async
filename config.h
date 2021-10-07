#define CMDLENGTH 50
#define DELIMITER "<"

typedef struct {
	char* command;
	unsigned int interval;
	unsigned int signal;
	char output[CMDLENGTH];
	int pipe[2];
} Block;

static Block blocks[] = {
	{"sb-mail", 1800, 17},
	{"sb-music", 0, 18},
	{"sb-disk", 1800, 19},
	{"sb-memory", 10, 20},
	{"sb-loadavg", 10, 21},
	{"sb-volume", 0, 22},
	{"sb-battery", 5, 23},
	{"sb-date", 20, 24},
	{"sb-network", 5, 25}
};
