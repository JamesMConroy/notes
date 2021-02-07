/*
 *	notes 
 *
 *	command-line notebook application
 * 
 *	Copyright (C) 2017-2021 Free Software Foundation, Inc.
 *
 *	This is free software: you can redistribute it and/or modify it under
 *	the terms of the GNU General Public License as published by the
 *	Free Software Foundation, either version 3 of the License, or (at your
 *	option) any later version.
 *
 *	It is distributed in the hope that it will be useful, but WITHOUT ANY
 *	WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *	for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with it. If not, see <http://www.gnu.org/licenses/>.
 *
 * 	Written by Nicholas Christopoulos <nereus@freemail.gr>
 */

#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <wordexp.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#define _GNU_SOURCE
#include <fnmatch.h>

#include "list.h"
#include "str.h"
#include "nc-lib.h"

#define OPT_AUTO	0x0001
#define OPT_LIST	0x0002
#define OPT_EDIT	0x0004
#define OPT_VIEW	0x0008
#define OPT_ADD		0x0010
#define OPT_APPD	0x0020
#define OPT_DEL		0x0040
#define OPT_MOVE	0x0080
#define OPT_FILES	0x0100
#define OPT_ALL		0x0200
#define OPT_COMPL	0x0400
#define OPT_STDIN	0x0800
#define OPT_PRINT	0x1000
#define OPT_NOCLOB	0x2000

int		opt_flags = OPT_AUTO;

// === configuration & interpreter ==========================================

static char home[PATH_MAX];	// user's home directory
static char ndir[PATH_MAX];	// app data directory (per user)
static char bdir[PATH_MAX];	// backup directory
static char conf[PATH_MAX];	// app configuration file
static char current_section[NAME_MAX];
static char current_filter[NAME_MAX];
static char default_ftype[NAME_MAX];
static char onstart_cmd[LINE_MAX];
static char onexit_cmd[LINE_MAX];
static list_t *exclude;

// returns true if the string 'str' is value of true
bool istrue(const char *str) {
	const char *p = str;
	while ( isblank(*p) ) p ++;
	return (strchr("YT1+", toupper(*p)) != NULL);
	}

// add pattern to exclude list
void excl_add_pat(const char *pars) {
	char *string = strdup(pars);
	const char *delim = " \t";
	char *ptr = strtok(string, delim);
	while ( ptr ) {
		list_addstr(exclude, ptr);
		ptr = strtok(NULL, delim);
		}
	free(string);
	}

// user menu
typedef struct { char label[256]; char cmd[LINE_MAX]; } umenu_item_t;
static list_t *umenu;

void umenu_add(const char *pars) {
	char	*src = strdup(pars), *p;
	umenu_item_t u;
	
	if ( (p = strchr(src, ';')) != NULL ) {
		*p ++ = '\0';
		while ( isblank(*p) )	p ++;
		strcpy(u.cmd, p);
		p = src;
		while ( isblank(*p) )	p ++;
		rtrim(p);
		strcpy(u.label, p);
		list_add(umenu, &u, sizeof(umenu_item_t));
		}
	free(src);
	}

// === configuration & interpreter ==========================================
// === rules ================================================================

// rule view *.[0-9] man
// rule view *.man   man
// rule view *.md    bat
// rule view *.txt   less
// rule view *.pdf   okular
// rule edit *       $EDITOR
typedef struct { int code; char pattern[PATH_MAX], command[LINE_MAX]; } rule_t;
static list_t *rules;

// add rule to list
void rule_add(const char *pars) {
	char	*destp, pattern[PATH_MAX];
	int		action = 0;
	const char *p = pars;

	while ( isblank(*p) ) p ++;
	switch ( *p ) {
	case 'v': action = 'v'; break;
	case 'e': action = 'e'; break;
	default: return; }
	while ( isgraph(*p) ) p ++;
	if ( *p ) {
		while ( isblank(*p) ) p ++;
		if ( *p ) {
			destp = pattern;
			while ( isgraph(*p) ) *destp ++ = *p ++;
			*destp = '\0';
			if ( *p ) {
				while ( isblank(*p) ) p ++;
				if ( *p ) {
					rule_t	*rule = (rule_t *) malloc(sizeof(rule_t));
					rule->code = action;
					strcpy(rule->pattern, pattern);
					strcpy(rule->command, p);
					list_add(rules, rule, sizeof(rule_t));
					}
				}
			}
		}
	}

// execute rule for the file 'fn'
bool rule_exec(int action, const char *fn) {
	const char *base;
	list_node_t	*cur = rules->head;
	if ( (base = strrchr(fn, '/')) == NULL )
		return false;
	base ++;
	while ( cur ) {
		rule_t *rule = (rule_t *) cur->data;
		if ( rule->code == action && fnmatch(rule->pattern, base, FNM_PATHNAME | FNM_PERIOD | FNM_EXTMATCH) == 0 ) {
			char cmd[LINE_MAX];
			snprintf(cmd, LINE_MAX, "%s '%s'", rule->command, fn);
			system(cmd);
			return true;
			}
		cur = (list_node_t *) cur->next;
		}
	return false;
	}

// === configuration & interpreter ==========================================

// table of variables
typedef struct { const char *name; char *value; } var_t;
var_t var_table[] = {
	{ "notebook", ndir },
	{ "backupdir", bdir },
	{ "deftype", default_ftype },
	{ "onstart", onstart_cmd },
	{ "onexit", onexit_cmd },
	{ NULL, NULL } };

// table of commands
typedef struct { const char *name; void (*func_p)(const char *); } cmd_t;
cmd_t cmd_table[] = {
	{ "exclude", excl_add_pat },
	{ "rule", rule_add },
	{ "umenu", umenu_add },
	{ NULL, NULL } };

// assign variables
void command_set(const char *variable, const char *value) {
	for ( int i = 0; var_table[i].name; i ++ ) {
		if ( strcmp(var_table[i].name, variable) == 0 ) {
			strcpy(var_table[i].value, value);
			return;
			}
		}
	fprintf(stderr, "uknown variable [%s]\n", variable);
	}

