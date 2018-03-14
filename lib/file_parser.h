/*
 * This file defines the data structures needed to store the contents of the
 * file.
 */
#ifndef _FILE_PARSER_H
#define _FILE_PARSER_H

#define MAX_FILE_NAME_SZ	256  /* max file name */
#define MAX_STR_SZ		256  /* max size of the string data */
#define MAX_FILE_SZ		1000 /* max numbers of records in a file */
#define MAX_LINE_SZ		1024

typedef struct rd_entry_t_ rd_entry_t;
typedef struct raw_data_t_ raw_data_t;

struct rd_entry_t_ {
	uint64_t	key;
	char		data[MAX_STR_SZ];
};

struct raw_data_t_ {
	int		size;
	rd_entry_t	*entries;
};

/*
 * parse_input_file
 * @desc: This API parses the contents of the input file
 * fname: name of the file to be parsed
 * rd: data will be stored in a array of struct raw_data_t
 * err: error string
 * err_sz: size of error string
 * return vlaue: 0 - file was parsed successfully, -1 - could not parse file
 */
extern int parse_input_file(char *fname, raw_data_t *rd, char *err, int err_sz);

/*
 * print raw data
 * @desc: print the raw data read from the input file
 * rd: array of raw_data
 * n: sizeof the array
 */
extern void print_raw_data(raw_data_t *rd, int n);

/*
 * raw_data_alloc
 * @desc: malloc raw_data
 * n: size of the array
 * return: allocated raw_data_t struct
 */
extern raw_data_t *raw_data_alloc(int n);

/*
 * raw_data_free
 * @desc: free allocated raw data
 * rd: pointer to raw data
 */
extern void raw_data_free(raw_data_t *rd);


#endif /* _FILE_PARSER_H */
