#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define INIT_BUFFER_SIZE 320000 // size of an average wordfile (in bytes)

typedef enum action_t {WRITE = 2, RENAME, DELETE} action_type;

typedef struct {
	unsigned char * data;
	size_t size;
} DATA;

void * read_file(FILE *);

struct action_s {
	action_type type;
	unsigned _lengthInBytes;
	size_t nameLength;
	char payload[1];
	// size_t payloadSize;
	// payload format: 
	// FIRST<ASCII30>SECOND
	// Ascii 30 is a record separator
	// char * payload;
	// char * filename;
	// Payloads: 
		// Rename: 
			// old name (payload)
			// new name (filenmae)
		// Write:
			// data (payload)
			// filename
		// DELETE
			// filename
			// data = NULL
} __attribute__((packed));

typedef struct action_s Action;
