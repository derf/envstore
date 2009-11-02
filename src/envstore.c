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

/*
 * This struct is part of a linked list and contains one shell parameter
 */

struct parameter {
	char *name;
	char *content;
	struct parameter *next;
};


/*
 * Add element to linked list
 *
 * Parameters: list head (may be NULL), new list element
 * Returns new list head
 */

static struct parameter * list_add(struct parameter *first, struct parameter *new) {
	new->next = first;
	first = new;
	return first;
}


/*
 * Remove element from linked list
 *
 * Parameters: list head, ->name part of element to remove
 * Returns new list head (may be NULL)
 */

static struct parameter * list_remove(struct parameter *first, char *string) {
	struct parameter *cur = first;
	struct parameter *prev = NULL;

	while (cur != NULL) {
		if (strcmp(cur->name, string) == 0) {

			if (prev == NULL)
				return cur->next;

			prev->next = cur->next;
			return first;
		}
		prev = cur;
		cur = cur->next;
	}
	return first;
}


static void store_save(struct parameter *first, char *file) {
	struct parameter *cur = first;
	FILE *fp;
	char *tmpfile = malloc(strlen(file) + 5);

	if (tmpfile == NULL)
		err(EXIT_FAILURE, "malloc");

	if (snprintf(tmpfile, strlen(file) + 5, "%s.tmp", file) < 3)
		err(EXIT_FAILURE, "snprintf");

	umask(0077);
	fp = fopen(tmpfile, "w+");
	if (fp == NULL)
		err(EXIT_FAILURE, "Unable to save store to '%s'", file);

	while (cur != NULL) {
		fprintf(fp, "%s %s\n", cur->name, cur->content);
		cur = cur->next;
	}

	if (fclose(fp) != 0)
		err(EXIT_FAILURE, "fclose %s", file);

	if (rename(tmpfile, file) != 0)
		err(EXIT_FAILURE, "Unable to rename '%s' to '%s'", tmpfile, file);
}


static struct parameter * store_load(char *file) {
	struct parameter *first = NULL;
	struct parameter *new;
	struct stat finfo;
	uid_t self_uid = geteuid();
	char vname[128];
	char vcontent[256];
	FILE *fp = fopen(file, "r");

	/* Assume the store file does not exist and the store is empty
	 *   (-> return NULL for empty list)
	 * If it does in fact exist but has wrong permissions or similar,
	 * the user will only notice if it happens with envstore save/rm (FIXME)
	 */
	if (fp == NULL)
		return NULL;

	if (fstat(fileno(fp), &finfo) != 0)
		errx(EXIT_FAILURE, "Unable to verify store file permissions (%s)", file);

	if (finfo.st_uid != self_uid)
		errx(EXIT_FAILURE, "Store file '%s' is insecure (must be owned by you, not uid %d)", file, finfo.st_uid);

	if ((finfo.st_mode & 077) > 0)
		errx(EXIT_FAILURE, "Store file '%s' has insecure permissions %04o (recommended: 0600)", file, finfo.st_mode & 07777);

	while (fscanf(fp, "%127s %255[^\n]\n", vname, vcontent) != EOF) {
		new = malloc(sizeof (struct parameter));
		if (new == NULL)
			err(EXIT_FAILURE, "malloc");

		new->name = strdup(vname);
		new->content = strdup(vcontent);

		if ((new->name == NULL) || (new->content) == NULL)
			err(EXIT_FAILURE, "strdup");

		first = list_add(first, new);
	}

	if (fclose(fp) != 0)
		err(EXIT_FAILURE, "fclose %s", file);

	return first;
}


static inline void command_clear(char *store_file) {
	/*
	 * No error checking - assume that the file didn't exist in the first place
	 * if unlink fails.
	 */
	unlink(store_file);
}


static inline void command_eval(char *store_file) {
	struct parameter *first = store_load(store_file);
	struct parameter *cur = first;
	unsigned long int i;

	while (cur != NULL) {

		printf("export %s='", cur->name);
		for (i = 0; i < strlen(cur->content); i++) {
			if (cur->content[i] == '\'')
				fputs("'\"'\"'", stdout);
			else
				putchar(cur->content[i]);
		}
		fputs("'\n", stdout);
		cur = cur->next;
	}
}


static inline void command_list(char *store_file) {
	struct parameter *first = store_load(store_file);
	struct parameter *cur = first;

	while (cur != NULL) {
		printf("%-15s = %s\n", cur->name, cur->content);
		cur = cur->next;
	}
}


static inline void command_rm(char *store_file, char *param) {
	struct parameter *first = store_load(store_file);
	first = list_remove(first, param);
	store_save(first, store_file);
}


static inline void command_save(char *store_file, char *param, char *value, int argc) {
	struct parameter *first = store_load(store_file);
	struct parameter new;
	char *newvalue;
	first = list_remove(first, param);

	if (argc > 3)
		newvalue = value;
	else
		newvalue = getenv(param);

	if (newvalue == NULL)
		errx(EXIT_FAILURE, "parameter '%s' has no value", param);

	if ((strlen(param) > 127) || (strlen(newvalue) > 255))
		errx(EXIT_FAILURE, "parameter or value too long (see man envstore -> LIMITATIONS)");

	new.name = param;
	new.content = newvalue;

	first = list_add(first, &new);
	store_save(first, store_file);
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
			command_eval(store_file);
			break;
		case 'l':
			command_list(store_file);
			break;
		case 'r':
			if (argc < 3)
				errx(EXIT_FAILURE, "Usage: rm <parameter>");
			command_rm(store_file, argv[2]);
			break;
		case 's':
			if (argc < 3)
				errx(EXIT_FAILURE, "Usage: save <parameter> [value]");
			command_save(store_file, argv[2], argv[3], argc);
			break;
		default:
			errx(EXIT_FAILURE, "Unknown action: %s", argv[1]);
			break;
	}

	return EXIT_SUCCESS;
}
