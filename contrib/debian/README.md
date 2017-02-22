
Debian
====================
This directory contains files used to package navcoind/navcoin-qt
for Debian-based Linux systems. If you compile navcoind/navcoin-qt yourself, there are some useful files here.

## navcoin: URI support ##


navcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install navcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your navcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/navcoin128.png` to `/usr/share/pixmaps`

navcoin-qt.protocol (KDE)

