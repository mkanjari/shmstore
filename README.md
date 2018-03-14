# shmstore - a shared memory key,value store
A simple shared memory <key,value> store.

Key is a uint64_t key and value is a string (of size 256). 

shmstore provides the follwing APIs defined in lib/shm.h
1. shm_hash_init
2. shm_hash_insert
3. shm_hash_lookup
4. shm_hash_clean

The shared memory is divided into two parts:
1. hash buckets
2. hash data
