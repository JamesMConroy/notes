# notes
Notes manager.

Concept: I want to keep my notes in plain text files, to search them like `man`
and to share them with my laptop and phone. I want to add outputs of programs plus
to append/create from cli with `echo`. I like to use `groff` format but I never
did it. I want to reorganize them with a file manager like `mc`.

* Notes are plain text files (txt,md,troff,...). It can support any type, but it is designed for plain-text.
* CLI interface. It is designed to behave as `man`.
* NCurses interface (run it without arguments).
* Nextcloud Notes compatible. I use nextcloud to synchronize my notes and use them in Android. So it designed to work well with Nextcloud.
* Viewer & Editor can specified per file type.
* User-defined popup menu (see `man notesrc`).

The notes-files must be stored in one user-defined place and can have subdirectories.
The subdirectories in the application are named **sections**. Normally this
directory is the `~/Nextcloud/Notes`, or `~/.notes` if there is no nextcloud.
It can be specified in configuration file (see `man notesrc`).
The viewer (pager) and the editor can be configured by file type in
configuration file. There is a list to exclude files or directories (for example `.git` directrory).
When the application referred to **pattern** it means shell patterns (with KSH
additions) not regular expressions (see `man fnmatch`).

You will need my [md2roff](https://github.com/nereusx/md2roff) utility to create the man pages; this in not necessary if you use the `*.man` files that are stored in this repo.

## Installation

First install md2roff from https://github.com/nereusx/md2roff if you have not
already.

```
git clone https://github.com/nereusx/notes
cd notes
make install
```

## Screenshots

![NCurses interface](https://raw.githubusercontent.com/nereusx/notes/main/screenshots/notes-112x30.png)

![CLI](https://raw.githubusercontent.com/nereusx/notes/main/screenshots/notes-cli.png)

[The fonts](https://github.com/nereusx/xsg-fonts)