// execute commands
void command_exec(const char *command, const char *parameters) {
	for ( int i = 0; cmd_table[i].name; i ++ ) {
		if ( strcmp(cmd_table[i].name, command) == 0 ) {
			if ( cmd_table[i].func_p )
				cmd_table[i].func_p(parameters);
			return;
			}
		}
	fprintf(stderr, "uknown command [%s]\n", command);
	}

// parse string line
void parse(const char *source) {
	char name[LINE_MAX], *d = name;
	const char *p = source;
	while ( isblank(*p) ) p ++;
	if ( *p && *p != '#' ) {
		while ( *p == '_' || isalnum(*p) )
			*d ++ = *p ++;
		*d = '\0';
		while ( isblank(*p) ) p ++;
		if ( *p == '=' ) {
			p ++;
			while ( isblank(*p) ) p ++;
			command_set(name, p);
			}
		else
			command_exec(name, p);
		}
	}

// copy file
bool copy_file(const char *src, const char *trg) {
	FILE	*inp, *outp;
	char	buf[LINE_MAX];
	size_t	bytes;
	
	if ( (inp = fopen(src, "r")) == NULL ) {
		fprintf(stderr, "%s: errno %d: %s\n", src, errno, strerror(errno));
		return false;
		}
	if ( (outp = fopen(trg, "w")) == NULL ) {
		fprintf(stderr, "%s: errno %d: %s\n", trg, errno, strerror(errno));
		fclose(inp);
		return false;
		}
	
	// copy
	while ( !feof(inp) ) {
		if ( (bytes = fread(buf, 1, sizeof(buf), inp)) > 0 )
			fwrite(buf, 1, bytes, outp);
		}
	
	// close
	fclose(outp);
	fclose(inp);
	return true;
	}

// read configuration file
void read_conf(const char *rc) {
	if ( access(rc, R_OK) == 0 ) {
		char buf[LINE_MAX];
		FILE *fp = fopen(rc, "r");
		if ( fp ) {
			while ( fgets(buf, LINE_MAX, fp) )
				parse(rtrim(buf));
			fclose(fp);
			}
		}
	}

// === notes ================================================================

typedef struct {
	char	file[PATH_MAX];		// full path filename
	char	name[NAME_MAX];		// name of note (basename)
	char	section[NAME_MAX];	// section
	char	ftype[NAME_MAX];	// file type
	struct stat st;				// just useless data for info
	} note_t;
list_t	*notes, *sections;

// backup note-file
bool note_backup(const note_t *note) {
	char	bckf[PATH_MAX];

	if ( strlen(bdir) ) {
		if ( strlen(note->section) )
			snprintf(bckf, PATH_MAX, "%s/%s/%s", bdir, note->section, note->name);
		else
			snprintf(bckf, PATH_MAX, "%s/%s", bdir, note->name);
		return copy_file(note->file, bckf);
		}
	return true;
	}

// prints information about the note
void note_pl(const note_t *note) {
	if ( opt_flags & OPT_FILES )
		printf("%s\n", note->file);
	else {
		size_t	seclen = 0;
		for ( list_node_t *cur = sections->head; cur; cur = (list_node_t *) cur->next )
			seclen = MAX(seclen, strlen((const char *) cur->data));
		printf("%-*s (%-3s) - %s\n", seclen, note->section, note->ftype, note->name);
		}
	}

// simple print (mode --print) of a note
void note_print(const note_t *note) {
	FILE *fp;
	char buf[LINE_MAX];
	
	printf("=== %s ===\n", note->name);
	if ( (fp = fopen(note->file, "rt")) != NULL ) {
		while ( fgets(buf, LINE_MAX, fp) )
			printf("%s", buf);
		fclose(fp);
		}
	else
		fprintf(stderr, "errno %d: %s\n", errno, strerror(errno));
	}

// delete a note
bool note_delete(const note_t *note) {
	note_backup(note);
	return (remove(note->file) == 0);
	}

// check filename to add in results list
bool dirwalk_checkfn(const char *fn) {
	list_node_t *cur;
	
    if ( strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0 )
		return false;
	for ( cur = exclude->head; cur; cur = (list_node_t *) cur->next ) {
		if ( fnmatch((const char *) cur->data, fn, FNM_PATHNAME | FNM_PERIOD | FNM_EXTMATCH | FNM_CASEFOLD) == 0 )
			return false;
		}
	return true;
	}

// walk throu subdirs to collect notes
void dirwalk(const char *name) {
	DIR *dir;
	struct dirent *entry;
    char path[PATH_MAX];
	size_t root_dir_len = strlen(ndir) + 1;

	if ( !(dir = opendir(name)) )	return;
	while ( (entry = readdir(dir)) != NULL ) {
		if ( !dirwalk_checkfn(entry->d_name) )
			continue;
		snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
		if ( entry->d_type == DT_DIR ) 
			dirwalk(path);
		else {
			note_t *note = (note_t *) malloc(sizeof(note_t));
			char	buf[PATH_MAX], *p, *e;
			strcpy(note->file, path);
			strcpy(buf, path + root_dir_len);
			if ( (e = strrchr(buf, '.')) != NULL ) {
				*e = '\0';
				strcpy(note->ftype, e + 1);
				}
			if ( (p = strrchr(buf, '/')) != NULL ) {
				*p = '\0';
				strcpy(note->section, buf);
				strcpy(note->name, p + 1);
				}
			else {
				note->section[0] = '\0';
				strcpy(note->name, buf);
				}
			if ( strlen(current_filter) == 0 || fnmatch(current_filter, note->name, FNM_PATHNAME | FNM_PERIOD | FNM_EXTMATCH | FNM_CASEFOLD) == 0 ) {
				stat(note->file, &note->st);
				list_add(notes, note, sizeof(note_t));
				if ( list_findstr(sections, note->section) == NULL )
					list_addstr(sections, note->section);
				}
			else
				free(note);
			}
		}
	closedir(dir);
	}

