#ifndef IDENT_SCRIPT_H
#define IDENT_SCRIPT_H

struct ident_script {
	char *name;
	char *email;
	char *date;
};

#define IDENT_SCRIPT_INIT { NULL, NULL, NULL }

void ident_script_init(struct ident_script *);

void ident_script_release(struct ident_script *);

/**
 * Reads and parses an author-script file, setting the ident_script fields
 * accordingly. Returns 0 on success, -1 if the file could not be parsed.
 *
 * The author script is of the format:
 *
 *	GIT_AUTHOR_NAME='$author_name'
 *	GIT_AUTHOR_EMAIL='$author_email'
 *	GIT_AUTHOR_DATE='$author_date'
 *
 * where $author_name, $author_email and $author_date are quoted. We are strict
 * with our parsing, as the file was meant to be eval'd in shell scripts, and
 * thus if the file differs from what this function expects, it is better to
 * bail out than to do something that the user does not expect.
 */
int read_author_script(struct ident_script *, const char *);

/**
 * Writes the ident_script to the author-script file.
 */
void write_author_script(const struct ident_script *, const char *);

int ident_script_from_line(struct ident_script *, const char *, size_t);

#endif /* IDENT_SCRIPT_H */
