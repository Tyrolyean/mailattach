/*
 * Mailattach - a program to remove attachments and replace them with links.
 * Licensed under the Apache 2.0 License. Parts taken from the sendmail 
 * libmilter sample. License restrictions from their license may apply.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "network.h"
#include "config.h"


int main(int argc, char* argv[]){
	
	printf("INIT\n");

	int c;

	while (1){
		static struct option long_options[] =
		{
			{"abort-pgp",	no_argument, &abort_on_pgp, 1},
			{"abort-dkim",	no_argument, &abort_on_dkim,1},
			{"noabort-pgp",	no_argument, &abort_on_pgp, 0},
			{"noabort-dkim",no_argument, &abort_on_dkim,0},
			{"verbose",	no_argument, &verbose,      1},
			{"quiet",	no_argument, &verbose,      0},
			{"only-base64",	no_argument, &only_base64,  1},
			{"other-base64",no_argument, &only_base64,  0},
			{"in-port",	required_argument, 0, 'i'},
			{"out-port",	required_argument, 0, 'o'},
			{"instance-id",	required_argument, 0, 'n'},
			{"directory",	required_argument, 0, 'd'},
			{"minfilesize",	required_argument, 0, 's'},
			{"url",		required_argument, 0, 'u'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "n:i:o:d:u:s:",
			       long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1){
			break;
		}

		switch (c){
		case 0:
			break;
		case 'i':
			listen_port = atoi(optarg);
			break;

		case 'o':
			forward_port = atoi(optarg);
			break;
			
		case 'n':
			instance_id = optarg;
			break;
		
		case 'd':
			directory = optarg;
			break;
		case 'u':
			url_base = optarg;
			break;
		case 's':
			min_filesize = atol(optarg);
			break;

		case '?':
			/* getopt_long already printed an error message. */
			return EXIT_FAILURE;
			break;

		default:
			abort ();
		}
	}

	/* If not already specified, populate the instance ID with the sytem 
	 * hostname.
	 */
	if(instance_id == NULL){
		instance_id = malloc(HOST_NAME_MAX + 1);
		memset(instance_id, 0, HOST_NAME_MAX + 1);
		if(gethostname(instance_id, HOST_NAME_MAX) < 0){
			perror("gethostname failed! set instance id manually");
		}
	}

	if(directory == NULL){
		fprintf(stderr, "directory option MUST be set!\n");
		return EXIT_FAILURE;
	}
	
	if(url_base == NULL){
		fprintf(stderr, "url option MUST be set!\n");
		return EXIT_FAILURE;
	}

	if(verbose){

		printf("Incoming port: %u outgoing port: %u on loopback "
			"interface\n",  listen_port, forward_port);

		printf("Aborting on PGP signed/encrypted messages: %s\n",
			abort_on_pgp ? "true" : "false");

		printf("Aborting on DKIM signed messages: %s\n",
			abort_on_dkim ? "true" : "false");
		
		printf("Instance id for messages: %s\n",
			instance_id);
		
		printf("Only saving bas64 encoded files: %s\n",
			only_base64 ? "true":"false");
		
		printf("Min filesize to store messages: %liB\n",
			min_filesize);

		
		printf("Placing files into [%s] linked by [%s]\n", directory,
		url_base);
	}

	if(init_net() < 0){
		return EXIT_FAILURE;
	}

	loop_clients();
	return EXIT_SUCCESS;

}