// copy contents of file to output
bool print_file_to(const char *file, FILE *output) {
	FILE	*input;
	char	buf[LINE_MAX];
	size_t	bytes;
	
	if ( (input = (file) ? fopen(file, "r") : stdin) != NULL ) {	
		while ( !feof(input) ) {
			if ( (bytes = fread(buf, 1, sizeof(buf), input)) > 0 )
				fwrite(buf, 1, bytes, output);
			}
		if ( file ) fclose(input);
		return true;
		}
	else 
		fprintf(stderr, "%s: errno %d: %s\n", file, errno, strerror(errno));
	return false;
	}

// expand envirnment variables
void vexpand(char *buf) {
	wordexp_t p;
	
	if ( wordexp(buf, &p, 0) == 0 ) {
		strcpy(buf, "");
		for ( int i = 0; i < p.we_wordc; i ++ )
			strcat(buf, p.we_wordv[i]);
		wordfree(&p);
		}
	}

//
void normalize_section_name(char *section) {
	list_node_t *cur;
	note_t		*note;

	for ( cur = notes->head; cur; cur = (list_node_t *) cur->next ) {
		note = (note_t *) cur->data;
		if ( strcasecmp(note->section, section) == 0 ) {
			strcpy(section, note->section);
			break;
			}
		}
	}

// if section does not exists, creates it
void make_section(const char *sec) {
	char dest[PATH_MAX];

	if ( strlen(sec) ) {
		snprintf(dest, PATH_MAX, "%s/%s", ndir, sec);
		if ( access(dest, X_OK) != 0 ) { // new section ?
			if ( mkdir(dest, 0700) != 0 ) {
				fprintf(stderr, "mkdir(%s): errno %d: %s\n", dest, errno, strerror(errno));
				exit(EXIT_FAILURE);
				}
			}
		}
	}

// create a note node
note_t*	make_note(const char *name, const char *defsec, int flags) {
	note_t *note = (note_t *) malloc(sizeof(note_t));
	FILE *fp;
	const char *p;

	if ( (p = strrchr(name, '/')) != NULL ) {
		strcpy(note->name, p+1);
		strncpy(note->section, name, p - name);
		note->section[p - name] = '\0';
		normalize_section_name(note->section);
		make_section(note->section);
		snprintf(note->file, PATH_MAX, "%s/%s/%s", ndir, note->section, note->name);
		}
	else {
		strcpy(note->name, name);
		if ( defsec && strlen(defsec) ) {
			strcpy(note->section, defsec);
			normalize_section_name(note->section);
			make_section(note->section);
			snprintf(note->file, PATH_MAX, "%s/%s/%s", ndir, note->section, note->name);
			}
		else {
			strcpy(note->section, "");
			snprintf(note->file, PATH_MAX, "%s/%s", ndir, note->name);
			}
		}
	
	if ( strrchr(name, '.') == NULL ) { // if no file extension specified
		strcat(note->file, ".");
		strcat(note->file, default_ftype);
		}
	
	// create the file
	if ( flags & 0x01 ) { // create file
		if ( (opt_flags & OPT_ADD) && !(opt_flags & OPT_NOCLOB) ) {
			if ( access(note->file, F_OK) == 0 ) {
				fprintf(stderr, "File '%s' already exists.\n", note->file);
				return NULL;
				}
			}
		if ( (fp = fopen(note->file, "wt")) != NULL )
			fclose(fp);
		else {
			free(note);
			note = NULL;
			}
		}
	return note;
	}

// === explorer =============================================================
static note_t **t_notes;
static int	t_notes_count;
static list_t	*tagged;
static WINDOW	*w_lst, *w_prv, *w_inf;

// short date
const char *sdate(const time_t *t, char *buf) {
	struct tm *tmp;
	
	tmp = localtime(t);
	if ( tmp ) 
		strftime(buf, 128, "%F %H:%M", tmp);
	else
		strcpy(buf, "* error *");
	return buf;
	}

// get a string value
bool ex_input(char *buf, const char *prompt_fmt, ...) {
	char	prompt[LINE_MAX];

	va_list ap;
	va_start(ap, prompt_fmt);
	vsnprintf(prompt, LINE_MAX, prompt_fmt, ap);
	va_end(ap);
	
	mvhline(getmaxy(stdscr) - 3, 0, ACS_HLINE, getmaxx(stdscr));
	move(getmaxy(stdscr) - 2, 0); clrtoeol();
	printw("  %s", prompt);
	move(getmaxy(stdscr) - 1, 0); clrtoeol();
	printw("> ");
	return nc_editstr(buf, getmaxx(stdscr)-4);
	}

// print status line
void ex_status_line(const char *fmt, ...) {
	char	msg[LINE_MAX];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, LINE_MAX, fmt, ap);
	va_end(ap);

	werase(w_inf);
	wattron(w_inf, A_REVERSE);
	mvwhline(w_inf, 0, 0, ' ', getmaxx(w_inf));
	// │┃
	nc_wprintf(w_inf, " %d notes ┃ %s ", list_count(notes), msg);
	wattroff(w_inf, A_REVERSE);
	wrefresh(w_inf);
	}

// display list of notes (list window)
void ex_print_list(int offset, int pos) {
	int		y = 0, lines = getmaxy(w_lst);
	
	werase(w_lst);
	if ( t_notes_count ) {
		for ( int i = offset; i < t_notes_count && i < offset + lines; i ++, y ++ ) {
			if ( pos == i ) wattron(w_lst, A_REVERSE);
			mvwhline(w_lst, y, 0, ' ', getmaxx(w_lst));
			if ( strlen(t_notes[i]->section) ) {
				int l = strlen(t_notes[i]->section);
				wattron(w_lst, A_DIM);
				mvwprintw(w_lst, y, getmaxx(w_lst)-l, "%s", t_notes[i]->section);
				wattroff(w_lst, A_DIM);
				}
			mvwprintw(w_lst, y, 0, "%c%s ",
				((list_findptr(tagged, t_notes[i]) ) ? '+' : ' '), t_notes[i]->name);
			if ( pos == i ) wattroff(w_lst, A_REVERSE);
			}
		}
	else
		mvwprintw(w_lst, 0, 0, "* No notes found! *");
	wrefresh(w_lst);
	}

