/*
 * This file contains the routines for manuplating shared memory hash table.
 * We will be keeping two shared memory location to implement the shm_hash.
 * phash: will contain a chunk of memory which will store all the hash indices.
 * pdata: will contain a chunk of memory which store all the data
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "shm.h"

/* private function */

/* get hash bucket */
//TODO: search an optimized hash function for 64 bit
int get_bucket(uint64_t key)
{
	return key % (NUM_HASH_BUCKETS);
}

shm_hash_t *shm_hash_init_internal(char *name)
{
	shm_hash_t	*shm_hash;    /* shared mem hash_table */
	char		*phash_url;   /* unique url to access the phash */
	char		*pdata_url;   /* unique url to access pdata */
	uint32_t	phash_url_sz; /* phash url = 'namehash' */
	uint32_t	pdata_url_sz; /* pdata url = 'namedata' */
	int		init = 0;     /* check if the memory was initialized
					 first time*/

	/* allocate the hash table */
	shm_hash = malloc(sizeof(shm_hash_t));
	if (!shm_hash)
		return NULL;
	strncpy(shm_hash->hash_name, name, sizeof(shm_hash->hash_name));

	/* create the shared memory unique url for phash. Other processes which
	 * wish to access the same shared memory need to use the same hash table
	 * name */
	phash_url_sz = strlen(name) + strlen("hash") + 1;
	phash_url = malloc(phash_url_sz);
	if (!phash_url) {
		printf("Could not allocate hash_name\n");
		return NULL;
	}
	snprintf(phash_url, phash_url_sz, "%s%s", name, "hash");

	/* create the shared memory segment for hash buckets */
	shm_hash->hash_fd = shm_open(phash_url, O_RDWR, 0666);
	if (shm_hash->hash_fd == -1) {
		shm_hash->hash_fd = shm_open(phash_url, O_CREAT | O_RDWR, 0666);
		if (shm_hash->hash_fd == -1) {
			printf("Failed to create shared memory !\n");
			return NULL;
		}
		ftruncate(shm_hash->hash_fd, sizeof(phash_t));
		init = 1;
	}

	/* now map the shared memory for hash keys in address space of the
	 * process */
	shm_hash->phash = mmap(0, sizeof(phash_t),
			       PROT_READ | PROT_WRITE, MAP_SHARED,
			       shm_hash->hash_fd, 0);
	if (shm_hash->phash == MAP_FAILED) {
		printf("Failed to create shared memory for hash buckets\n");
		return NULL;
	}

	/* create the shared memory unique name for pdata. Other processes which
	 * wish to access the same shates memeory need to use the same hash
	 * table name */
	pdata_url_sz = strlen(name) + strlen("data") + 1;
	pdata_url = malloc(pdata_url_sz);
	if (!pdata_url) {
		printf("Could not allocate data_name\n");
		return NULL;
	}
	snprintf(pdata_url, pdata_url_sz, "%s%s", name, "data");

	/* create the shared memory segment for the data region */
	shm_hash->data_fd = shm_open(pdata_url, O_RDWR, 0666);
	if (shm_hash->data_fd == -1) {
		shm_hash->data_fd = shm_open(pdata_url, O_CREAT | O_RDWR, 0666);
		if (shm_hash->data_fd == -1) {
			printf("Failed to allocate shared memory for data block!\n");
			return NULL;
		}
		ftruncate(shm_hash->data_fd, sizeof(pdata_t));
	}

	/* now map the shared memory for hash keys in address space of the
	 * process */
	shm_hash->pdata = mmap(0, sizeof(pdata_t), PROT_READ | PROT_WRITE, MAP_SHARED,
			       shm_hash->data_fd, 0);
	if (shm_hash->pdata == MAP_FAILED) {
		printf("Failed to create shared memory for hash data\n");
		return NULL;
	}

	/* initialise shared memory if needed */
	if (init) {
		memset(shm_hash->phash, -1, sizeof(phash_t));
		memset(shm_hash->pdata, -1, sizeof(pdata_t));

		/* first free location = 0 */
		shm_hash->pdata->header.loc = 0;

		/* free entry list head  = -1 */
		shm_hash->pdata->header.ffl = -1;
	}

	/* for now we will hard code this to NUM_HASH_BUCKETS */
	shm_hash->size = NUM_HASH_BUCKETS;

	free(phash_url);
	free(pdata_url);
	return shm_hash;
}

