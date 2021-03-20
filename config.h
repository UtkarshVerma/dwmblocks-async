#define CMDLENGTH 50
#define delimiter " "

static const Block blocks[] = {
    {"sb-mail", 1800, 1}, {"sb-music", 0, 2},	 {"sb-disk", 1800, 4},
    {"sb-memory", 10, 3}, {"sb-loadavg", 10, 9}, {"sb-volume", 0, 5},
    {"sb-battery", 5, 6}, {"sb-date", 60, 7},	 {"sb-network", 5, 8},
};

