/*
 * Mailattach - a program to remove attachments and replace them with links.
 * Licensed under the Apache 2.0 License. Parts taken from the sendmail 
 * libmilter sample. License restrictions from their license may apply.
 */

#include <stdio.h>
#include <stdlib.h>


#include "network.h"
#include "config.h"


int main(int argc, char* argv[]){
	
	printf("INIT\n");
	
	if(init_net() < 0){
		return EXIT_FAILURE;
	}



}
