#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include "readBytes.h"
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))


in_addr_t serverIP;

Action * createAction(const char* data, const unsigned dataSize) {
	unsigned actionSize = sizeof(Action) + dataSize - 1; // -1 porque el payload tiene 1 caracter y no lo necesitamos considerar
	Action * action = (Action *)malloc(actionSize);
	action->_lengthInBytes = actionSize;
	memcpy(action->payload, data, dataSize);
	return action;
}

int sendPayload(Action * action);
void sendFileToServer(char * filename);
void deleteFileFromServer(const char * filename);
void renameFileOnServer(const char * oldname, const char * newname); 
void handler(struct inotify_event *i);

char * renameFileOrig;
char * chosenDir;

int main(int argc, char *argv[]) {
	char buf[BUF_LEN] __attribute__ ((aligned(__alignof__(struct inotify_event))));

	ssize_t numRead;


	struct inotify_event *event;

	if (argc < 3 || strcmp(argv[1], "--help") == 0) {
		fprintf(stderr,"%s <watch directory> <serverIP>\n", argv[0]);
		exit(1);
	}

	renameFileOrig = NULL;

	// Save server IP address
	serverIP = inet_addr(argv[2]);

	int inotifyFd = inotify_init();
	if (inotifyFd == -1) {
		fprintf(stderr, "Could not create inotify instance\n");
		exit(1);
	}


	int wd = inotify_add_watch(inotifyFd, argv[1], IN_ALL_EVENTS);
	if (wd == -1)
		exit(1);

	printf("Watching %s using wd %d\n", argv[1], wd);
	chosenDir = malloc(strlen(argv[1]) + 1);
	strcpy(chosenDir, argv[1]);


	for (;;) {                                  /* Read events forever */
		numRead = read(inotifyFd, buf, BUF_LEN);

		if (numRead <= 0)
			exit(1);


		/* Process all of the events in buffer returned by read() */

		for (char * p = buf; p < buf + numRead; ) {
			event = (struct inotify_event *) p;
			handler(event);

			p += sizeof(struct inotify_event) + event->len;
		}
	}
	free(chosenDir);
	exit(EXIT_SUCCESS);
}


void handler(struct inotify_event *i) {
	// solo me interesa IN_CLOSE_WRITE cuando se modifica o crea un archivo
	if(i->len <= 0) // No nos interesan los directories
		return;
	if(!(i->mask & IN_DELETE || i->mask & IN_MOVED_FROM || i->mask & IN_MOVED_TO || i->mask & IN_CLOSE_WRITE))
		return;
	// printf("    wd =%2d; ", i->wd);
	// if (i->cookie > 0)
	// 	printf("cookie =%4d; ", i->cookie);

	// printf("mask = ");
	// if (i->mask & IN_ACCESS)        printf("IN_ACCESS ");
	// if (i->mask & IN_ATTRIB)        printf("IN_ATTRIB ");
	// if (i->mask & IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");

	char * filename = i->name;

	if (i->mask & IN_CLOSE_WRITE) {

		printf("IN_CLOSE_WRITE\n");

		// A file was either created or modified
		// Replace the file on the server (send new contents)

		// Get the name
		sendFileToServer(filename);
	}
	// if (i->mask & IN_CREATE)        printf("IN_CREATE ");
	if (i->mask & IN_DELETE) {
		printf("IN_DELETE\n");
		// Delete file (send deleted filename)
		deleteFileFromServer(filename);
	}
	// if (i->mask & IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
	// if (i->mask & IN_IGNORED)       printf("IN_IGNORED ");
	// if (i->mask & IN_ISDIR)         printf("IN_ISDIR ");
	// if (i->mask & IN_MODIFY)        printf("IN_MODIFY ");
	// if (i->mask & IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
	if (i->mask & IN_MOVED_FROM){ 
		printf("IN_MOVED_FROM\n");
		// A file was renamed
		// Contains the old filename when a file is renamed

		renameFileOrig = malloc(strlen(filename)+1);
		if(renameFileOrig == NULL) {
			fprintf(stderr, "Could not save filename for rename\n");
			exit(1);
		}
		strcpy(renameFileOrig, filename);
	}
	if (i->mask & IN_MOVED_TO) { 
		printf("IN_MOVED_TO\n");
		// A file was either renamed or moved to the folder
		// Find out which
		if(renameFileOrig == NULL) {
			// No old filename, a new file was moved to the folder
			sendFileToServer(filename);
		}
		else {
			// A file was renamed
			renameFileOnServer(renameFileOrig, filename); // oldname, newname
			free(renameFileOrig);
			renameFileOrig = NULL;
		}
	}
	// if (i->mask & IN_OPEN)          printf("IN_OPEN ");
	// if (i->mask & IN_Q_OVERFLOW)    printf("IN_Q_OVERFLOW ");
	// if (i->mask & IN_UNMOUNT)       printf("IN_UNMOUNT ");
	// printf("\n");

	// if (i->len > 0)
	// 	printf("        name = %s\n", i->name);
}