// display the contents of the note (preview window)
void ex_print_note(const note_t *note) {
	FILE	*fp;
	char	buf[LINE_MAX];
	bool	inside_code = false;
	
	werase(w_prv);
	if ( note ) {
		nc_wprintf(w_prv, "Name: \eb+%s\eb-", note->name);
		if ( strlen(note->section) )
			nc_wprintf(w_prv, ", Section: \eb+%s\eb-", note->section);
		nc_wprintf(w_prv, "\nFile: \eb+%s\eb-\n", note->file);
		nc_wprintf(w_prv, "Date: \eb+%s\eb-\n", sdate(&note->st.st_mtime, buf));
		nc_wprintf(w_prv, "Stat: \eb+%6d\eb- bytes, mode \eb+0%o\eb-, owner \eb+%d\eb-:\eb+%d\eb-\n",
			note->st.st_size, note->st.st_mode & 0777, note->st.st_uid, note->st.st_gid);
		for ( int i = 0; i < getmaxx(w_prv); i ++ ) wprintw(w_prv, "─");
		if ( (fp = fopen(note->file, "rt")) != NULL ) {
			while ( fgets(buf, LINE_MAX, fp) ) {
				if ( strcmp(note->ftype, "md") == 0 ) {
					switch ( buf[0] ) {
					case '#': if ( inside_code ) nc_wprintf(w_prv, "\ec20+%s\ec20-", buf); else nc_wprintf(w_prv, "\eb+%s\eb-", buf); break;
					case '`': if ( buf[1] == '`' && buf[2] == '`' ) inside_code = !inside_code; nc_wprintf(w_prv, "%s", buf); break;
					case '\t': nc_wprintf(w_prv, "\ec20+%s\ec20-", buf); break;
					default: if ( inside_code ) nc_wprintf(w_prv, "\ec20+%s\ec20-", buf); else nc_wprintf(w_prv, "%s", buf);
						}
					}
				else
					nc_wprintf(w_prv, "%s", buf);
				if ( getcury(w_prv) >= (getmaxy(w_prv)-1) )
					break;
				}
			fclose(fp);
			}
		}
	wrefresh(w_prv);
	}

// qsort callback
static int t_notes_cmp(const void *va, const void *vb) {
	const note_t **a = (const note_t **) va;
	const note_t **b = (const note_t **) vb;
	return strcasecmp((*a)->name, (*b)->name);
	}

// help
static char *ex_help_s = "$? help, $quit, $view, $edit, $rename, $delete, $new, $filter, $tag, $untag all";
static char ex_help[LINE_MAX];
static char *ex_help_long = "\
?      ... Help. This window.\n\
q      ... Quit. Terminates the program.\n\
ENTER  ... Display the current note at $PAGER.\n\
v      ... View. Display the current or the tagged notes at $PAGER.\n\
e      ... Edit. Edit the current or the tagged notes with the $EDITOR.\n\
r      ... Rename. Renames the current note.\n\
d, DEL ... Delete. Deletes the curret or the tagged notes.\n\
n      ... New. Invokes the $EDITOR with the new file; you have to save it.\n\
a      ... Add. Creates a new empty note.\n\
s      ... Select Section.\n\
c      ... Change section. Changes the section of the current or the tagged notes.\n\
t, INS ... Tag/Untag current note.\n\
u      ... Untag all.\n\
f, /   ... Set Filter[1].\n\
m, F2  ... Menu. Invoke user-defined menu.\n\
x, F10 ... Execute something with current/tagged notes.\n\
F4     ... Open notes directory in file manager.\n\
F5     ... Rebuild list.\n\
\n\
Notes:\n\
[1] The application uses the same pattern as the shell with KSH extentions.\n\
    (see `man fnmatch`)\n\
";

// build the table with notes
bool ex_build() {
	if ( notes )
		list_clear(notes);
	if ( strlen(current_section) ) {
		char path[PATH_MAX];
		snprintf(path, PATH_MAX, "%s/%s", ndir, current_section);
		dirwalk(path);
		}
	else	
		dirwalk(ndir);
	t_notes = (note_t **) list_to_table(notes);
	t_notes_count = list_count(notes);
	if ( t_notes_count == 0 )
		return false;
	qsort(t_notes, t_notes_count, sizeof(note_t*), t_notes_cmp);
	return true;
	}

// rebuild the table with notes
bool ex_rebuild() {
	free(t_notes);
	return ex_build();
	}

// (re)build explorer windows
void ex_build_windows() {
	int cols3 = getmaxx(stdscr) / 3;
	int lines = getmaxy(stdscr) - 1;
	
	if ( w_lst ) delwin(w_lst);
	if ( w_prv ) delwin(w_prv);
	if ( w_inf ) delwin(w_inf);
	w_lst = newwin(lines, cols3, 0, 0);
	w_prv = newwin(lines, (getmaxx(stdscr) - cols3) - 1, 0, cols3+1);
	w_inf = newwin(1, getmaxx(stdscr), lines, 0);
	mvvline(0, cols3, ' ', lines);
	keypad(w_lst, TRUE);
	keypad(w_prv, TRUE);
	keypad(w_inf, TRUE);
	refresh();
	}

// find a note by name, returns the index in the t_notes
int	ex_find(const char *name) {
	int		i;
	
	for ( i = 0; i < t_notes_count; i ++ ) {
		if ( strcmp(t_notes[i]->name, name) == 0 ) 
			return i;
		}
	return -1;
	}

