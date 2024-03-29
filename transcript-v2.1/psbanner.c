#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Id: psbanner.c,v 1.6 1999-01-22 23:11:27 ghudson Exp $";
#endif

/* psbanner.c
 *
 * Copyright (C) 1985,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * 4.2BSD lpr/lpd banner break page filter for PostScript printers
 * (formerly "psof" in TranScript release 1.0)
 *
 * psbanner is used ONLY to print the "break page" --
 * the job seperator/banner page; the "if" filter
 * which invokes (pscomm) MUST be in
 * the printcap database for handling communications.
 *
 * This filter does not actually print anything; it sends
 * nothing to the printer (stdout).  Instead, it writes a
 * file in the current working directory (which is the
 * spooling directory) called ".banner" which, depending
 * on the value of printer options BANNERFIRST and
 * BANNERLAST, may get printed by pscomm filter when
 * it is envoked.
 *
 * psbanner parses the standard input (the SHORT banner string)
 * into PostScript format as the argument to a "Banner"
 * PostScript procedure (defined in $BANNERPRO, from the environment)
 * which will set the job name on the printer and print a
 * banner-style job break page.
 *
 * NOTE that quoting characters into PostScript form is
 * necessary.
 *
 * psbanner gets called with:
 *	stdin	== the short banner string to print (see below)
 *	stdout	== the printer (not used here)
 *	stderr	== the printer log file
 *	argv	== (empty)
 *	cwd	== the spool directory
 *	env	== VERBOSELOG, BANNERPRO
 *
 * An example of the "short banner string" input is:
adobe:shore  Job: test.data  Date: Tue Sep 18 16:22:33 1984
^Y^A
 * where the ^Y^A are actual control characters signalling
 * the end of the banner input and the time to do SIGSTOP
 * ourselves so that the controlling lpd will
 * invoke the next filter. (N.B. this is, regretably, NOT
 * documented in any of the lpr/printcap/spooler manual pages.)
 *
 *
 * I have decided to let the PostScript side try to parse the
 * string if possible, since it's easier to change the PS code
 * then to do so in C.  If you don't like the way the
 * banner page looks, change the BANNERPRO file to
 * do something else.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "transcript.h"
#include "psspool.h"

#ifndef BPRO_ENV
#define BPRO_ENV "BANNERPRO"
#endif

#ifdef BDEBUG
#define debugp(x) fprintf x; fflush(stderr);
#else
#define debugp(x)
#endif BDEBUG

private char	*prog;	/* invoking program name (i.e., psbanner) */
private char	*pname;	/* intended printer ? */

#define BANBUFSIZE 500     /* Size of banner string input buffer */
private char	bannerbuf[BANBUFSIZE];	/* PS'd short banner string */
private int     bannerfull;     /* TRUE when bannerbuf is full */
private char	*verboselog;	/* do log file reporting (from env) */
private char	*bannerpro;	/* prolog file name (from env) */
private int	VerboseLog;
private int     dobanner;       /* True = We are printing banners */

private VOID	on_int();

