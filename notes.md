# notes 1 2021-01-22 "NDC Tools Collection"
## NAME
notes - manages note files.

## SYNOPSIS
notes [*OPTIONS*] [-s *section*] {*note* | *pattern*} [-|*file* ...]

## DESCRIPTION
Your notes (files) are stored in a directory (and its subdirectories).
This directory can be the **Nextcloud®**'s (`~/Nextcloud/Notes`).
The files can be plain text, markdown or anything else that
can configured by the *rule* command in the configuration file (see notesrc(5)).
If the *note* is `-` then it reads from *stdin*.

Running program without arguments, enters in TUI mode (ncurses interface).

The program was designed to behave as the `man` command.

```
> # show note 'sig11'; if not found it will display all titles beginning with
> # the string
> notes sig11

> # show all pages whose title begins with 'sig11'
> notes sig11 -a

> # show page of section (i.e. subdirectory) 'unix' whose title begins with 'sig11'
> notes -s unix sig11
```

## OPTIONS

#### -a[!], --add[!]
Creates a new note file. If file extension is not specified then it will be used the
default (see notesrc).
If additional files are specified in the command line, their contents will be inserted into the new note.
Use it with `-e` to invoke the editor or `-` to get input from *stdin*.
If the name is already used in this section, then an error will be issued;
use `!` option to replace the existing file,
or set the clobber variable to `false` in the configuration file. (see notesrc(5))

```
# Example 1: cat yyy zzz >> xxx
> notes -a xxx yyy zzz

# example 2:
> echo "hello world" | notes -a xxx -

# example 3:
> cat ~/.notesrc | notes -a! notesrc -
```

#### -a[!]+, --append[!]
Same as `-a` but instead of overwriting, the new note is appended to the file.
If the name does not exist, then an error will be issued;
use `!` option to create it,
or set the clobber variable to `false` in the configuration file. (see notesrc(5))

#### -v, --view
Shows the *note* with the default *$PAGER* if one is not specified in the configuration file.

#### -p, --print
Same as `-v` but writes the contents to *stdout*.

#### -e, --edit
Loads the *note* to the default *$EDITOR* if one is not specified in the configuration file.

#### -l, --list
Displays the notes names that match *pattern*.

#### -f, --files
Same as `-l` but prints out the full path filenames.

#### -d, --delete
Deletes a note.

#### -r, --rename
Renames and/or moves a note. A second parameter is required to specify the new
name. If file extension is specified in the new name, then it will use it.
*rename* can also change the section if separated by '/' before the name,
e.g., `section3/new-name`.

#### --all
Displays all notes that were found; it works together with `-p`, `-e`, and `-v`.

#### -h, --help
Displays a short help text and exits.

#### --version
Displays the program version, copyright and license information and exits.

#### --onstart
Executes the command defined by `onstart` in the configuration file.
This option is useful when custom synchronization is needed.

#### --onexit
Executes the command defined by `onexit` in the configuration file.
This option is useful when custom synchronization is needed.

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

