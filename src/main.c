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

#include <libmilter/mfapi.h>
#include <libmilter/mfdef.h>

#include "version.h"
#include "config.h"
#include "attach.h"

struct mlfiPriv
{
	char	*mlfi_fname;
	FILE	*mlfi_fp;
};

#define MLFIPRIV	((struct mlfiPriv *) smfi_getpriv(ctx))

static unsigned long mta_caps = 0;

sfsistat mlfi_cleanup(SMFICTX* ctx, bool ok) {
	sfsistat rstat = SMFIS_CONTINUE;
	struct mlfiPriv *priv = MLFIPRIV;
	
	if (priv == NULL){
		/* Something weired mus have happened... Maybe a crash? */
		return rstat;
	}

	if (ok){

		/* add a header to the message announcing our presence */
		smfi_addheader(ctx, "X-Mail-Attached", VERSION);
		
		/* replace body if attachements have been found */

		char* new_body = attach_files(priv->mlfi_fp);
		if(new_body != NULL){
			const char* head = "REPLACED:";
			size_t new_bodylen = strlen(new_body)+10;
			unsigned char * replacement = malloc(new_bodylen+1);
			memset(replacement, 0, new_bodylen+1);
			strcat((char*)replacement, head);
			strcat((char*)replacement, new_body);
			free(new_body);
			printf("Replacing body of mail message with len %lu\n", 
                                  strlen((char*)replacement));
			printf("Content: [%s]\n",replacement);
			if(smfi_replacebody(ctx, replacement, 
				strlen((char*)replacement)) == 
				MI_FAILURE){
				printf("Failed to replace body of message...\n"
					);
				free(replacement);
			}
		}
		
	}
	/* close the archive file */
	if (priv->mlfi_fp != NULL && fclose(priv->mlfi_fp) == EOF){
		/* failed; we have to wait until later */
		rstat = SMFIS_TEMPFAIL;
		(void) unlink(priv->mlfi_fname);
	}
	/* In any case release the temporary data storage file */
	(void) unlink(priv->mlfi_fname);


	/* release private memory */
	free(priv->mlfi_fname);
	free(priv);
	smfi_setpriv(ctx, NULL);

	return rstat;
}


sfsistat mlfi_envfrom(__attribute__((unused)) SMFICTX *ctx,
	 __attribute__((unused)) char** envfrom) {

struct mlfiPriv *priv;
	int fd = -1;

	/* allocate some private memory */
	priv = malloc(sizeof *priv);
	if (priv == NULL)
	{
		/* can't accept this message right now, memory has run out */
		return SMFIS_TEMPFAIL;
	}
	memset(priv, '\0', sizeof *priv);

	/* open a file to store this message */
	priv->mlfi_fname = strdup("/tmp/msg.XXXXXXXX");
	if (priv->mlfi_fname == NULL){

		free(priv);
		return SMFIS_TEMPFAIL;
	}

	if ((fd = mkstemp(priv->mlfi_fname)) < 0 ||
	    (priv->mlfi_fp = fdopen(fd, "w+")) == NULL) {
		if (fd >= 0){
			(void) close(fd);
		}
		free(priv->mlfi_fname);
		free(priv);
		return SMFIS_TEMPFAIL;
	}
	printf("Storing temp message to %s\n", priv->mlfi_fname);

	/* save the private data */
	smfi_setpriv(ctx, priv);

	/* continue processing */
	return SMFIS_CONTINUE;
}

sfsistat mlfi_header(__attribute__((unused)) SMFICTX *ctx, 
	__attribute__((unused)) char * headerf, 
	__attribute__((unused))char * headerv){

	/* continue processing */
	return ((mta_caps & SMFIP_NR_HDR) != 0)
		? SMFIS_NOREPLY : SMFIS_CONTINUE;
}


sfsistat mlfi_body(SMFICTX* ctx, unsigned char * bodyp, size_t bodylen) {

	/* output body block to log file */
	if (fwrite(bodyp, bodylen, 1, MLFIPRIV->mlfi_fp) <= 0){
		perror("Failed to write body to file...");
		
		(void) mlfi_cleanup(ctx, false);
		return SMFIS_TEMPFAIL;
	}
	/* continue processing */
	return SMFIS_CONTINUE;
}

sfsistat mlfi_eom(ctx)
	SMFICTX *ctx;
{
	return mlfi_cleanup(ctx, true);
}

sfsistat mlfi_abort(ctx)
	SMFICTX *ctx;
{
	return mlfi_cleanup(ctx, false);
}

sfsistat mlfi_negotiate(__attribute__((unused)) SMFICTX* ctx, 
	__attribute__((unused)) unsigned long f0, unsigned long f1, 
	__attribute__((unused)) unsigned long f2, 
	__attribute__((unused)) unsigned long f3, unsigned long* pf0, 
	unsigned long*  pf1, unsigned long* pf2, unsigned long* pf3)
{
	/* milter actions: add headers */
	*pf0 = SMFIF_ADDHDRS;

	/* milter protocol steps: all but connect, HELO, RCPT */
	*pf1 = SMFIP_NOCONNECT|SMFIP_NOHELO|SMFIP_NORCPT;
	mta_caps = f1;
	if ((mta_caps & SMFIP_NR_HDR) != 0)
		*pf1 |= SMFIP_NR_HDR;
	*pf2 = 0;
	*pf3 = 0;
	return SMFIS_CONTINUE;
}

struct smfiDesc smfilter =
{
	"mailattach",	/* filter name */
	SMFI_VERSION,	/* version code -- do not change */
	SMFIF_ADDHDRS|SMFIF_CHGFROM|SMFIF_ADDRCPT|SMFIF_DELRCPT|0b01|SMFIF_NONE,	/* flags */
	NULL,		/* connection info filter */
	NULL,		/* SMTP HELO command filter */
	mlfi_envfrom,	/* envelope sender filter */
	NULL,		/* envelope recipient filter */
	mlfi_header,	/* header filter */
	NULL,		/* end of header */
	mlfi_body,	/* body block filter */
	mlfi_eom,	/* end of message */
	mlfi_abort,	/* message aborted */
	NULL,		/* connection cleanup */
	NULL,		/* unknown/unimplemented SMTP commands */
	NULL,		/* DATA command filter */
	mlfi_negotiate	/* option negotiation at connection startup */
};

/* The actual entry point for the program. Performs initialisation routines and
 * then hands over to the libmilter main function */
int main(){
	
	printf("INIT\n");
	
	const char* conn_prefix = "local:";
	size_t conn_len = strlen(socket_location)+strlen(conn_prefix)+1;
	char * conn_str = malloc(conn_len);
	memset(conn_str, 0, conn_len);

	strcat(conn_str, conn_prefix);
	strcat(conn_str, socket_location);
	
	printf("Using socket for milter communication at [%s]\n", 
		conn_str);
	
	smfi_setconn(conn_str);

	if (smfi_register(smfilter) == MI_FAILURE)
	{
		fprintf(stderr, "smfi_register failed\n");
		return EXIT_FAILURE;
	}
	
	if(smfi_opensocket(true) == MI_FAILURE){
		fprintf(stderr, "smfi_opensocket failed at location [%s]\n",
			socket_location);
		return EXIT_FAILURE;
	}
	if(chmod(socket_location,0x1FF) < 0){
		perror("Failed to change socket permissions");

	}

	printf("READY, handing over to libmilter\n");
	return smfi_main();
}
