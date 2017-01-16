all:
	gcc -std=c99  filesys.c shell.c -o shell.exe
	gcc -std=c99  filesys.c optional.c -o optional.exe
