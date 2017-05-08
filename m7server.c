#include <stdio.h>
#include <string.h>
#include <unistd.h>
// #include <winsock2.h>
// #include <ws2tcpip.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "readBytes.h"
#include <errno.h>

Action * handler(int);
void saveToFile(Action *);
void deleteFile(Action *);
void renameFile(Action *);

char * destination; // Destination directory

int main(int argc, char * argv[]) {


	// Initalise on Windows
	// WSADATA wsaData;
	// int result;
	// result = WSAStartup(MAKEWORD(2,2), &wsaData);
	// if (result != 0) {
	// 	printf("WSAStartup failed: %d\n", result);
	// 	exit(1);
	// }
	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		fprintf(stderr,"%s <destination directory>\n", argv[0]);
		exit(1);
	}

	int dirlen = strlen(argv[1]);
	int isProperlyEnded = 1;
	if(argv[1][dirlen-1] != '/'){
		dirlen += 2;
		isProperlyEnded = 0;
	}
	else
		dirlen += 1;

	destination = malloc(dirlen);
	strcpy(destination, argv[1]);
	if(!isProperlyEnded)
		strcat(destination, "/");



	// Opens the stream
	int socketId = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(socketId == -1) { // check if successfull
		printf("Failed\n");
		exit(1);
	}
	else {
		printf("Opened stream\n");
	}

	// Create a new address data structure
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(3030);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	// Assign an address to socket
	int status = bind(socketId, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if(status == -1) {
		printf("Failed to bind address\n");
		exit(1);
	}

	// Binding is skipped:
	// - destination is determined during connection setup
	// - we don't need to know which port we're sending from

	// listen(sockId, queueLimit)
	// queueLimit: # of active participants that can listen
	status = listen(socketId, 10);
	if(status == 0) {
		printf("Listening on port 3030\n");
	}
	else {
		printf("Could not open connection\n");
		exit(1);
	}

	struct sockaddr_in clientAddr;
	unsigned int clntLen;
	int clntSock;
	while(1) {
		clntLen = sizeof(clientAddr);
		// Establish a connection
		if((clntSock = accept(socketId, (struct sockaddr *) &clientAddr, &clntLen)) < 0) {
			printf("Could not accept connection\n");
			exit(1);
		}

		printf("Handling client %s\n", inet_ntoa(clientAddr.sin_addr));
		Action * a = handler(clntSock);
		printf("Client %s disconnected\n", inet_ntoa(clientAddr.sin_addr));

		printf("\nRECEIVED ACTION \n");
		printf("==================================\n");
		printf("Action type: %d\n", a->type);
		printf("Filename length: %lu\n", a->nameLength);
		printf("\n");
		
		switch(a->type) {
			case WRITE:
				saveToFile(a);
				break;
			case DELETE:
				deleteFile(a);
				break;
			case RENAME:
				renameFile(a);
				break;
			default: 
				fprintf(stderr, "Unsupported action type\n");
				break;
		}

		// Save the action

		free(a);
	}


	// Closes the streams AND frees the port
	status = close(socketId); // Unix
	// status = closesocket(socketId); // Windows
	if(status == 0) {
		printf("Closed stream\n");
	}
	free(destination);
}

#define RECEIVE_BUFFER_SIZE 55

Action * handler(int clientSocket) {
	Action * buffer = NULL;
	int msg_size;
	int payload_size = 0;
	// Receive size first
	if((msg_size = recv(clientSocket, &payload_size, sizeof(payload_size), 0)) < 0) {
		printf("Error receving message\n");
		exit(1);
	}
	payload_size = ntohl(payload_size);
	buffer = malloc(payload_size);

	// Receive message
	if((msg_size = recv(clientSocket, buffer, payload_size, 0)) < 0) {
		printf("Error receving message\n");
		exit(1);
	}
	// while(msg_size > 0) { // 0 indicates end of transmission
	// 	if(send(clientSocket, buffer, msg_size, 0) != msg_size) {
	// 		printf("send() failed\n");
	// 		exit(1);
	// 	}
	// 	if((msg_size = recv(clientSocket, buffer, RECEIVE_BUFFER_SIZE, 0)) < 0) {
	// 		printf("Receive failed()\n");
	// 		exit(1);
	// 	}
	// }
	close(clientSocket); // Unix
	// closesocket(clientSocket); // Windows
	return buffer;
}


void saveToFile(Action * action) {

	size_t length = action->nameLength;
	char * filename = malloc(length + 1);
	char * p = action->payload;

	size_t pos = 0;

	// Look for record separator
	while(*p != 30) {
		p++;
		pos++;
	}
	// Payload in 0 - (pos -1)
	p++;
	strncpy(filename, p, length);
	filename[length] = '\0';


	printf("Filename: %s\n", filename);

	char * fullpath = (char *)malloc((strlen(destination) + length + 1)*sizeof(char));
	strcpy(fullpath, destination);
	strcat(fullpath, filename);

	FILE * outfile = fopen(fullpath, "wb");


	// Write it out!
	if(outfile == NULL) {
		fprintf(stderr, "Could not save received file to %s\n", fullpath);
	}
	else {
		printf("Writing file\n");
		fwrite(action->payload, pos, 1, outfile);
		printf("Successfully wrote file\n");
	}
	fclose(outfile);

	free(fullpath);
	free(filename);
	outfile = NULL;
	filename = NULL;
	fullpath = NULL;


}

void deleteFile(Action * action) { 
	// Get filename
	size_t length = action->nameLength;
	size_t destinationLength = strlen(destination);

	char * filename = malloc(destinationLength + length + 1);
	strcpy(filename, destination);
	strcpy(&filename[destinationLength], action->payload);

	if(unlink(filename) == 0) {
		printf("Successfully deleted %s\n", filename);
	}
	else {
		fprintf(stderr, "Could not delete %s\n", filename);
	}
	free(filename);
}
void renameFile(Action * action) {
	size_t lengthOfNewName = action->nameLength;
	size_t lengthOfDestination = strlen(destination);

	char *p = action->payload;
	char *newname = malloc(lengthOfDestination + lengthOfNewName + 1);
	int pos = 0;
	while(*p != 30) {
		p++;
		pos++;
	}
	char * oldname = malloc(lengthOfDestination + pos + 1);
	strcpy(oldname, destination);
	memcpy(&oldname[lengthOfDestination], action->payload, pos);
	oldname[lengthOfDestination + pos] = '\0';
	printf("Old name: %s\n", oldname);

	strcpy(newname, destination);
	p++;
	strncpy(&newname[lengthOfDestination], p, lengthOfNewName);
	newname[lengthOfDestination + lengthOfNewName] = '\0';
	printf("New name: %s\n", newname);

	if(rename(oldname, newname) != -1)
		printf("Successfully renamed: %s -> %s\n", oldname, newname);
	else
		fprintf(stderr, "Could not rename %s\n", oldname);

	free(oldname);
	free(newname);

}

