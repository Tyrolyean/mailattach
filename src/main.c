/*
 * Mailattach - a program to remove attachments and replace them with links.
 * Licensed under the Apache 2.0 License. Parts taken from the sendmail 
 * libmilter sample. License restrictions from their license may apply.
 */


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "version.h"
#include "config.h"
#include "attach.h"

int main(int argc, char* argv[]){
	
	FILE* logfile = fopen("/tmp/shit", "a+");
	
	fprintf(logfile, "INIT\n");
	fprintf(logfile, "Arguments passed on:\n");
	for(size_t i = 0; i < argc; i++){
		fprintf(logfile, "\t%lu:%s\n",i, argv[i]);
	}

	fclose(logfile);


	return EX_TEMPFAIL;

}
