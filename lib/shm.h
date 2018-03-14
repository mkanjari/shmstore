/*
 * This file contains all the DS definitions for shared memory key, value store.
 * the key, value store is implemented as a shared memory hash table.
 */

#ifndef _SHM_H
#define _SHM_H

#include <stdint.h>
#include <pthread.h>

/* various sizes assumed in the program */
#define MAX_DATA_SZ			256
#define NUM_HASH_BUCKETS		256  /* def number of indices in hash table */
#define NUM_HASH_DATA_ENTRIES		4096 /* 4k entries at max for now */

/* private data structires for data manuplation in hash table */
/* meta data for phash block */
typedef struct phash_header {
	pthread_mutex_t mutex; //TODO
}phash_header;

/* phash entry */
typedef struct phash_entry {
	int location; /* head of ll */
}phash_entry;

/* hash block */
typedef struct phash_t {

	phash_header	header;
	phash_entry	bucket[NUM_HASH_BUCKETS];
}phash_t;

/* metadata related to data block */
typedef struct pdata_header{
	int loc; /* location to write the new data */
	int ffl;  /* first free data entry - freed during delete */
}pdata_header;

/* data entry block */
typedef struct pdata_entry {
	int		next;      /* location of the next data in the ll if it
				      allocated, or location of the next free if
				      it is not allocated */
	uint64_t	key;	   /* key of the data stored here */
	char		data[MAX_DATA_SZ]; /* user data stored at this location */
}pdata_entry;

/* pdata block */
typedef struct pdata_t {

	pdata_header	header;
	pdata_entry	entry[NUM_HASH_DATA_ENTRIES];
}pdata_t;

/* PUBLIC APIs and DS */

/*
 * Shared memory hash table
 * hash_name: is used in shm_open to get the shm_fd
 * phash: shared memory block created using mmap
 * pdata: shared memory block created using mmap
 * size: size of the hash table
 */
typedef struct shm_hash_t_ {

	/* unique name for the shm hash table */
	char	hash_name[256];

	/* shared memory space for hash keys */
	phash_t	*phash;
	int	hash_fd;

	/* shared memory space for hash data */
	pdata_t	*pdata;
	int	data_fd;

	/* size of the hash table - number of buckets */
	int	size;
}shm_hash_t;

/* APIs */

/* inline utilities */
static inline int is_shm_hash_valid(shm_hash_t *shm_hash)
{
	if (!shm_hash || !shm_hash->pdata || !shm_hash->phash)
		return 0;
	return 1;
}

/*
 * shm_hash_init
 * @desc: initialise the shared memory  hash table
 * name: unique name to map to the shared memory region
 * @return: returns a shared memory hash table
 */
extern shm_hash_t *shm_hash_init(char *name);

/*
 * shm_hash_insert
 * @desc: insert the key, value pair to the hash table
 * key: key to be inserted in the hash table
 * data: data associated with the key
 * @return: 0 - success -1: error
 */
extern int shm_hash_insert(shm_hash_t *shm_hash, uint64_t key, char *data);

/*
 * shm_hash_lookup
 * @desc: lookup a key in the shared hash table
 * key: uint64_t key
 * data: char ** data - user needs to free the memory
 * @return: location of the entry, -1 if not found
 */
extern int shm_hash_lookup(shm_hash_t *shm_hash, uint64_t key, char **data);

/*
 * shm_hash_delete
 * @desc: delete the key, value pair from the hash table
 * key: uint64_t key
 * return: 0 - success -1: error
 */
extern int shm_hash_delete(shm_hash_t *shm_hash, uint64_t key);

/*
 * shm_hash_print
 * @desc: print the key, value pair
 */
extern void shm_hash_print(shm_hash_t *shm_hash);

/*
 * shm_hash_clean
 * @desc: unmap the shared memory locations and clean the hash table
 * shm_hash: hash table
 * unlink: if set the process will unlink the shared memory via shm_unlink
 */
extern int shm_hash_clean(shm_hash_t *shm_hash, int unlink);

#endif /* _SHM_H */