int shm_hash_insert_at_ff(shm_hash_t *shm_hash, uint64_t key, char *data)
{
	int		bucket = -1;
	int		location = -1;
	int		bhead = -1;

	/* new entry will be added here */
	location = shm_hash->pdata->header.ffl;

	/* update the ffl to the next of the current entry at ffl */
	shm_hash->pdata->header.ffl = shm_hash->pdata->entry[location].next;

	/* form the new entry to be inserted at location */
	shm_hash->pdata->entry[location].next = -1;
	shm_hash->pdata->entry[location].key = key;
	strncpy(shm_hash->pdata->entry[location].data, data, MAX_DATA_SZ);

	/* get the bucket in phash */
	bucket = get_bucket(key);
	bhead = shm_hash->phash->bucket[bucket].location;
	if (bhead != -1)
		/* next of the new entry to current head */
		shm_hash->pdata->entry[location].next = bhead;

	/* this is the first entry in the bucket */
	shm_hash->phash->bucket[bucket].location = location;
	return 0;
}

/* insert the data */
int shm_hash_insert_at_loc(shm_hash_t *shm_hash, uint64_t key, char *data)
{
	int		bucket = -1;
	int		location = -1;
	int		bhead = -1;

	/* get the first free location */
	location = shm_hash->pdata->header.loc;
	if (location >= NUM_HASH_DATA_ENTRIES) {
		printf("Could Not add key: %lu - Out of memory\n", key);
		return -1;
	}

	/* increment the location as the current location will be taken by the
	 * newly inserted entry */
	shm_hash->pdata->header.loc++;

	/* form the new entry to be inserted at location */
	shm_hash->pdata->entry[location].next = -1;
	shm_hash->pdata->entry[location].key = key;
	strncpy(shm_hash->pdata->entry[location].data, data, MAX_DATA_SZ);

	/* get the bucket in phash */
	bucket = get_bucket(key);
	bhead = shm_hash->phash->bucket[bucket].location;
	if (bhead != -1)
		/* next of the new entry to current head */
		shm_hash->pdata->entry[location].next = bhead;

	/* this is the first entry in the bucket */
	shm_hash->phash->bucket[bucket].location = location;
	return 0;
}

/* given the key return location if found */
int shm_hash_lookup_internal(shm_hash_t *shm_hash, uint64_t key)
{
	int		bucket = 0;
	int		bhead = 0;
	int		ffloc = 0;
	pdata_entry	*entry;

	if (!is_shm_hash_valid(shm_hash))
		return -1;

	bucket = get_bucket(key);
	bhead = shm_hash->phash->bucket[bucket].location;

	/* if bucket is empty - nothing to do */
	if (bhead == -1)
		return -1;

	entry = &shm_hash->pdata->entry[bhead];

	/* entry is the head node and that is what we are looking for */
	if (entry->key == key) {
		return bhead;
	}

	while (entry->next != -1) {
		pdata_entry		*curr_entry;
		int			curr_loc = entry->next;

		curr_entry = &shm_hash->pdata->entry[curr_loc];
		if (curr_entry->key == key) {
			return curr_loc;
		}

		/* prev = curr*/
		entry = curr_entry;
	}

	return -1;
}

/* public functions */

/* initialize the shared memory hash table */
shm_hash_t *shm_hash_init(char *name)
{
	if (name)
		return shm_hash_init_internal(name);
	return NULL;
}

int shm_hash_insert(shm_hash_t *shm_hash, uint64_t key, char *data)
{
	int		present = 0;
	char		tmp_str[256];
	int		loc = 0;

	if (!is_shm_hash_valid(shm_hash))
		return -1;

	/* check if this is an update */
	loc = shm_hash_lookup_internal(shm_hash, key);
	if (loc != -1) {
		strncpy(shm_hash->pdata->entry[loc].data, data, MAX_DATA_SZ);
		return 0;
	}

	/* check if we can assign a slot from free list */
	if (shm_hash->pdata->header.ffl != -1)
		return shm_hash_insert_at_ff(shm_hash, key, data);

	/* if we didnt find a location which was already freed, try inserting at
	 * one of the available location */
	return shm_hash_insert_at_loc(shm_hash, key, data);
}

/* API mallocs the data which is to befreed by the user */
int shm_hash_lookup(shm_hash_t *shm_hash,
		    uint64_t key,
		    char **data)
{
	int			loc = 0;

	if (!is_shm_hash_valid)
		return -1;
	if (!data)
		return -1;

	loc = shm_hash_lookup_internal(shm_hash, key);
	if (loc != -1) {
		int		len = 0;
		pdata_entry	*entry;

		entry = &shm_hash->pdata->entry[loc];
		len = strlen(entry->data);
		*data = malloc(len);
		assert(*data);
		strcpy(*data, entry->data);
	}

	return loc;
}

