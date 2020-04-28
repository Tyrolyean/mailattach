/*
 * Mailattach - a program to remove attachments and replace them with links.
 * Licensed under the Apache 2.0 License. Parts taken from the sendmail 
 * libmilter sample. License restrictions from their license may apply.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

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
			{"in-port",	required_argument, 0, 'i'},
			{"out-port",	required_argument, 0, 'o'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "i:o:pd",
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

		case '?':
			/* getopt_long already printed an error message. */
			return EXIT_FAILURE;
			break;

		default:
			abort ();
		}
	}

	printf("Incoming port: %u outgoing port: %u on loopback interface\n",
		 listen_port, forward_port);

	printf("Ignoring PGP signed/encrypted messages: %s\n",
		abort_on_pgp ? "true":false);

	printf("Ignoring DKIM signed messages: %s\n",
		abort_on_dkim ? "true" : "false");

	if(init_net() < 0){
		return EXIT_FAILURE;
	}

	loop_clients();
	return EXIT_SUCCESS;

}
