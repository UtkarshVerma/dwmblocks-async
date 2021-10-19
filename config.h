#define CMDLENGTH 50
#define DELIMITER "<"
#define LEADING_DELIMITER
#define CLICKABLE_BLOCKS

const Block blocks[] = {
	BLOCK("sb-mail",    1800, 17)
	BLOCK("sb-music",   0,    18)
	BLOCK("sb-disk",    1800, 19)
	BLOCK("sb-memory",  1800, 20)
	BLOCK("sb-loadavg", 1800, 21)
	BLOCK("sb-volume",  0,    22)
	BLOCK("sb-battery", 5,    23)
	BLOCK("sb-date",    1,    24)
	BLOCK("sb-network", 5,    25)
};
