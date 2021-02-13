# notesrc 5 2021-01-22 NDC
## NAME
notesrc - NDC's notes configuration file.

## DESCRIPTION
*notes* uses a directory and its subdirectories to store the user's notes in files.
See `notebook` bellow.
The files can be plain text, markdown or anything else that
can configured by *rule* statements in the configuration file.

During startup *notes* reads the configuration file
*$XDG\_CONFIG\_HOME/notes/noterc* or *~/.config/notes/noterc* or *~/.notesrc*;
whichever is encountered first.

## VARIABLES

#### notebook = <directory>
The directory where the files are located.
If the directory is not specified then,
if *~/Nextcloud/Notes* found, it will
be used, otherwise the *~/.notes* will be used.

#### backupdir = <directory>
The *notes* stores backup files of the notes before edited or deleted.
If *backupdir* is omitted then environment variable *$BACKUPDIR* will be used if it is exists;
otherwise no backup will be used.

#### deftype = <extension>
This is the default extension file name when the user does not specify one in a new
note - name.

```
deftype=txt
```

#### onstart = <command-line>
Command to execute at startup of TUI or by option `--onstart`.

#### onexit = <command-line>
Command to execute at exit of TUI or by option `--onexit`.

#### clobber = <boolean>
Protection of unintentionally overwrite (same as shell).
Default is true.

## STATEMENTS

#### rule *action* *pattern* *command*
*Rules* defines how the program will act of each file type.
There are two *actions* for now, *view* and *edit*.

```
rule view *.[0-9] man %f
rule view *.man   man %f
rule view *.md    bat %f
rule view *.txt   less %f
rule view *.pdf   zathura %f
rule edit *       $EDITOR %f
```

#### exclude *pattern* [*pattern* ...]
File match patterns of files and/or directories to ignore.

```
exclude .git trash *.sqlite
```

#### umenu *label* ; *command*
Specifies user-defined menu item to deal with current or tagged files.
The command will get the full path files as parameters.
Use the 'm' key to invoke the user-defined menu.

```
umenu print the files ; echo
umenu print the contents ; cat
```

## COPYRIGHT
Copyright © 2020-2021 Nicholas Christopoulos.

License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

## AUTHOR
Written by Nicholas Christopoulos <mailto:nereus@freemail.gr>.

## SEE ALSO
[notes 1](man)