int shm_hash_delete(shm_hash_t *shm_hash, uint64_t key)
{
	int		bucket = 0;
	int		bhead = 0;
	int		ffloc = 0;
	pdata_entry	*entry;

	if (!is_shm_hash_valid(shm_hash))
		return -1;

	bucket = get_bucket(key);
	bhead = shm_hash->phash->bucket[bucket].location;
	ffloc = shm_hash->pdata->header.ffl;

	/* if bucket is empty - nothing to do */
	if (bhead == -1)
		return -1;

	entry = &shm_hash->pdata->entry[bhead];

	/* entry is the head node and that is what we are deleting */
	if (entry->key == key) {

		/* update the head in bucket */
		shm_hash->phash->bucket[bucket].location = entry->next;

		/* mark the deleted entry as a free entry */
		memset(entry, -1, sizeof(pdata_entry));
		entry->next = ffloc;

		/* set the ffloc to the newly deleted entry */
		shm_hash->pdata->header.ffl = bhead;

		return 0;
	}

	while (entry->next != -1) {
		pdata_entry		*curr_entry;
		int			curr_loc = entry->next;

		curr_entry = &shm_hash->pdata->entry[curr_loc];
		if (curr_entry->key == key) {

			/* prev->next = curr->next */
			entry->next = curr_entry->next;

			/* mark the current entry as free */
			memset(curr_entry, -1, sizeof(pdata_entry));
			curr_entry->next = ffloc;

			/* set the ffloc to the newly created free entry */
			shm_hash->pdata->header.ffl = curr_loc;

			return 0;
		}

		/* prev = curr*/
		entry = curr_entry;
	}

	/* entry not found! */
	return -1;
}

void shm_hash_print(shm_hash_t *shm_hash)
{
	int		i = 0;

	if (!is_shm_hash_valid)
		return;

	/* loop through all the buckets */
	for (i = 0; i < NUM_HASH_BUCKETS ; i++) {
		int		head = shm_hash->phash->bucket[i].location;
		pdata_entry	*tmp = NULL;

		if (head == -1)
			continue;
		printf("Bucket: %d\n", i);
		/* print all entries in this bucket if any */
		tmp = &shm_hash->pdata->entry[head];
		printf("	key: %lu data: %s\n", tmp->key, tmp->data);
		while(tmp->next != -1) {
			pdata_entry		*centry = NULL;

			centry = &shm_hash->pdata->entry[tmp->next];
			printf("	key: %lu data: %s\n", centry->key,
			       centry->data);
			tmp = centry;
		}
	}
}

int shm_hash_clean(shm_hash_t *shm_hash, int unlink)
{

	if (!is_shm_hash_valid)
		return -1;

	/* remove the memory mapped space from the process's address space */
	if (munmap(shm_hash->phash, sizeof(phash_t)) == -1) {
		printf("Failed to unmap hash buckets for %s\n",
		       shm_hash->hash_name);
		return -1;
	}

	if (close(shm_hash->hash_fd) == -1) {
		printf("Failed to close hash buckets for %s\n",
		       shm_hash->hash_name);
		return -1;
	}

	/* remove the memory mapped space from the process's address space */
	if (munmap(shm_hash->pdata, sizeof(pdata_t)) == -1) {
		printf("Failed to unmap data for %s\n",
		       shm_hash->hash_name);
		return -1;
	}

	if (close(shm_hash->data_fd) == -1) {
		printf("Failed to close data for %s\n",
		       shm_hash->hash_name);
		return -1;
	}

	/* unlink the shared memory objects */
	if (unlink) {
		char		*name = shm_hash->hash_name;
		char		*phash_url;   /* unique url to access the phash */
		char		*pdata_url;   /* unique url to access pdata */
		uint32_t	phash_url_sz; /* phash url = 'namehash' */
		uint32_t	pdata_url_sz; /* pdata url = 'namedata' */

		/* unlink the hash block */
		phash_url_sz = strlen(name) + strlen("hash") + 1;
		phash_url = malloc(phash_url_sz);
		if (!phash_url) {
			printf("Could not allocate hash_name\n");
			return -1;
		}
		snprintf(phash_url, phash_url_sz, "%s%s", name, "hash");
		shm_unlink(phash_url);

		/* unlink the data block */
		pdata_url_sz = strlen(name) + strlen("data") + 1;
		pdata_url = malloc(pdata_url_sz);
		if (!pdata_url) {
			printf("Could not allocate data_name\n");
			return -1;
		}
		snprintf(pdata_url, pdata_url_sz, "%s%s", name, "data");
		shm_unlink(pdata_url);

		free(phash_url);
		free(pdata_url);
	}

	/* free the allocated memory for shm_hash */
	free(shm_hash);
	shm_hash = NULL;

	return 0;
}