main(argc,argv)
	int argc;
	char *argv[];
{
    register int c;
    register char *bp;
    register FILE *in = stdin;


    int done = 0;
    char  *p;
#if defined(POSIX) && !defined(ultrix)
    struct sigaction sa;
    (void) sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = on_int;
    (void) sigaction(SIGINT, &sa, (struct sigaction *)0);
#else
    VOIDC signal(SIGINT, on_int);
#endif

    VOIDC fclose(stdout); /* we don't talk to the printer */
    prog = *argv; /* argv[0] == program name */
    pname = *(++argv);

    VerboseLog = 1;
    if (verboselog = envget("VERBOSELOG")) {
	VerboseLog = atoi(verboselog);
    }

    dobanner = 0;   /* Assume we will not be doing banners */
    p = envget("BANNERFIRST");
    if( p != NULL && atoi(p) != 0 ) dobanner = 1;
    p = envget("BANNERLAST");
    if( p != NULL && atoi(p) != 0 ) dobanner = 1;

    bannerpro = envget(BPRO_ENV);

    bp = bannerbuf;
    bannerfull = 0;
    while (!done) {
	switch (c = getc (in)) {
	    case EOF: /* this doesn't seem to happen */
		done = 1;
		break;
	    case '\n':
	    case '\f':
	    case '\r': 
		break;
	    case '\t': 
		if (!bannerfull) *bp++ = ' ';
		break;
	    case '\\': case '(': case ')': /* quote chars for POSTSCRIPT */
	        if (!bannerfull) {
		    *bp++ = '\\';
		    *bp++ = c;
		}
		break;
	    case '\031': 
		if ((c = getc (in)) == '\1') {
		    *bp = '\0';
		    /* done, ship the banner line */
		    if (bp != bannerbuf) {
			if(dobanner) DoBanner();  /* Do it if we will use it */
			if (verboselog) {
			    fprintf(stderr, "%s: %s\n", prog, bannerbuf);
			    VOIDC fflush(stderr);
			}
		    }
		    VOIDC kill (getpid (), SIGSTOP);
		    /* do we get continued for next job ? */
		    debugp((stderr,"%s: continued %d\n",
		    	prog,bp == bannerbuf));
		    bp = bannerbuf;
		    *bp = '\0';
		    bannerfull = 0;
		    break;
		}
		else {
		    VOIDC ungetc(c, in);
		    if (!bannerfull) *bp++ = ' ';
		    break;
		}
	    default: 		/* simple text */
		if (!bannerfull) *bp++ = c;
		break;
	}
	if (!bannerfull && bp >= &bannerbuf[BANBUFSIZE-3]) {
	    *bp++ = '\n'; *bp++ = '\0';
	    fprintf (stderr,"%s: banner buffer overflow\n",prog);
	    VOIDC fflush(stderr);
	    bannerfull = 1;
	}
    }
 /* didn't expect to get here, just exit */
 debugp((stderr,"%s: done\n",prog));
 VOIDC unlink(".banner");
 exit (0);
}

#define	STRSIZE	128

private DoBanner() {
    register FILE *out;
    char user[9];
    char host[STRSIZE];
    char jname[STRSIZE];
    char command[256];
    char *date;
    register char *cp;
    register int i;

    memset(user, 0, 9);
    memset(host, 0, STRSIZE);
    memset(jname, 0, STRSIZE);

    for (i = 0, cp = bannerbuf; (i < STRSIZE) && (*cp != ':'); i++, cp++)
	host[i] = *cp;
    host[STRSIZE - 1] = '\0';

    if (i == STRSIZE)
	goto dooutput;

    cp++;				/* past the colon */
    for (i = 0; i < 8 && !isspace(*cp); i++, cp++)
	user[i] = *cp;

    while (isspace(*cp))		/* get to whitespace (end of name) */
	cp++;
    while (isspace(*cp))		/* get past whitespace */
	cp++;
    while (!isspace(*cp))		/* get to whitespace (past Job:)*/
	cp++;
    while (isspace(*cp))		/* get past whitespace */
	cp++;
    /* now we are looking at the job name */
    for (i = 0; i < STRSIZE && !isspace(*cp); i++, cp++)
	jname[i] = *cp;
    jname[STRSIZE - 1] = '\0';

    if (i == STRSIZE)
	goto dooutput;

    while (isspace(*cp))		/* get past whitespace */
	cp++;
    while (!isspace(*cp))		/* get to whitespace (past Date:)*/
	cp++;
    while (isspace(*cp))		/* get past whitespace */
	cp++;

    date = cp;
    
dooutput:
    if ((out = fopen(".banner","w")) == NULL) {
	fprintf(stderr,"%s: can't open .banner", prog);
	VOIDC fflush(stderr);
	exit (THROW_AWAY);
    }
    if (verboselog) {
	fprintf(stderr, "%s: %s\n", prog, bannerbuf);
	VOIDC fflush(stderr);
    }
    if (copyfile(bannerpro,out)) {
	/* error copying file, don't print a break page */
	fprintf(stderr,"%s: trouble writing .banner\n",prog);
	VOIDC fflush(stderr);
	VOIDC unlink(".banner");
	return;
    }
    /* order of fields is:
       note	(unused)
       client	(host:user)
       jfname	(filename)
       filespec (filename path)
       account	(username)
       submitdate (date)
       printq 	(queue printed on) (printer name)

       */
/*    fprintf(out, "()(%s)(%s)(%s)(%s@%s)(%s)(%s) do_flagpage end\n",
	    user,
	    jname,
	    jname,
	    user,
	    host,
	    date,
	    pname);  */
    fprintf(out, "(%s)(%s)Banner\n", pname, bannerbuf); 
    VOIDC fclose(out);	/* this does a flush */

}

private VOID on_int() {
    /* NOTE: Let pscomm unlink banner -- it may want to use it */
    exit (THROW_AWAY);
}
