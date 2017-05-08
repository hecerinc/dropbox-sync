#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "readBytes.h"

void * read_file(FILE * stream) {
	unsigned char * buffer = NULL;

	size_t capacity = INIT_BUFFER_SIZE;
	size_t size = 0; // number of bytes actually in buffer

	// Initialise buffer
	buffer = malloc(capacity);

	if(buffer == NULL) {
		fprintf(stderr, "Could not allocate memory\n");
		return NULL;
		// exit(1);
	}

	int c;
	while((c = fgetc(stream)) != EOF ) { // reading one byte at a time
		// Does it fit?
		if(size + 1 > capacity) {
			if(capacity <= (SIZE_MAX / 2))
				capacity *= 2;
			else if(capacity < SIZE_MAX) {
				capacity = SIZE_MAX;
			}
			else {
				free(buffer);
				return NULL;
			}

			void * tmp = realloc(buffer, capacity);
			if(tmp == NULL) {
				free(buffer);
				return NULL;
			}
			buffer = tmp;
		}
		buffer[size++] = c;
	}

	if(c == EOF) {
		if(size == 0)
			return NULL;
		unsigned char * s = realloc(buffer, size);
		// hit end of file
		DATA * alpha = malloc(sizeof(DATA));
		if(s == NULL || alpha == NULL) {
			free(buffer);
			return NULL;
		}
		alpha->data = s;
		alpha->size = size;
		return alpha;
	}
	else {
		// some other error interrupted read
		free(buffer);
		fprintf(stderr, "An error occurred reading the file\n");
		return NULL;
	}
	return NULL;
}