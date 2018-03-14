/*
 * This file provides the APIs to parse the contents of the input file.
 * Expected File format:
 *	1. Each new line represents a <key>,<value> pair
 *	2. data is stored as <key> <string>
 *	3. <key> and <str> are seperated with a space
 *      4. Max string size is 256
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "file_parser.h"

/* Public functions */

/* allocate raw data */
raw_data_t *raw_data_alloc(int n)
{
	raw_data_t	*rd = NULL;

	rd = malloc(sizeof(raw_data_t));
	if (!rd)
		return NULL;
	memset(rd, 0, sizeof(raw_data_t));

	rd->entries = malloc(n * sizeof(rd_entry_t));
	if (!rd->entries) {
		free(rd);
		return NULL;
	}
	memset(rd->entries, 0 , n * sizeof(rd_entry_t));

	return rd;
}

/* free raw data */
void raw_data_free(raw_data_t *rd)
{
	free(rd->entries);
	rd->entries = NULL;
	free(rd);
	rd = NULL;
}

/* parse input file */
int parse_input_file(char *fname,
		     raw_data_t *rd,
		     char *err,
		     int err_sz)
{
	FILE		*stream;
	char		*line = NULL;
	size_t		len = 0;
	ssize_t		nread = 0;
	int		count = 0;

	if (!rd) {
		strncpy(err, "could not open file\n", err_sz);
		return -1;
	}

	stream = fopen(fname, "r");
	if (stream == NULL) {
		strncpy(err, "could not open file\n", err_sz);
		return -1;
	}

	while ((nread = getline(&line, &len, stream)) != -1) {

		char		*key = NULL;
		char		*data = NULL;

		/* get line gets the newline character as well - remove it here */
		line[nread - 1] = '\0';

		/* parse the key */
		key = strtok(line, " ");
		rd->entries[count].key = strtoull(key, NULL, 10);

		/* parse the value */
		data = strtok(NULL, " ");
		strncpy(rd->entries[count].data, data, MAX_STR_SZ);

		rd->size++;
		count++;
	}
	free(line);
	fclose(stream);
	return 0;
}

/* print_raw_data */
void print_raw_data(raw_data_t *rd, int n)
{
	int			i = 0;

	if (!rd)
		return;

	for (i = 0; i < rd->size; i++) {
		printf("key: %" PRIu64 " value: %s\n",
		       rd->entries[i].key,
		       rd->entries[i].data);
	}
}