void sendFileToServer(char * filename) {
	printf("File was modified or created\n");
	printf("Sending file to server: %s\n", filename);
	// Read the file to memory
	printf("FILENAME: %s\n", filename);
	// Build full filename
	char * fullpath = NULL;
	fullpath = (char*) malloc((strlen(filename) + strlen(chosenDir) + 1)*sizeof(char));
	if(fullpath == NULL) {
		fprintf(stderr, "Could not allocate memory for fullpath\n");
		return;
	}
	strcpy(fullpath, chosenDir);
	strcat(fullpath, filename);
	printf("Checking fullpath:\n");
	char *p = fullpath;
	while(*p != '\0'){
		printf("%c", *p);
		p++;
	}
	printf("\nCheck passed\n");
	printf("FULLPATH: %s\n", fullpath);
	printf("FULLPATH: %s\n", fullpath);
	printf("FULLPATH: %s\n", fullpath);
	printf("FULLPATH: %s\n", fullpath);
	FILE * file = fopen(fullpath, "rb");
	if(file == NULL) {
		fprintf(stderr, "Could not open file %s\n", fullpath);
		return;
	}
	free(fullpath);

	DATA * fileData = (DATA *)read_file(file);
	if(fileData == NULL) {
		fprintf(stderr, "Could not read file data.\n");
		fclose(file);
		exit(1);
	}

	fclose(file);
	fullpath = NULL;
	file = NULL;

	char * finalString;

	size_t dataSize = fileData->size + strlen(filename) + 2; // 1 para el record separator
	printf("Size of payload in bytes: %lu\n", fileData->size);

	finalString = malloc(dataSize);

	memcpy(finalString, fileData->data, fileData->size);
	finalString[fileData->size] = 30;
	memcpy(&finalString[fileData->size + 1], filename, strlen(filename));
	finalString[dataSize-1] = '\0';

	Action * action = createAction(finalString, dataSize);
	action->nameLength = strlen(filename);
	action->payloadSize = fileData->size;
	action->type = WRITE;
	
	// send the action!
	if(sendPayload(action)){
		printf("Successfully sent the action!\n");
	}
	else {
		fprintf(stderr, "Failed to send action\n");
	}

	free(fileData->data);
	free(fileData);
	free(finalString);
	free(action);
	fileData = NULL;
	finalString = NULL;
	action = NULL;
}
void deleteFileFromServer(const char * filename) {
	printf("Deleting file from server: %s\n", filename);
	Action * action = createAction(filename, strlen(filename)+1);
	action->type = DELETE;
	action->nameLength = strlen(filename);
	sendPayload(action);
	free(action);
}
void renameFileOnServer(const char * oldname, const char * newname) {
	printf("Renaming file on server: %s -> %s\n", oldname, newname);

	size_t lenold = strlen(oldname);
	size_t length = lenold + strlen(newname) + 1;

	char * finalString = malloc(length); // record separator
	strcpy(finalString, oldname);
	finalString[lenold] = 30;
	strcpy(&finalString[lenold+1], newname);

	Action * action = createAction(finalString, length);
	action->type = RENAME;
	action->nameLength = strlen(newname);

	sendPayload(action);

	free(action);
	free(finalString);
} 

// #define RESPONSEBUFFER 10

int sendPayload(Action * action) {
	// Open a socket to the server
	// Send the file
	// Close the socket
	printf("Sending payload!\n");
	int sockId = socket(PF_INET, SOCK_STREAM, 0);
	if(sockId == -1) { // check if successfull
		fprintf(stderr, "Failed to open socket\n");
		return -1;
	}
	else {
		printf("Opened stream\n");
	}

	// Create network address data structure
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(3030); // TODO: change this to not fixed port number
	serverAddress.sin_addr.s_addr = serverIP;


	// Connect 
	int status = connect(sockId, (struct sockaddr *) &serverAddress, sizeof(serverAddress)); 
	if(status == 0) {
		printf("Successfully connected to server\n");
	}
	else {
		printf("Could not successfully connect\n");
		close(sockId);
		return -1;
	}


	// char response[RESPONSEBUFFER]; 
	// int msg_size;
	int msg_size = htonl(action->_lengthInBytes);

	// send size first
	send(sockId, &msg_size, sizeof(msg_size), 0);
	if(send(sockId, action, action->_lengthInBytes, 0) != action->_lengthInBytes) {
		printf("send() failed\n");
		fprintf(stderr, "send() failed\n");
		close(sockId);
		return -1;
	}

	// Reply 
	// Do we want to receive a reply? This is a blocking operation
	// msg_size = recv(sockId, response, RESPONSEBUFFER, 0);

	// if(msg_size < 0) {
	// 	printf("Receive failed()\n");
	// 	exit(1);
	// }
	// else {
	// 	printf("Response received: %s\n", response);
	// 	memset(&response[0], 0, sizeof(response));
	// }

	close(sockId); // Unix
	return 10;
}