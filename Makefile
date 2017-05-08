

all: server client

client: m7.c readBytes.h readBytes.c
	gcc m7.c readBytes.c -o m7 -W -Wall -Werror
server: m7server.c readBytes.c
	gcc m7server.c readBytes.c -o server -W -Wall -Werror
