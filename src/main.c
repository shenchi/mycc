#include <stdlib.h>
#include <stdio.h>
#include "string.h"
#include "symbol.h"
#include "lexer.h"
#include "parser.h"
#include "memmgr.h"
#include "optimizer.h"
#include "asmgen.h"

char cmdline[256];
char fnamebuf[256];
char *inname;
char *outname;

char *makeoutname(char *inname){
	char *outname;
	int i, j = -1;
	for(i = 0; i < ccstrlen(inname); i++)
		if(inname[i] == '.')j = i;

	if(j >= 0){
		inname[j] = 0;
	}
	outname = ccstrcpy(inname);
	outname = ccstrcat(outname, '.');
	outname = ccstrcat(outname, 'a');
	outname = ccstrcat(outname, 's');
	outname = ccstrcat(outname, 'm');
	return outname;
}

int main(int argc, char* argv[]){

    if(argc < 2){
		printf("input file:");
		scanf("%s", fnamebuf);
		inname = fnamebuf;
    }else
		inname = argv[1];

    printf("compiling ... ");

    mem_init();
    lexer_load(inname);
    parser_init();
    parser_start();
	
	// for debug
	optimizer_init();
	optimizer_dag();
	optimizer_finish();
	//ERROR_STATUS = 1;
	// ================

	if(!ERROR_STATUS){
		outname = makeoutname(inname);
		generate(outname);
	}

    lexer_free();

	if(!ERROR_STATUS){
		printf("done.\n");
	
		/*
		
		// for linux

		sprintf(cmdline, "nasm -felf32 %s", outname);
		printf("%s\n", cmdline);
		system(cmdline);
		outname[ccstrlen(outname)-3] = 'o';
		outname[ccstrlen(outname)-2] = 0;

		sprintf(cmdline, "gcc %s mycclib.o", outname);
		printf("%s\n", cmdline);
		system(cmdline);
		*/

		// for windows

	}

    mem_free();

	system("pause");
    return 0;
}
