#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

void print_file(FILE* file, int n) {
	printf("FILE %d:\n", n);
	printf("\t_Mode = %d\n", file->_Mode);
	printf("\t_Idx = %d\n", file->_Idx);
	printf("\t_Wstate = %d\n", file->_Wstate);
	printf("\t_Cbuf = %d\n", file->_Cbuf);
	printf("\tfpos = %d\n", file->fpos);
	printf("\t_Rsize = %d\n", file->_Rsize);
	printf("\t_offset = %d\n", file->_offset);
	printf("\n\n");
}

int main() {
	FILE* f = fopen("/app_home/file", "rb");
	print_file(f, 0);
	size_t file_size = 0;
    fseek( f, 0, SEEK_END );
	print_file(f, 1);
    file_size = ftell( f );
    rewind( f );
	print_file(f, 2);

	printf("size: %d\n", file_size);

	char buf[100];
	printf("pos: %d\n", ftell(f));
	fread(buf, 2, 2, f);
	print_file(f, 3);
	printf("pos: %d\n", ftell(f));
	
	int readbytes = fread(buf, 1, file_size, f);
	print_file(f, 4);
	printf("readbytes: %d\n", readbytes);
	buf[10] = 0;
	printf("content(10): %s\n", buf);
	rewind(f);
	print_file(f, 5);

	readbytes = fread(buf, file_size, 1, f);
	print_file(f, 6);
	printf("readbytes: %d\n", readbytes);
	buf[10] = 0;
	printf("content(10): %s\n", buf);
}