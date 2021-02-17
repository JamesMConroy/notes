# notes
Notes manager.

Concept: I want to keep my notes in plain text files, to search them like `man`
and to share them with my laptop and phone. I also want to use outputs of programs,
and to create, or append to, CLI files using echo. I would like to use the `groff`
format but I never did. I want to reorganize them with a file manager like `mc`.

* Notes can support any file type, but it is designed for plain-text (txt,md,troff,...).
* CLI interface. It is designed to behave as `man`.
* NCurses interface (run it without arguments).
* Nextcloud Notes compatible. I use nextcloud to synchronize my notes and use them in Android. So it designed to work well with Nextcloud.
* It can synchronized with almost any known utility (rsync, git, rclone, etc)
	(see notesrc(5), onstart/onexit)
* Viewer & Editor can be specified per file type.
* User-defined popup menu (see `man notesrc`).

The notes-files are stored in a user-defined directory with optional subdirectories.
The subdirectories in the application are named **sections**. Normally this
directory is the `~/Nextcloud/Notes`, or `~/.notes` if there is no nextcloud.
It can be specified in the configuration file (see `man notesrc`).
The viewer (pager) and the editor can be configured by file type in the configuration file.
There is a list to exclude files or directories from the notes-list (for example `.git` directrory).
In this application, _pattern_ means a shell pattern (with KSH additions), not a regular expression (see man fnmatch).

You will need my [md2roff](https://github.com/nereusx/md2roff) utility to create the man pages;
this in not necessary if you use the `*.man` files that are stored in this repo.

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