//
void ex_colorize(char *dest, const char *src) {
	const char *p = src, *e;
	char *d = dest;
	char nextchar[8];
	
	if ( has_colors() ) strcpy(nextchar, "\ec74");
	else strcpy(nextchar, "\eu");
	
	for ( p = src; *p; p ++ ) {
		if ( *p == '$' ) {
			p ++;
			for ( e = nextchar; *e; e ++ ) *d ++ = *e;
			*d ++ = '+';
			*d ++ = *p;
			for ( e = nextchar; *e; e ++ ) *d ++ = *e;
			*d ++ = '-';
			}
		else
			*d ++ = *p;
		}
	*d = '\0';
	}

//
#define ex_presh()		{ def_prog_mode(); endwin(); }
#define ex_refresh()	{ keep_status = 1; clear(); ungetch(12); }
#define fix_offset()	{ \
	if ( pos > offset + lines ) offset = pos - lines; \
	if ( offset > pos ) offset = pos; \
	if ( offset < 0 ) offset = 0; }

// TUI
void explorer() {
	bool	exitf = false;
	int		ch, offset = 0, pos = 0;
	int		lines, keep_status = 0;
	char	buf[LINE_MAX];
	char	prompt[LINE_MAX];
	char	status[LINE_MAX];
	
	if ( strlen(onstart_cmd) )
		system(onstart_cmd);

	ex_build();
	tagged = list_create();

	nc_init();
	ex_build_windows();
	ex_colorize(ex_help, ex_help_s);
	
	status[0] = '\0';
	do {
		lines = getmaxy(stdscr) - 2;
		fix_offset();
		ex_print_list(offset, pos);
		ex_print_note(t_notes[pos]);
		
		if ( status[0] == '\0' )
			ex_status_line("%s", ex_help);
		else {
			ex_status_line("%s", status);
			if ( keep_status )
				keep_status --;
			else
				status[0] = '\0';
			}
		
		// read key
		ch = getch();
		switch ( ch ) {
		case KEY_RESIZE:
			ex_build_windows();
			break;
		case 'q': exitf = true; break; // quit
		case '?':
			nc_view("Help", ex_help_long); 
			mvvline(0, getmaxx(stdscr) / 3, ' ', getmaxy(stdscr) - 1);
			break;
		case 'k': case KEY_UP:
			if ( t_notes_count )
				{ if ( pos ) pos --; }
			else
				offset = pos = 0;
			break;
		case 'j': case KEY_DOWN:
			if ( t_notes_count )
				{ if ( pos < t_notes_count - 1 ) pos ++; }
			else
				offset = pos = 0;
			break;
		case KEY_HOME: case 'g':
			offset = pos = 0;
			break;
		case KEY_END: case 'G':
			if ( t_notes_count )
				pos = t_notes_count - 1;
			break;
		case KEY_PPAGE:
		case KEY_NPAGE:
			if ( t_notes_count ) {
				int dir = (ch == KEY_NPAGE) ? 1 : -1;
				pos += lines * dir;
				offset += lines * dir;
				if ( pos >= t_notes_count )
					offset = pos = t_notes_count - 1;
				if ( pos < 0 )
					offset = pos = 0;
				}
			break;
		case '\n': case '\r':	// enter -> view current note
			if ( t_notes_count ) {
				ex_presh();
				rule_exec('v', t_notes[pos]->file);
				ex_refresh();
				}
			break;
		case 'u': // untag all
			list_clear(tagged);
			sprintf(status, "untag all.");
			ex_refresh();
			break;
		case 't': case KEY_IC: // tag/untag
			if ( t_notes_count ) {
				list_node_t *node = list_findptr(tagged, t_notes[pos]);
				if ( node )
					list_delete(tagged, node);
				else {
					list_addptr(tagged, t_notes[pos]);
					ungetch(KEY_DOWN);
					}
				}
			break;
		case 'v': // view in pager
			if ( t_notes_count ) {
				ex_presh();
				if ( list_count(tagged) ) {
					strcpy(buf, "$PAGER");
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next )
						{ strcat(buf, " '"); strcat(buf, ((note_t *) (cur->data))->file); strcat(buf, "'"); }
					setenv("NOTES_LIST", buf, 1);
					system(buf);
					}
				else
					rule_exec('v', t_notes[pos]->file);
				ex_refresh();
				}
			break;
		case 'f': case '/': // search/filter
			strcpy(buf, current_filter);
			if ( ex_input(buf, "Set filter (current filter: '%s')", current_filter) ) {
				strcpy(current_filter, buf);
				ex_rebuild();
				offset = pos = 0;
				}
			ex_refresh();
			break;
		case 'e': // edit
			if ( t_notes_count ) {
				ex_presh();
				if ( list_count(tagged) ) {
					strcpy(buf, "$EDITOR");
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next )
						{ strcat(buf, " '"); strcat(buf, ((note_t *) (cur->data))->file); strcat(buf, "'"); }
					system(buf);
					}
				else
					rule_exec('e', t_notes[pos]->file);
				ex_refresh();
				}
			break;
		case 's': // select current section
			strcpy(buf, "");
			if ( ex_input(buf, "Select section ?") ) {
				strcpy(current_section, buf);
				ex_rebuild();
				offset = pos = 0;
				}
			ex_refresh();
			break;
		case 'c': // change section
			if ( t_notes_count ) {
				strcpy(buf, "");
				if ( ex_input(buf, "Enter the new section") && strlen(buf) && (strchr(buf, '/') == NULL) ) {
					char *new_section = strdup(buf);
					normalize_section_name(new_section);
					make_section(new_section);
						
					// add the current element to tagged list
					if ( !list_count(tagged) )
						list_addptr(tagged, t_notes[pos]);
					
					// move files
					int succ = 0, fail = 0;
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next ) {
						note_t *cn = (note_t *) cur->data;
						note_backup(cn);
						note_t *nn = make_note(cn->name, new_section, 0);
						if ( rename(cn->file, nn->file) != 0 ) {
							sprintf(status, "move failed");
							fail ++;
							}
						else
							succ ++;
						free(nn);
						}

					// report
					if ( succ == 1 ) sprintf(status, "one note moved%c", ((fail)?';':'.'));
					else sprintf(status, "%d notes moved%c", succ, ((fail)?';':'.'));
					if ( fail ) sprintf(status+strlen(status), " %d failed.", fail);

					// cleanup
					list_clear(tagged);
					free(new_section);
					}
				
				ex_rebuild();
				ex_refresh();
				}
			break;
		case KEY_F(5):
			ex_rebuild();
			sprintf(status, "rebuilded.");
			ex_refresh();
			break;
		case KEY_F(4): // show in filemanager
			{
			char *fmans[] = { "xdg-open", "nnn", "mc", "thunar", "dolphin", NULL };
			int idx = nc_listbox("File Manager", (const char **) fmans);
			if ( idx > -1 ) {
				ex_presh();
				sprintf(buf, "%s %s", fmans[idx], ndir);
				system(buf);
				}
			ex_refresh();
			break;
			}
		case 'd': case KEY_DC: // delete
			if ( t_notes_count ) {
				strcpy(buf, "");
				if ( list_count(tagged) )
					sprintf(prompt, "Delete all tagged notes ?");
				else
					sprintf(prompt, "Do you want to delete '%s' ?", t_notes[pos]->name);
				
				if ( ex_input(buf, "%s", prompt) && istrue(buf) ) {
					if ( !list_count(tagged) )
						list_addptr(tagged, t_notes[pos]);
					int succ = 0, fail = 0;
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next )
						(note_delete(cur->data)) ? succ ++ : fail ++;
					if ( succ == 1 ) sprintf(status, "one note deleted%c", ((fail)?';':'.'));
					else sprintf(status, "%d notes deleted%c", succ, ((fail)?';':'.'));
					if ( fail ) sprintf(status+strlen(status), " %d failed.", fail);
					
					list_clear(tagged);
					ex_rebuild();
					if ( t_notes_count ) {
						if ( pos >= t_notes_count )
							pos = t_notes_count - 1;
						}
					else
						offset = pos = 0;
					}
				ex_refresh();
				}
			break;
		case 'r':	// rename
			if ( t_notes_count ) {
				strcpy(buf, t_notes[pos]->name);
				if ( ex_input(buf, "Enter the new name for '%s'", t_notes[pos]->name)
						&& strlen(buf)
						&& strcmp(buf, t_notes[pos]->name) != 0 ) {
					note_backup(t_notes[pos]);
					note_t *nn = make_note(buf, t_notes[pos]->section, 1);
					if ( !copy_file(t_notes[pos]->file, nn->file) )
						sprintf(status, "copy failed");
					else {
						if ( remove(t_notes[pos]->file) != 0 )
							sprintf(status, "delete old note failed");
						}
					free(nn);
					ex_rebuild();
					if ( (pos = ex_find(buf)) == -1 ) pos = 0;
					}
				ex_refresh();
				}
			break;
		case 'm': case KEY_F(2):
			if ( t_notes_count && umenu->head ) {
				int idx;
				umenu_item_t **opts;
				
				opts = (umenu_item_t **) list_to_table(umenu);
				if ( (idx = nc_listbox("User Menu", (const char **) opts)) > -1 ) {
					int tcnt = list_count(tagged);
					if ( !tcnt )
						list_addptr(tagged, t_notes[pos]);
					strcpy(buf, opts[idx]->cmd);
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next )
						{ strcat(buf, " '"); strcat(buf, ((note_t *) (cur->data))->file); strcat(buf, "'"); }
					
					clear();
					refresh();
					ex_presh();
					printf("Executing [%s]\n\n", buf);
					system(buf);
					printf("\nPress any key to return...\n");
					getch();
					if ( !tcnt )
						list_clear(tagged);
					}
				free(opts);
				ex_refresh();
				}
			break;
		case 'x': case KEY_F(10):
			if ( t_notes_count ) {
				char	cmd[LINE_MAX];
				strcpy(cmd, "");
				if ( ex_input(cmd, "Enter command") && strlen(cmd) ) {
					int tcnt = list_count(tagged);
					if ( !tcnt )
						list_addptr(tagged, t_notes[pos]);
					strcpy(buf, cmd);
					for ( list_node_t *cur = tagged->head; cur; cur = (list_node_t *) cur->next )
						{ strcat(buf, " '"); strcat(buf, ((note_t *) (cur->data))->file); strcat(buf, "'"); }
					clear();
					refresh();
					ex_presh();
					printf("Executing [%s]\n\n", buf);
					system(buf);
					printf("\nPress any key to return...\n");
					getch();
					if ( !tcnt )
						list_clear(tagged);
					}
				ex_refresh();
				}
			break;
		case 'n':	// new note
		case 'a':	// add a new note
			strcpy(buf, "");
			if ( ex_input(buf, "Enter new name") && strlen(buf) ) {
				note_t *note = make_note(buf, current_section, (ch == 'a') ? 1 : 0);
				if ( note ) {
					sprintf(status, "'%s' created", note->name);
					ex_rebuild();
					if ( (pos = ex_find(note->name)) == -1 ) pos = 0;
					if ( ch == 'n' ) { // 'new' key invokes the editor, 'add' key do not
						ex_presh();
						rule_exec('e', note->file);
						}
					free(note);
					}
				else
					sprintf(status, "failed: errno (%d) %s", errno, strerror(errno));
				}
			ex_refresh();
			break;
			}
		} while ( !exitf );
	nc_close();
	tagged = list_destroy(tagged);
	free(t_notes);
	if ( strlen(onexit_cmd) )
		system(onexit_cmd);
	}

