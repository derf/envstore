/*
 * Copyright © 2009 by Daniel Friesel <derf@derf.homelinux.org>
 * License: WTFPL <http://sam.zoy.org/wtfpl>
 *
 * Function policy: Fail early (use err/errx if something goes wrong)
 */

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CMD_EVAL 1
#define CMD_LIST 2
#define CMD_RM 3
#define CMD_SAVE 4

#define PARAM_LENGTH 256
#define VALUE_LENGTH 1024
#define SCAN_FORMAT "%255s %1023[^\n]\n"

static FILE * store_open(char *file) {
	struct stat finfo;
	uid_t self_uid = geteuid();
	FILE *fp = fopen(file, "r");

	/* Assume the store file does not exist and the store is empty
	 *   (-> return NULL for empty list)
	 * If it does in fact exist but has wrong permissions or similar,
	 * the user will only notice if it happens with envstore save/rm (FIXME)
	 */
	if (fp == NULL)
		return NULL;

	if (fstat(fileno(fp), &finfo) != 0)
		err(EXIT_FAILURE, "Unable to verify store file permissions (%s)", file);

	if (finfo.st_uid != self_uid)
		errx(EXIT_FAILURE, "Store file '%s' is insecure (must be owned by you, not uid %d)", file, finfo.st_uid);

	if ((finfo.st_mode & 077) > 0)
		errx(EXIT_FAILURE, "Store file '%s' has insecure permissions %04o (recommended: 0600)", file, finfo.st_mode & 07777);

	return fp;
}

static char * new_filename(char *file) {
	char *new_file = malloc(strlen(file) + 5);

	if (new_file == NULL)
		err(EXIT_FAILURE, "malloc");

	if (snprintf(new_file, strlen(file) + 5, "%s.tmp", file) < 3)
		err(EXIT_FAILURE, "snprintf");

	return new_file;
}

static FILE * store_open_new(char *file) {
	FILE *fp;

	umask(0077);
	fp = fopen(file, "w");

	if (fp == NULL)
		err(EXIT_FAILURE, "fopen %s", file);

	return fp;
}


/*
 * A little note on shell quoting:
 * ' at beginning and end do most of the job for us, except when we have a '
 * inside the quoted value. It can't be escaped directly, so we end the quote
 * (with a ') and then add an escaped '. Possible ways are \' and "'" (in this
 * case, the latter is used). Afterwards another normal ' is added and the
 * quoted string continues.
 */
static inline void print_escaped(char *name, char *content) {
	unsigned int i;

	printf("export %s='", name);

	for (i = 0; i < strlen(content); i++) {
		if (content[i] == '\'')
			fputs("'\"'\"'", stdout);
		else
			putchar(content[i]);
	}

	fputs("'\n", stdout);
}


static void command_disp(char *file, int command) {
	char vname[PARAM_LENGTH];
	char vcontent[VALUE_LENGTH];
	FILE *fp = store_open(file);

	if (fp == NULL)
		exit(EXIT_SUCCESS);

	while (fscanf(fp, SCAN_FORMAT, vname, vcontent) != EOF) {
		if (command == CMD_LIST)
			printf("%-15s = %s", vname, vcontent);
		else
			print_escaped(vname, vcontent);
	}
	if (fclose(fp) != 0)
		err(EXIT_FAILURE, "fclose %s", file);
}

static void command_rm_save(char *old_file, char *param, char *value, int argc, int mode) {
	char curparam[PARAM_LENGTH];
	char curvalue[VALUE_LENGTH];
	char *newvalue;
	char *new_file = new_filename(old_file);
	FILE *old_fp = store_open(old_file);
	FILE *new_fp = store_open_new(new_file);

	if (old_fp != NULL) {
		while (fscanf(old_fp, SCAN_FORMAT, curparam, curvalue) != EOF) {
			if (strcmp(curparam, param) != 0)
				if (fprintf(new_fp, "%s %s\n", curparam, curvalue) <= 0)
					err(EXIT_FAILURE, "fprintf %s", new_file);
		}
		if (fclose(old_fp) != 0)
			err(EXIT_FAILURE, "fclose %s", old_file);
	}

	if (mode == CMD_SAVE) {
		if (argc > 3)
			newvalue = value;
		else
			newvalue = getenv(param);

		if (newvalue == NULL)
			errx(EXIT_FAILURE, "parameter '%s' has no value", param);

		if ((strlen(param) > PARAM_LENGTH - 1) || (strlen(newvalue) > VALUE_LENGTH - 1))
			errx(EXIT_FAILURE, "parameter or value too long (see man envstore -> LIMITATIONS)");

		if (fprintf(new_fp, "%s %s\n", param, newvalue) <= 0)
			err(EXIT_FAILURE, "fprintf %s", new_file);
	}

	if (fclose(new_fp) != 0)
		err(EXIT_FAILURE, "fclose %s", new_file);

	if (rename(new_file, old_file) != 0)
		err(EXIT_FAILURE, "Unable to rename '%s' to '%s'", new_file, old_file);
}


static inline void command_clear(char *store_file) {
	/*
	 * No error checking - assume that the file didn't exist in the first place
	 * if unlink fails.
	 */
	unlink(store_file);
}


int main(int argc, char **argv) {
	char *store_file;
	uid_t my_uid = geteuid();

	if (argc < 2)
		errx(EXIT_FAILURE, "Insufficient arguments");

	store_file = getenv("ENVSTORE_FILE");
	if (store_file == NULL) {
		store_file = malloc(64);
		if (store_file == NULL) {
			err(EXIT_FAILURE, "malloc");
		}
		if (snprintf(store_file, 63, "/tmp/envstore-%d", (int)my_uid) < 10) {
			err(EXIT_FAILURE, "snprintf");
		}
	}


	switch (argv[1][0]) {
		case 'c':
			command_clear(store_file);
			break;
		case 'e':
			command_disp(store_file, CMD_EVAL);
			break;
		case 'l':
			command_disp(store_file, CMD_LIST);
			break;
		case 'r':
			if (argc < 3)
				errx(EXIT_FAILURE, "Usage: rm <parameter>");
			command_rm_save(store_file, argv[2], argv[3], argc, CMD_RM);
			break;
		case 's':
			if (argc < 3)
				errx(EXIT_FAILURE, "Usage: save <parameter> [value]");
			command_rm_save(store_file, argv[2], argv[3], argc, CMD_SAVE);
			break;
		default:
			errx(EXIT_FAILURE, "Unknown action: %s", argv[1]);
			break;
	}

	return EXIT_SUCCESS;
}
