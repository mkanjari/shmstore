#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "lib/file_parser.h"
#include "lib/shm.h"

#define MAX_CMD_ARGS	2
#define MAX_ERR_SZ	256

/* unique URL to create the shared memory store */
char *hash_name = "shmstore";

/* parse_and_validate_argv */
static int parse_and_validate_argv(int argc, char **argv,
				   int *op, char *fname)
{
	/* check if the number of arg are valid */
	if (argc != 3) {
		printf("the program expects 2 cmd line parameters\n");
		return 0;
	}

	/* first argument is the file name */
	if (strlen(argv[1]) > MAX_FILE_NAME_SZ) {
		printf("file name cannot be more than 256 characters\n");
		return 0;
	}
	strncpy(fname, argv[1], MAX_FILE_NAME_SZ);

	/* TODO: validate the length of the file - right now I am assuming 1000
	 * records at max */

	/* second argument is the operation */
	*op = atoi(argv[2]);
}

int main(int argc, char *argv[])
{
	int		op = 0;
	char		fname[MAX_FILE_NAME_SZ];
	raw_data_t	*rd = NULL;
	char		err[MAX_ERR_SZ];
	shm_hash_t	*shm_hash = NULL;

	if (!parse_and_validate_argv(argc, argv, &op, fname))
		return -1;

	rd = raw_data_alloc(MAX_FILE_SZ);
	if (!rd) {
		printf("Failed to allocate raw data\n");
		return -1;
	}

	if (parse_input_file(fname, rd, err, sizeof(err)) != 0) {
		printf("file parse error : %s", err);
		return -1;
	}

	shm_hash = shm_hash_init(hash_name);
	if (!shm_hash) {
		printf("Failed to create a shared memory hash table\n");
	}

	switch(op) {
	case 1: {
		int	i = 0;

		for (i = 0 ; i < rd->size; i++) {
			shm_hash_insert(shm_hash, rd->entries[i].key,
					rd->entries[i].data);
		}
		break;
	}
	case 2: {
		int	i = 0;

		for (i = 0 ; i < rd->size; i++) {
			shm_hash_delete(shm_hash, rd->entries[i].key);
		}
		break;
	}
	case 3: {
		int	i = 0;

		for (i = 0 ; i < rd->size; i++) {
			char		*data = NULL;
			int		ret = 0;
			ret = shm_hash_lookup(shm_hash, rd->entries[i].key,
					      &data);
			if (ret == -1)
				printf("Key %lu NOT FOUND\n",
				       rd->entries[i].key);
			else {
				printf("key: %lu data: %s\n",
				       rd->entries[i].key, data);
				free(data);
			}
		}
		break;
	}
	default:
		shm_hash_print(shm_hash);
		break;
	}

	shm_hash_clean(shm_hash, 0);
	raw_data_free(rd);
	return 0;
}