// === main =================================================================

// set by env. variable, if $b exists then a=$b else a=c
#define setevar(a,b,c)	{ const char *e = getenv((b)); strcpy((a), (e)?e:(c)); }

// initialization
void init() {
	exclude = list_create();
	rules = list_create();
	umenu = list_create();
	notes = list_create();
	sections = list_create();
	
	// default values
	strcpy(default_ftype, "txt");
	setevar(home, "HOME", "/tmp");
	setevar(bdir, "BACKUPDIR", "");
	
	sprintf(ndir, "%s/Nextcloud/Notes", home);
	if ( access(conf, R_OK) != 0 )
		sprintf(ndir, "%s/.notes", home);
	if ( getenv("XDG_CONFIG_HOME") )
		sprintf(conf, "%s/notes/notesrc", getenv("XDG_CONFIG_HOME"));
	else
		sprintf(conf, "%s/.config/notes/notesrc", home);
	if ( access(conf, R_OK) != 0 )
		sprintf(conf, "%s/.notesrc", home);

	// read config file
	read_conf(conf);
	
	// expand
	vexpand(ndir);
	vexpand(bdir);

	// setting up default pager and editor
	rule_add("view * ${PAGER:-less}");
	rule_add("edit * ${EDITOR:-vi}");
	}

//
void cleanup() {
	exclude = list_destroy(exclude);
	rules = list_destroy(rules);
	umenu = list_destroy(umenu);
	notes = list_destroy(notes);
	sections = list_destroy(sections);
	}

