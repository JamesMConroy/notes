# notesrc 5 2021-01-22 NDC
## NAME
notesrc - NDC's notes configuration file.

## DESCRIPTION
*notes* uses a directory to store users files for notes. The directory can be
a **Nextcloud®** Notes. The files can be plain text, markdown or anything else that
can configured by *rule* statements in configuration file.

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
This is the default file type that used when user did not specify one in the
note - name. Actually this is used only when you add new notes.

```
deftype=txt
```

## STATEMENTS

#### rule *action* *pattern* *command*
*Rules* defines how the program will react of each file type.
The are two *actions* for now, the *view* and the *edit*.

```
rule view *.[0-9] man
rule view *.man   man
rule view *.md    bat
rule view *.txt   less
rule view *.pdf   zathura
rule edit *       $EDITOR
```

#### exclude *pattern* [*pattern* ...]
File match patterns of files / directories to ignore.

```
exclude .git trash *.sqlite
```

#### umenu *label* ; *command*
Specifies user-defined menu item to deal with current or tagged files.
The command will get the list of files as parameter.
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

