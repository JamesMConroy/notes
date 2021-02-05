# notes
Notes manager.

* Notes are plain text files (txt,md,troff,...). It can support any type, but it is designed for plain-text.
* CLI interface. It is designed to behave as `man`.
* NCurses interface (run it without arguments).
* Nextcloud Notes compatible. I use nextcloud to synchronize my notes and use them in Android. So it builded to work well with Nextcloud.
* User-defined popup menu (see `man notesrc`).

You will need my [md2roff](https://github.com/nereusx/md2roff) utility to create the man pages; this in not necessary if you use the `*.man` files that are stored in this repo.

## Installation

First install md2roff from https://github.com/nereusx/md2roff if you have not
already. Run 'cd notes; make install'.

```
git clone https://github.com/nereusx/notes
cd notes
make install
```

## Screenshots

![NCurses interface](https://raw.githubusercontent.com/nereusx/notes/main/screenshots/notes-112x30.png)

![CLI](https://raw.githubusercontent.com/nereusx/notes/main/screenshots/notes-cli.png)
