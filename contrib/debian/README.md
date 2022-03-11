
Debian
====================
This directory contains files used to package thoughtd/thought-qt
for Debian-based Linux systems. If you compile thoughtd/thought-qt yourself, there are some useful files here.

## thought: URI support ##


thought-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install thought-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your thought-qt binary to `/usr/bin`
and the `../../share/pixmaps/thought128.png` to `/usr/share/pixmaps`

thought-qt.protocol (KDE)