#define APP_DESCR \
"notes - notes manager"

#define APP_VER "1.0"

static const char *usage = "\
"APP_DESCR"\n\
Usage: notes [mode] [options] [-s section] {note|pattern} [-|file(s)]\n\
\n\
Modes:\n\
    -a, --add      add a new note\n\
    -A, --append   append to note\n\
    -l, --list     list notes ('*' displays all)\n\
    -f, --files    same as list but displays only full pathnames (for scripts)\n\
    -v, --view     sends the note[s] to the $PAGER (see --all)\n\
    -p, --print    display the contents of a note[s] (see --all)\n\
    -e, --edit     load note[s] to $EDITOR (see --all)\n\
    -d, --delete   delete a note\n\
    -r, --rename   rename or move a note\n\
\n\
Options:\n\
    -s, --section  define section\n\
    -a, --all      displays all files that found, use it with -p, -v or -e\n\
    -              input from stdin\n\
\n\
Utilities:\n\
    WIP --cleanup      removes empty sections\n\
    WIP --complete     list notes for scripts (completion code)\n\
    --onstart      executes the 'onstart' command and returns its exit code.\n\
    --onexit       executes the 'onexit' command and returns its exit code.\n\
\n\
    -h, --help     this screen\n\
    --version      version and program information\n\
";

static const char *verss = "\
notes version "APP_VER"\n\
"APP_DESCR"\n\
\n\
Copyright (C) 2020-2021 Nicholas Christopoulos.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>\n\
";

