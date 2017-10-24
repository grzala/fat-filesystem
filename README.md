This project is a simulation of a FAT16 Filesystem. It implements features such as:

	- ls (list contents of a directory)
	- cd (change directory)
	- mkdir and rmdir (create/remove directories)
	- creating, writing to, appending to, and removing files
	- move/copy files
	- using either relative or absolute paths

Use the makefile to compile the program (`make all`). The project is meant to be compiled and run on Linux.

File filesys.c and .h contain the filesystem itself. Shell.c shows an example usage of the filesystem.

Use `hexdump -C virtualdisk > file.txt` to write contents of the virtual disk to a file.