#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define INIT_BUFFER_SIZE 320000 // size of an average wordfile (in bytes)

typedef struct {
	unsigned char * data;
	size_t size;
} DATA;

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
	int i = 0;
	while((c = fgetc(stream)) != EOF ) { // reading one byte at a time
		printf("Iteration %d\n", i++);
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


int main() {
	printf("Reading file\n");
	FILE * in = fopen("infile.docx", "rb");
	DATA * beta = (DATA *)read_file(in);
	if(beta == NULL) {
		fclose(in);
		fprintf(stderr, "Could not read file\n");
		exit(1);
	}
	fclose(in);
	printf("Writing file\n");
	FILE * out = fopen("outfile.docx", "wb");
	fwrite(beta->data, beta->size, 1, out);
	fclose(out);
	for(size_t i = 0; i < beta->size; i++) {
		printf("%c ", beta->data[i]);
	}
	free(beta->data);
	free(beta);
	printf("Succeed\n");
	// fread()
}