// main()
int main(int argc, char *argv[]) {
	int		i, j, exit_code = EXIT_FAILURE;
	char	*asw = NULL;
	list_t	*args;
	note_t	*note;
	list_node_t *cur_arg = NULL;
	bool	sectionf = false;

	setlocale(LC_ALL, "");
	init();
	args = list_create();
	for ( i = 1; i < argc; i ++ ) {
		if ( argv[i][0] == '-' ) {
			if ( argv[i][1] == '\0' ) {     // one minus, read from stdin
				opt_flags |= OPT_STDIN;
				continue; // we finished with this argv
				}

			// check options
			for ( j = 1; argv[i][j]; j ++ ) {
				switch ( argv[i][j] ) {
				case 'l': opt_flags = OPT_LIST; break;
				case 'e': opt_flags = (opt_flags & OPT_AUTO) ? OPT_EDIT : opt_flags | OPT_EDIT; break;
				case 'v': opt_flags = OPT_VIEW; break;
				case 'p': opt_flags = OPT_VIEW|OPT_PRINT; break;
				case 'f': opt_flags |= OPT_FILES; break;
				case 'a': opt_flags = (opt_flags & OPT_AUTO) ? OPT_ADD : opt_flags | OPT_ALL; break;
				case '!': opt_flags |= OPT_NOCLOB; break;
				case 's': asw = current_section; sectionf = true; break;
				case 'r': opt_flags = OPT_MOVE; break;
				case 'd': opt_flags = OPT_DEL; break;
				case 'A': opt_flags = OPT_APPD;
				case 'h': puts(usage); return exit_code;
//				case 'v': puts(verss); return exit_code;
				case '-': // -- double minus
					if ( strcmp(argv[i], "--all") == 0 )			{ opt_flags |= OPT_ALL; }
					else if ( strcmp(argv[i], "--add") == 0 )		{ opt_flags = OPT_ADD; }
					else if ( strcmp(argv[i], "--add!") == 0 )		{ opt_flags = OPT_ADD | OPT_NOCLOB; }
					else if ( strcmp(argv[i], "--append") == 0 )	{ opt_flags = OPT_APPD; }
					else if ( strcmp(argv[i], "--list") == 0 )		{ opt_flags = OPT_LIST; }
					else if ( strcmp(argv[i], "--view") == 0 )		{ opt_flags = OPT_VIEW; }
					else if ( strcmp(argv[i], "--print") == 0 )		{ opt_flags = OPT_VIEW|OPT_PRINT; }
					else if ( strcmp(argv[i], "--edit") == 0 )		{ opt_flags = (opt_flags & OPT_AUTO) ? OPT_EDIT : opt_flags | OPT_EDIT; }
					else if ( strcmp(argv[i], "--files") == 0 )		{ opt_flags |= OPT_FILES; }
					else if ( strcmp(argv[i], "--delete") == 0 )	{ opt_flags = OPT_DEL; }
					else if ( strcmp(argv[i], "--rename") == 0 )	{ opt_flags = OPT_MOVE; }
					else if ( strcmp(argv[i], "--complete") == 0 )	{ opt_flags = OPT_COMPL; }
					else if ( strcmp(argv[i], "--section") == 0 )	{ asw = current_section; sectionf = true; }
					else if ( strcmp(argv[i], "--help") == 0 )		{ puts(usage); return exit_code; }
					else if ( strcmp(argv[i], "--version") == 0 )	{ puts(verss); return exit_code; }
					else if ( strcmp(argv[i], "--onstart") == 0 )	{ if ( strlen(onstart_cmd) ) return system(onstart_cmd); }
					else if ( strcmp(argv[i], "--onexit") == 0 )	{ if ( strlen(onexit_cmd) ) return system(onexit_cmd); }
					return exit_code;
				default:
					fprintf(stderr, "unknown option [%c]\n", argv[i][j]);
					return exit_code;
					}
				}
			}
		else {
			if ( asw ) { // wait for string
				strcpy(asw, argv[i]);
				asw = NULL;
				}
			else 
				list_addstr(args, argv[i]);
			}
		}

	// no parameters
	if ( args->head == NULL ) {
		if ( opt_flags & OPT_LIST )
			list_addstr(args, "*");
		else {
			if ( opt_flags & OPT_ADD )		{ printf("usage: notes -a new-note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_APPD )		{ printf("usage: notes -A note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_DEL )		{ printf("usage: notes -d note-name\n"); exit(EXIT_FAILURE); }
			if ( opt_flags & OPT_MOVE )		{ printf("usage: notes -r note-name new-note-name\n"); exit(EXIT_FAILURE); }
			
			// default action with no parameters: run explorer
			explorer(); 
			return EXIT_SUCCESS; 
			}
		}
	cur_arg = args->head;

	if ( (opt_flags & OPT_ADD) || (opt_flags & OPT_APPD) ) {
		//
		//	create/append note, $1 is the name
		//
		char	*name = (char *) cur_arg->data;
		note_t	*note;
		FILE	*fp;
		
		note = make_note(name, current_section, 1);
		if ( note ) {
			// create / truncate / open-for-append file
			if ( (fp = fopen(note->file, ((opt_flags & OPT_ADD) ? "w" : "a"))) != NULL ) {
				exit_code = EXIT_SUCCESS;
				cur_arg = (list_node_t *) cur_arg->next;
				while ( cur_arg ) {
					if ( print_file_to((const char *) cur_arg->data, fp) )
						printf("* '%s' copied *\n", (const char *) cur_arg->data);
					cur_arg = (list_node_t *) cur_arg->next;
					}
				if ( opt_flags & OPT_STDIN ) // the '-' option used
					print_file_to(NULL, fp);
				fclose(fp);
				if ( opt_flags & OPT_EDIT )  // the '-e' option used
					rule_exec('e', note->file);
				}
			else
				fprintf(stderr, "%s: errno %d: %s\n", note->file, errno, strerror(errno));
			free(note);
			}
		else
			fprintf(stderr, "%s: errno %d: %s\n", name, errno, strerror(errno));
		}
	else {
		//
		//	$1 is the note pattern, find note and do .. whatever
		//	
		
		// create list of files
		if ( sectionf ) {
			char path[PATH_MAX];
			snprintf(path, PATH_MAX, "%s/%s", ndir, current_section);
			dirwalk(path);
			}
		else
			dirwalk(ndir);

		// get list of notes according the pattern (argv)
		const char *note_pat = (const char *) cur_arg->data;
		cur_arg = (list_node_t *) cur_arg->next;
		list_t *res = list_create(); // list of results
		for ( list_node_t *np = notes->head; np; np = (list_node_t *) np->next ) {
			note = (note_t *) np->data;
			if ( sectionf ) {
				if ( strcmp(current_section, note->section) != 0 )
					continue;
				}
			if ( fnmatch(note_pat, note->name, FNM_PERIOD | FNM_CASEFOLD | FNM_EXTMATCH) == 0 ) {
				if ( (opt_flags & OPT_LIST) || (opt_flags & OPT_AUTO) || (opt_flags & OPT_FILES) )
					note_pl(note);
				list_addptr(res, note);
				}
			}

		//
		//	'res' has the collected files, now do whatever with them
		//
		size_t res_count = list_count(res);
		if ( !(opt_flags & OPT_FILES) && res_count ) {
			list_node_t *cur = res->head;
			
			exit_code = EXIT_SUCCESS;
			if ( (opt_flags == OPT_AUTO) && res_count == 1 )
				opt_flags |= OPT_VIEW;
			
			while ( cur ) {
				note = (note_t *) cur->data;
				
				if ( (opt_flags & OPT_VIEW) || (opt_flags & OPT_EDIT) ) {
					int action = 'v';
					if ( opt_flags & OPT_EDIT ) {
						action = 'e';
						note_backup(note);
						}
					if ( opt_flags & OPT_PRINT )
						note_print(note);
					else 
						rule_exec(action, note->file);
					if ( (opt_flags & OPT_ALL) == 0 )
						break;
					}
				else if ( opt_flags & OPT_DEL ) {
					if ( note_delete(note) )
						printf("* '%s' deleted *\n", note->name);
					else
						fprintf(stderr, "errno %d: %s\n", errno, strerror(errno));
					}
				else if ( opt_flags & OPT_MOVE ) {
					if ( cur_arg == NULL )
						fprintf(stderr, "usage: notes -r old-name new-name\n");
					else {
						char	new_file[PATH_MAX], *p, *ext;
						char	*arg = (char *) cur_arg->data;
						size_t	root_dir_len = strlen(ndir) + 1;
						
						if ( (ext = strrchr(note->file, '.')) == NULL )
							ext = default_ftype;
						else 
							ext ++;
						strcpy(new_file, note->file);
						new_file[root_dir_len] = '\0';
						strcat(new_file, arg);
						if ( (p = strrchr(arg, '.')) == NULL ) {
							strcat(new_file, ".");
							strcat(new_file, ext);
							}
						if ( rename(note->file, new_file) == 0 )
							printf("* '%s' -> '%s' succeed *\n", note->name, arg);
						else
							fprintf(stderr, "rename failed:\n[%s] -> [%s]\nerrno %d: %s\n",
								note->file, new_file, errno, strerror(errno));
						}
					break; // only one file
					}

				// next note
				cur = (list_node_t *) cur->next;
				}
			}
		else
			fprintf(stderr, "* no notes found *\n");
		
		res = list_destroy(res);
		}

	// finish
	args = list_destroy(args);
	cleanup();
	return exit_code;
	}


