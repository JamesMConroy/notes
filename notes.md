# notes 1 2021-01-22 "NDC Tools Collection"
## NAME
notes - manages note files.

## SYNOPSIS
notes [*OPTIONS*] [-s *section*] {*note* | *pattern*} [-|*file* ...]

## DESCRIPTION
notes uses a directory to store users files for notes. The directory can be
a **Nextcloud®** Notes. The files can be plain text, markdown or anything else that
can configured by *rule* command in configuration file (see notesrc(5)).
If the *note* is `-` then it reads from *stdin*.

Running program without arguments, enters in TUI mode (ncurses interface).

The program was designed to behave as `man` command.

```
> # show note 'sig11'; if not found it will display all titles containing
> # the string
> notes sig11

> # show all pages that match 'sig11'
> notes sig11 -a

> # show page of section 'unix' that match 'sig11'
> notes -s unix sig11
```

## OPTIONS

#### -a[!], --add[!]
Creates a new note file. If file extension is not specified then it will use the
default (see notesrc). If additional file(s) are specified in the command line
will be inserted on the final note.
Use it with `-e` to invoke the editor or with `-` to get input from *stdin*.
If note exists then an error will created; use `!` to avoid this and truncate
the existing file.

#### -A, --append
Same as `-a` but appends to an existing (or not) note. If note does not exists
then it will be created.

#### -v, --view
Shows the *note* with the default *$PAGER* or the one specified by `rule` (see
notesrc).

#### -p, --print
Same as `-v` but writes the contents to *stdout*.

#### -e, --edit
Loads the *note* to the default *$EDITOR* or the one specified by `rule` (see
notesrc).

#### -l, --list
Displays the notes names that match to the *pattern*.

#### -f, --files
Same as `-l` but prints out the full-path filenames instead of their titles.

#### -d, --delete
Deletes a note.

#### -r, --rename
Renames (and/or moves) note. A second parameter is required to specify the new
name. If file extension is specified in the new name, then it will use it.
Rename can also change the section if separated by '/' before the name.

#### --all
Displays all notes that found (-p, -e, -v).

#### -h, --help
Displays a short-help text and exits.

#### --version
Displays the program version, copyright and license information and exists.

#### --onstart
Executes the command defined by `onstart` in the configuration file.
This option is useful when custom synchronization needed.

#### --onexit
Executes the command defined by `onexit` in the configuration file.
This option is useful when custom synchronization needed.

## COPYRIGHT
Copyright © 2020-2021 Nicholas Christopoulos.

License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

## AUTHOR
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>.

## SEE ALSO
[notesrc 5](man),
[groff_man 7](man), [man-pages 7](man).

