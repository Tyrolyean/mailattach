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
#include <stdbool.h>
#include <sys/stat.h>

#include <libmilter/mfapi.h>
#include <libmilter/mfdef.h>

#include "version.h"
#include "config.h"
#include "detect.h"

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

	/* close the archive file */
	if (priv->mlfi_fp != NULL && fclose(priv->mlfi_fp) == EOF){
		/* failed; we have to wait until later */
		rstat = SMFIS_TEMPFAIL;
		(void) unlink(priv->mlfi_fname);
	} else if (ok){

		/* add a header to the message announcing our presence */
		smfi_addheader(ctx, "X-Mail-Attached", VERSION);
	}

	/* In any case release the temporary data storage file */
	(void) unlink(priv->mlfi_fname);


	/* release private memory */
	free(priv->mlfi_fname);
	free(priv);
	smfi_setpriv(ctx, NULL);

	return rstat;
}


sfsistat mlfi_envfrom(SMFICTX *ctx, char** envfrom) {
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

sfsistat mlfi_header(SMFICTX *ctx, char * headerf,char * headerv)
{

	/* continue processing */
	return ((mta_caps & SMFIP_NR_HDR) != 0)
		? SMFIS_NOREPLY : SMFIS_CONTINUE;
}

sfsistat mlfi_eoh(SMFICTX *ctx) {

	/* continue processing */
	return SMFIS_CONTINUE;
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

sfsistat mlfi_close(ctx)
	SMFICTX *ctx;
{
	return SMFIS_ACCEPT;
}

sfsistat mlfi_abort(ctx)
	SMFICTX *ctx;
{
	return mlfi_cleanup(ctx, false);
}

sfsistat mlfi_unknown(ctx, cmd)
	SMFICTX *ctx;
	char *cmd;
{
	return SMFIS_CONTINUE;
}

sfsistat mlfi_data(ctx)
	SMFICTX *ctx;
{
	return SMFIS_CONTINUE;
}

sfsistat mlfi_negotiate(ctx, f0, f1, f2, f3, pf0, pf1, pf2, pf3)
	SMFICTX *ctx;
	unsigned long f0;
	unsigned long f1;
	unsigned long f2;
	unsigned long f3;
	unsigned long *pf0;
	unsigned long *pf1;
	unsigned long *pf2;
	unsigned long *pf3;
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
	SMFIF_ADDHDRS,	/* flags */
	NULL,		/* connection info filter */
	NULL,		/* SMTP HELO command filter */
	mlfi_envfrom,	/* envelope sender filter */
	NULL,		/* envelope recipient filter */
	mlfi_header,	/* header filter */
	mlfi_eoh,	/* end of header */
	mlfi_body,	/* body block filter */
	mlfi_eom,	/* end of message */
	mlfi_abort,	/* message aborted */
	mlfi_close,	/* connection cleanup */
	mlfi_unknown,	/* unknown/unimplemented SMTP commands */
	mlfi_data,	/* DATA command filter */
	mlfi_negotiate	/* option negotiation at connection startup */
};

/* The actual entry point for the program. Performs initialisation routines and
 * then hands over to the libmilter main function */
int main(){
	
	printf("INIT\n");
	printf("Using socket for milter communication at [%s]\n", 
		socket_location);
	
	
	smfi_setconn("local:/var/run/mailattach");

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
