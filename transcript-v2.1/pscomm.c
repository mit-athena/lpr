#ifndef lint
#define _NOTICE static char
_NOTICE N1[] = "Copyright (c) 1985,1986,1987 Adobe Systems Incorporated";
_NOTICE N2[] = "GOVERNMENT END USERS: See Notice file in TranScript library directory";
_NOTICE N3[] = "-- probably /usr/lib/ps/Notice";
_NOTICE RCSID[]="$Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/transcript-v2.1/pscomm.c,v 1.3 1990-04-16 17:07:58 epeisach Exp $";
#endif
/* pscomm.c
 *
 * Copyright (C) 1985,1986,1987 Adobe Systems Incorporated. All rights reserved.
 * GOVERNMENT END USERS: See Notice file in TranScript library directory
 * -- probably /usr/lib/ps/Notice
 *
 * 4.2BSD lpr/lpd communications filter for PostScript printers
 * (formerly "psif" in TranScript release 1.0)
 *
 * pscomm is the general communications filter for sending files to
 * a PostScript printer via RS232 lines.  It does page accounting,
 * error handling/reporting, job logging, banner page printing, etc.
 * It observes (parts of) the PostScript file structuring conventions.
 * In particular, it distinguishes between PostScript files (beginning
 * with the "%!" magic number) -- which are shipped to the printer --
 * and text files (no magic number) which are formatted and listed
 * on the printer.  Files which begin with "%!PS-Adobe-" may be
 * page-reversed if the target printer has that option specified.
 *
 * depending on the values of BANNERFIRST and BANNERLAST, 
 * pscomm looks for a file named ".banner", (created by the "of" filter)
 * in the current working directory and ships it to the printer also.
 *
 * pscomm gets called with:
 *	stdin	== the file to print (may be a pipe!)
 *	stdout	== the printer
 *	stderr	== the printer log file
 *	cwd	== the spool directory
 *	argv	== set up by interface shell script:
 *	  filtername	-P printer
 *			-p filtername
 *			[-r]		(don't ever reverse)
 *			-n login
 *			-h host
 *                      -a account
 *                      -m mediacost
 *			[accntfile]
 *
 *	environ	== various environment variable effect behavior
 *		VERBOSELOG	- do verbose log file output
 *		BANNERFIRST	- print .banner before job
 *		BANNERLAST	- print .banner after job
 *		REVERSE		- page reversal filter program
 *				  (no reversal if null or missing)
 *		PSLIBDIR	- transcript library directory
 *		PSTEXT		- simple text formatting filter
 *		JOBOUTPUT	- file for actual printer stream
 *				  output (if defined)
 *
 * General flow of control for communications:
 * 1) "Sync" with printer: Send small accounting job, and make sure reply
 *    matches what we sent.  If not a match, sleep a little and try again.
 * 2) Send user job: Start listener process, and write user job to printer.
 *    Banner may get sent first. CTRL-T gets sent occasionally. Listener
 *    get ALL output from printer.  If listener sees "idle" status message
 *    it aborts, which flushes the job (printer rebooted). End-of-job character
 *    is written to printer after job has been sent.
 * 3) Wait for user job to complete: Listener process exits when end-of-job
 *    character is received from printer.  "idle" status will make job
 *    abort as in (2) above.
 * 4) If doing accounting or "bannerlast", use accounting job to "sync"
 *    printer again.  A banner may be sent before the accounting job.
 * Note that only output from the user job is read by the listener process.
 * Output from the banner, for instance, is ignored (there shouldn't be any).
 * Also, note that jobs containing or printing end-of-job characters will
 * may print correctly -- the "sync" process guarantees the next job will
 * be executed correctly, however.
 *
 * pscomm depends on certain additional features of the 4.2BSD spooling
 * architecture.  In particular it assumes that the printer status file
 * has the default name (./status) and it uses this file to communicate
 * printer error status information to the user -- the contents of the
 * status file gets incorporated in "lpq" and "lpc status" messages.
 *
 * RCSLOG:
 * $Log: not supported by cvs2svn $
 * Revision 1.2  89/05/22  10:58:39  epeisach
 * Fixed RT problem in dealing with '\]' which vax does not complain about.
 * 
 * Revision 1.1  89/05/22  10:51:24  epeisach
 * Initial revision
 * 
 * Revision 2.2  87/11/17  16:51:17  byron
 * Release 2.1
 * 
 * Revision 2.1.1.19  87/11/12  13:41:02  byron
 * Changed Government user's notice.
 * 
 * Revision 2.1.1.18  87/09/30  16:52:36  byron
 * 1. Added some info to the "printer sync" error message.
 * 2. Changed the timeout parsing not to recognize "PrinterError: timeout".
 * 
 * Revision 2.1.1.17  87/09/22  11:04:37  byron
 * Added check for "printer:" in status, which is like "PrinterError:".
 * 
 * Revision 2.1.1.16  87/09/16  11:16:25  byron
 * 1. Added check for printer timeout, which aborts job and tries again.
 * 2. Added warning if CTRL-D is found while job is still being sent.
 * 
 * Revision 2.1.1.15  87/09/09  10:10:22  byron
 * Added blank-skipping to status close string, so that %%[ ... ] %% works.
 * 
 * Revision 2.1.1.14  87/07/24  16:08:25  byron
 * Added some error checking that was developed while debugging pscomm.sysv.
 * 
 * Revision 2.1.1.13  87/07/15  16:27:33  byron
 * Changed "unexpected EOF" message when child gets EOF from printer.
 * 
 * Revision 2.1.1.12  87/07/01  11:41:04  byron
 * Changed Bridge code to flush I/O buffers in resetprt().
 * Also, added more code to give up when job is aborted and things look
 * confused.  Added errno declaration for portability.
 * 
 * Revision 2.1.1.11  87/06/25  15:58:45  byron
 * Bug in page reverser process critical region.  Would have caused pscomm
 * to hang if reverser got done before parent continued (and redid stdin).
 * 
 * Revision 2.1.1.10  87/06/24  11:48:49  byron
 * Added back some RestoreStatus calls.  This makes status correct when
 * an error happens in one file in a multi-file user job.
 * 
 * Revision 2.1.1.9  87/06/22  10:15:37  byron
 * 1. Changed when status message is restored -- don't do it so often.
 * 2. Fixed bugs where original value of errno was getting lost before it was
 * printed out.  Now save and restore errno around some system calls.
 * 
 * Revision 2.1.1.8  87/06/18  16:07:37  byron
 * Complete rearrangement of code (most algorithms the same). Goals:
 * 1. Make listener process simpler, and able to communicate better w/ parent.
 * 2. Make sure printer is replying to the messages we are sending.
 *    syncprinter() accomplishes this using accounting info.
 * 3. Eliminate race conditions, especially involving signal processing.
 *    Had multiple signal() calls -- now has single signal() call for each
 *    signal and state variable (intstate). Also added STARTCRIT and ENDCRIT.
 * 4. Explicitly kill existing children, and make sure they go away.
 * 5. Make goto's and setjmp/longjmp's more straightforward (if possible).
 * 
 * Revision 2.1.1.7  87/05/14  16:52:43  byron
 * Added support for BANNERLAST=2, which unlinks the .banner file after use.
 * This breaks multiple-copy operation, but make -h jobs work correctly.
 * 
 * Revision 2.1.1.6  87/04/23  10:26:10  byron
 * Copyright notice.
 * 
 * Revision 2.1.1.5  87/03/24  16:44:31  shore
 * fixed parsing of %%X messages back from printer, X was getting dropped
 * 
 * Revision 2.1.1.4  86/11/02  14:40:56  shore
 * fixed race around page reversal
 * (thanks to Ron Stanonik @ nprdc)
 * 
 * Revision 2.1.1.3  86/07/15  15:05:45  shore
 * fixed accounting/BannerLast end-of-job race
 * 
 * Revision 2.1.1.2  86/06/08  11:12:50  shore
 * fixed 11 character minimum magic restriction
 * 
 * Revision 2.1.1.1  86/03/25  13:33:13  shore
 * fixed lpr -p bug (when text input is a pipe)
 * fixed lprm bug when printer is off-line
 * 
 * Revision 2.1  85/11/24  11:50:16  shore
 * Product Release 2.0
 * 
 * Revision 1.1  85/11/20  00:35:21  shore
 * Initial revision
 * 
 * Revision 1.2  85/05/14  11:25:29  shore
 * better support for BANNERLAST, still buggy though
 * 
 *
 */

#include <ctype.h>
#include <setjmp.h>
#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>

#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef BRIDGE  /* Unsupported code for Bridge communications boxes */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif BRIDGE

#include "transcript.h"
#include "psspool.h"

#ifdef BDEBUG
#define debugp(x) {fprintf x ; (void) fflush(stderr);}
#else
#define debugp(x)
#endif BDEBUG

#ifdef SYSLOG
#include <syslog.h>
#endif
#ifdef ZEPHYR
#include <zephyr/zephyr.h>
#endif

/*
 * the following string is sent to the printer when we want it to
 * report its current pagecount (for accounting)
 */

private char *getpages =
"\n(%%%%[ pagecount: )print statusdict/pagecount get exec(                )cvs \
print(, %d %d ]%%%%)= flush\n%s";

private jmp_buf initlabel, synclabel, sendlabel, croaklabel;

private char	*prog;			/* invoking program name */
private char	*name;			/* user login name */
private char	*host;			/* host name */
private char	*pname;			/* printer name */
private char	*accountingfile;	/* file for printer accounting */
private char    *account = NULL;        /* account number - Ilham (2/20/90) */
private char    *mediacost = "0";	/* media cost     - Ilham (2/20/90 */
private int	doactng;		/* true if we can do accounting */
private int	progress, oldprogress;	/* finite progress counts */
private int	getstatus = FALSE;      /* TRUE = Query printer for status */
private int	newstatmsg = FALSE;     /* TRUE = We changed status message */
private int	childdone = FALSE;      /* TRUE = Listener process finished */
private int	jobaborted = FALSE;     /* TRUE = Aborting current job */
private long	startpagecount;         /* Page count at start of job */
private long	endpagecount;           /* Page count at end of job */
private long	starttime;              /* Timer start. For status warnings */
private int	saveerror;		/* Place to save errno when exiting */
private int	bannerpages = 0;	/* Count of banner pages printed */

private char *bannerfirst;
private char *bannerlast;
private char *verboselog;
private char *reverse;
private int BannerFirst;
private int BannerLast;
private int VerboseLog;
private int UnlinkBannerLast;   /* Unlink banner file after printing banner. */
				/* Only looked at if BannerLast=TRUE. */

/* Interrupt state variables */
typedef enum {                  /* Values for "die" interrupts, like SIGINT */
    init,                       /* Initialization */
    syncstart,                  /* Synchronize communications with printer */
    sending,                    /* Send info to printer */
    waiting,                    /* Waiting for listener to get EOF from prntr */
    lastpart,                   /* Final processing following user job */
    synclast,                   /* Syncronize communications at the end */
    ending,                     /* Cleaning up */
    croaking,			/* Abnormal exit, waiting for children to die */
    child                       /* Used ONLY for the child (listener) process */
    } dievals;
private dievals intstate;       /* State of interrupts */
private flagsig;                /* TRUE = On signal receipt, just set flag */
#define DIE_INT    1            /* Got a "die" interrupt. like SIGINT */
#define ALARM_INT  2            /* Got an alarm */
#define EMT_INT    4            /* Got a SIGEMT signal (child-parent comm.) */
private int gotsig;             /* Mask that may take any of the above values */

#define STARTCRIT() {gotsig=0; flagsig=TRUE;}  /* Start a critical region */
#define ENDCRIT()   {flagsig=FALSE;}           /* End a critical region */

/* WARNING: Make sure reapchildren() routine kills all processes we started */
private int	fpid = 0;	/* formatter pid */
private int	rpid = 0;	/* page reversal pid */
private int	cpid = 0;	/* listener pid */
private int	mpid = 0;	/* current process pid */
private int     wpid;		/* Temp pid */
private union wait status;	/* Return value from wait() */

private char abortbuf[] = "\003";	/* ^C abort */
private char statusbuf[] = "\024";	/* ^T status */
private char eofbuf[] = "\004";		/* ^D end of file */

/* global file descriptors (avoid stdio buffering!) */
private int fdsend;		/* to printer (from stdout) */
private int fdlisten;		/* from printer (same tty line) */
private int fdinput;		/* file to print (from stdin) */

private FILE *psin=NULL;        /* Buffered printer input */
private FILE *jobout;		/* special printer output log */

#ifdef BRIDGE
/* socket numbers here are for SUN with ntohs() to convert */
#define GENERIC_SOCKET	ntohs((u_short) 0x07D0)
#define NMUI_SOCKET	ntohs((u_short) 0x004D)
private int socklen;
struct sockaddr_in dest;
struct hostent *hp;
#endif BRIDGE

/* Return values from the NextCh() routine */
typedef enum {
    ChOk,			/* Everything is fine */
    ChIdle,			/* We got status="idle" from the printer */
    ChTimeout			/* The printer timed out */
    } ChStat;


extern char *getenv();
extern int errno;

private VOID    GotDieSig();
private VOID    GotAlarmSig();
private VOID    GotEmtSig();
private VOID    syncprinter();
private VOID    listenexit();
private VOID    closedown();
private VOID    myexit1(),myexit2();
private VOID    croak();
private VOID    acctentry();
private char 	*FindPattern();
private ChStat  NextCh();

/* The following are alarms settings for various states of this program */
#define SENDALARM 90		/* Status check while sending job to printer */
#define WAITALARM 90            /* Status check while waiting for job to end */
#define CROAKALARM 10           /* Waiting for child processes to die */
#define CHILDWAIT 60		/* Child waiting to die -- checking on parent */
#define ABORTALARM 90           /* Waiting for printer to respond to job abort */
#define SYNCALARM 30            /* Waiting for response to communications sync */
#define MAXSLEEP  15            /* Max time to sleep while sync'ing printer */

/* Exit values from the listener process */
#define LIS_NORMAL 0        /* No problems */
#define LIS_EOF    1        /* Listener got EOF on printer */
#define LIS_IDLE   2        /* Heard a "status: idle" from the printer */
#define LIS_DIE    3        /* Parent told listener to kill itself */
#define LIS_NOPARENT  4     /* Parent is no longer present */
#define LIS_ERROR  5        /* Unrecoverable error */
#define LIS_TIMEOUT 6       /* Printer timed out */



main(argc,argv)            /* MAIN ROUTINE */
	int argc;
	char *argv[];
{
    register char  *cp;
    register int cnt, wc;
    register char *mbp;

    char  **av;
    char magic[11];	/* first few bytes of stdin ?magic number and type */
    int  magiccnt;	/* actual number of magic bytes read */
    int  noReverse = 0; /* flag if we should never page reverse */
    int  canReverse = 0;/* flag if we can page-reverse the ps file */
    int  reversing = 0;
    FILE *streamin;

    int tmp;		/* fd for temporary when input is text pipe */
    struct stat sbuf;

    char mybuf[BUFSIZ];
    int fdpipe[2];
    int format = 0;
    int i;

    mpid = getpid();    /* Save the current process ID for later */

    /* initialize signal processing */
    flagsig = FALSE;           /* Process the signals */
    intstate = init;       /* We are initializing things now */
    VOIDC signal(SIGINT, GotDieSig);
    VOIDC signal(SIGHUP, GotDieSig);
    VOIDC signal(SIGTERM, GotDieSig);
    VOIDC signal(SIGALRM, GotAlarmSig);
    VOIDC signal(SIGEMT, GotEmtSig);

#ifdef SYSLOG
	openlog("lpcomm", LOG_PID, LOG_LPR);
#endif

    /* parse command-line arguments */
    /* the argv (see header comments) comes from the spooler daemon */
    /* itself, so it should be canonical, but at least one 4.2-based */
    /* system uses -nlogin -hhost (insead of -n login -h host) so I */
    /* check for both */

    av = argv;
    prog = *av;

    while (--argc) {
	if (*(cp = *++av) == '-') {
	    switch (*(cp + 1)) {
		case 'P':	/* printer name */
		    argc--;
		    pname = *(++av);
		    break;

		case 'n': 	/* user name */
		    argc--;
		    name = *(++av);
		    break;

		case 'h': 	/* host */
		    argc--;
		    host = *(++av);
		    break;

		case 'p':	/* prog */
		    argc--;
		    prog = *(++av);
		    break;

		case 'r':	/* never reverse */
		    argc--;
		    noReverse = 1;
		    break;

		case 'a':       /* account number */
		    argc--;
		    account = *(++av);
		    break;

		case 'm':       /* media cost */
		    argc--;
		    mediacost = *(++av);
		    break;

		default:	/* unknown */
		    fprintf(stderr,"%s: unknown option: %s\n",prog,cp);
		    break;
	    }
	}
	else
	    accountingfile = cp;
    }

    debugp((stderr,"args: %s %s %s %s\n",prog,host,name,accountingfile));

    /* do printer-specific options processing */

    VerboseLog = 1;
    BannerFirst = BannerLast = 0;
    UnlinkBannerLast = 0;
    reverse = NULL;
    if (bannerfirst=envget("BANNERFIRST")) {
	BannerFirst=atoi(bannerfirst);
    }
    if (bannerlast=envget("BANNERLAST")) {
	switch (atoi(bannerlast)) {
	    case 0:
	    default:
		BannerLast = UnlinkBannerLast = 0;
		break;
	    case 1:
		BannerLast = 1; UnlinkBannerLast = 0;  /* No unlink banner */
		break;
	    case 2:
		BannerLast = UnlinkBannerLast = 1;  /* Unlink banner file */
		break;
	    }
    }
    if (verboselog=envget("VERBOSELOG")) {
	VerboseLog=atoi(verboselog);
    }
    if (!noReverse) {
	reverse=envget("REVERSE");	/* name of the filter itself */
    }

    if (VerboseLog) {
	VOIDC time(&starttime);
	fprintf(stderr, "%s: %s:%s %s start - %s", prog, host, name, pname,
            ctime(&starttime));
	VOIDC fflush(stderr);
    }
    debugp((stderr,"%s: pid %d ppid %d\n",prog,getpid(),getppid()));
    debugp((stderr,"%s: options BF=%d BL=%d VL=%d R=%s\n",prog,BannerFirst,
    	BannerLast, VerboseLog, ((reverse == NULL) ? "norev": reverse)));

    /* IMPORTANT: in the case of cascaded filters, */ 
    /* stdin may be a pipe! (and hence we cannot seek!) */

    if ((magiccnt = read(fileno(stdin),magic,11)) <= 0) {
        fprintf(stderr,"%s: bad magic number, EOF\n", prog);
	VOIDC fflush(stderr);
	croak(THROW_AWAY);
	}
    debugp((stderr,"%s: magic number is (%11.11s)\n",prog,magic));
    streamin = stdin;

    if (strncmp(magic,"%!PS-Adobe-",11) == 0) {
	canReverse = TRUE;
    }
    else if (strncmp(magic,"%!",2) == 0) {
	canReverse = FALSE;
    }
    else {

	/* here is where you might test for other file type
	 * e.g., PRESS, imPRESS, DVI, Mac-generated, etc.
	 */

	/* final sanity check on the text file, to guard
	 * against arbitrary binary data
	 */

	for (i = 0; i < magiccnt; i++) {
	    if (!isascii(magic[i]) || (!isprint(magic[i]) && !isspace(magic[i]))){
		fprintf(stderr,"%s: Error: spooled binary file rejected\n",prog);
		VOIDC fflush(stderr);
		sprintf(mybuf,"%s/bogusmsg.ps",envget("PSLIBDIR"));
		if ((streamin = freopen(mybuf,"r",stdin)) == NULL) {
		    croak(THROW_AWAY);
		}
		format = 1;
		goto printit;
	    }
	}

	VOIDC fstat(fileno(stdin), &sbuf);
	if ((sbuf.st_mode & S_IFMT) != S_IFREG) {
	    debugp((stderr,"creating temporary\n"));
	    /* ack! input is a pipe! must copy into a temp file */
	    if ((tmp = open(".temp", O_WRONLY | O_CREAT, 0600)) < 0) {
		myexit2(prog,"creating .temp file",THROW_AWAY);
	    }
	    VOIDC write(tmp,magic,magiccnt);
	    while ((cnt = read(fileno(stdin), mybuf, sizeof mybuf)) > 0) {
		if (write(tmp, mybuf, cnt) != cnt) {
		    myexit2(prog,"copying .temp file",THROW_AWAY);
		}
	    }
	    if (cnt < 0) {
		myexit2(prog,"copying .temp file",THROW_AWAY);
	    }
	    VOIDC close(tmp);
	    if (freopen(".temp","r",stdin) == NULL) {
		myexit2(prog,"opening .temp file",THROW_AWAY);
	    }
	    VOIDC unlink(".temp");
	}

	/* exec dumb formatter to make a listing */
	debugp((stderr,"%s: formatting\n",prog));
	format = 1;
	lseek(0,0L,0);
	rewind(stdin);
	if (pipe (fdpipe)) myexit2(prog, "format pipe",THROW_AWAY);
	if ((fpid = fork()) < 0) myexit2(prog, "format fork",THROW_AWAY);
	if (fpid == 0) { /* child */
	    /* set up child stdout to feed parent stdin */
	    if (close(1) || (dup(fdpipe[1]) != 1)
	    || close(fdpipe[1]) || close(fdpipe[0])) {
		myexit2(prog, "format child",THROW_AWAY);
	    }
	    execl(envget("PSTEXT"), "pstext", pname, 0);
	    myexit2(prog,"format exec",THROW_AWAY);
	}
	/* parent continues */
	/* set up stdin to be pipe */
	if (close(0) || (dup(fdpipe[0]) != 0)
	|| close(fdpipe[0]) || close(fdpipe[1])) {
	    myexit2(prog, "format parent",THROW_AWAY);
	}

	/* fall through to spooler with new stdin */
	/* can't seek here but we should be at the right place */
	streamin = fdopen(0,"r");
	canReverse = TRUE; /* we know we can reverse pstext output */
    }


    /* do page reversal if specified */
    if (reversing = ((reverse != NULL) && canReverse)) {
	debugp((stderr,"%s: reversing\n",prog));
	if (pipe (fdpipe)) myexit2(prog, "reverse pipe", THROW_AWAY);
	STARTCRIT();        /* Make sure pipes get closed correctly */
	if ((rpid = fork()) < 0) myexit2(prog, "reverse fork", THROW_AWAY);
	if (rpid == 0) { /* child */
	    /* set up child stdout to feed parent stdin */
	    if (close(1) || (dup(fdpipe[1]) != 1)
	    || close(fdpipe[1]) || close(fdpipe[0])) {
		myexit2(prog, "reverse child", THROW_AWAY);
	    }
	    execl(reverse, "psrv", pname, 0);
	    myexit2(prog,"reverse exec",THROW_AWAY);
	}

	/* Parent continues.  Note the critical region here -- the stdin
	 * must get set back correctly before we accept the fact that
	 * the reverser process has finished.
	 */
	if (close(0) || (dup(fdpipe[0]) != 0)
	|| close(fdpipe[0]) || close(fdpipe[1])) {
	    myexit2(prog, "reverse parent", THROW_AWAY);
	}
	/* fall through to spooler with new stdin */
	streamin = fdopen(0,"r");

	if( !setjmp(initlabel) ) {    /* Do this FIRST time through */
	    ENDCRIT();        /* Make sure pipes get closed correctly */
	    if(!gotsig) pause();    /* Wait for reverser to start */
	    }
	if( gotsig & DIE_INT ) closedown();

	debugp((stderr,"%s: reverse feeding\n",prog));
    }

    printit:;

    fdinput = fileno(streamin); /* the file to print */

#ifdef BRIDGE
    /* open network connection to the printer */
    if (envget("NETNAME")) pname=envget("NETNAME");

    if ((hp = gethostbyname(pname)) == NULL) {
	myexit2(prog,"badhost",TRY_AGAIN);
    }
    bcopy(hp->h_addr, &dest.sin_addr.s_addr, hp->h_length);
    dest.sin_family = AF_INET;
    dest.sin_port = GENERIC_SOCKET;

    if ((fdsend = socket(AF_INET,SOCK_STREAM, 0)) < 0) {
	myexit2(prog,"opening socket",TRY_AGAIN);
    }
    socklen = sizeof(dest);
    if (connect(fdsend,&dest,socklen)) {
	saveerror = errno;
	sleep(30);
	errno = saveerror;
	myexit2(prog,"connecting",TRY_AGAIN);
    }
    if (getsockname(fdsend,&dest,&socklen)) {
	saveerror = errno;
	VOIDC close(fdsend);
	errno = saveerror;
	myexit2(prog,"getting socket name",TRY_AGAIN);
    }
    /* socket options processing */
    if (setsockopt(fdsend,SOL_SOCKET,SO_DONTLINGER,0,0) < 0) {
	perror("sockopt dontlinger");
    }
#else
    fdsend = fileno(stdout);	/* the printer (write) */
#endif BRIDGE

    doactng = name && accountingfile && (access(accountingfile, W_OK) == 0);

    /* get control of the "status" message file.
     * we copy the current one to ".status" so we can restore it
     * on exit (to be clean).
     * Our ability to use this is publicized nowhere in the
     * 4.2 lpr documentation, so things might go bad for us.
     * We will use it to report that printer errors condition
     * has been detected, and the printer should be checked.
     * Unfortunately, this notice may persist through
     * the end of the print job, but this is no big deal.
     */
    BackupStatus(".status","status");

    intstate = syncstart;   /* start communications */

    debugp((stderr,"%s: sync printer and get initial page count\n",prog));
    syncprinter(&startpagecount);     /* Make sure printer is listening */

    if(newstatmsg) RestoreStatus();   /* Put back old message -- we're OK now */

    STARTCRIT();		/* Child state change and setjmp() */
    intstate = sending;

    if ((cpid = fork()) < 0) myexit1(prog, THROW_AWAY);
    else if (cpid) {/* START PARENT -- SENDER */

	if( setjmp(sendlabel) ) goto donefile;
	ENDCRIT();		/* Child state change and setjmp() */
	if( gotsig & DIE_INT ) closedown();

	debugp((stderr,"%s: printer responding\n",prog));
	progress = oldprogress = 0; /* finite progress on sender */

	/* initial break page ? */
	if (BannerFirst) {
	    SendBanner();
	    progress++;
	    if (!BannerLast) VOIDC unlink(".banner");
	}

	/* ship the magic number! */
	if ((!format) && (!reversing)) {
	   VOIDC write(fdsend,magic,magiccnt);
	   progress++;
	}

	/* now ship the rest of the file */

	VOIDC alarm(SENDALARM); /* schedule an alarm */

	while ((cnt = read(fdinput, mybuf, sizeof mybuf)) > 0) {
	    if (getstatus) {       /* Get printer status sometimes */
		VOIDC write(fdsend, statusbuf, 1);
		getstatus = FALSE;
		progress++;
	    }
	    mbp = mybuf;
	    while ((cnt > 0) && ((wc = write(fdsend, mbp, cnt)) != cnt)) {
		if (wc < 0) {
		    fprintf(stderr,"%s: error writing to printer",prog);
		    perror("");
		    sleep(10);
		    croak(TRY_AGAIN);
		}
		mbp += wc;
		cnt -= wc;
		progress++;
	    }
	    progress++;
	}
	if (cnt < 0) {
	    fprintf(stderr,"%s: error reading from stdin", prog);
	    perror("");
	    sleep(10);
	    croak(TRY_AGAIN);
	}

	donefile:   /* Done sending the user's file */

	/* Send the PostScript end-of-job character */
	debugp((stderr,"%s: done sending\n",prog));
	STARTCRIT();		/* Only do end-of-job char once */
	VOIDC write(fdsend, eofbuf, 1);
	intstate = waiting;    /* Waiting for end of user job */
	ENDCRIT();		/* Only do end-of-job char once */
	if( gotsig & DIE_INT ) VOIDC kill( getpid(),SIGINT );

	VOIDC alarm(WAITALARM);
	while( !childdone ) pause();	/* Wait for listener to finish */
	VOIDC alarm(0);

	intstate = lastpart;
	if( BannerLast || doactng ) {
	    if (BannerLast) {              /* final banner page */
		SendBanner();
		if (UnlinkBannerLast) VOIDC unlink(".banner");
	    }
	    intstate = synclast;
	    syncprinter(&endpagecount);    /* Get communications in sync again */
	    intstate = ending;
	    if(doactng) VOIDC acctentry(startpagecount,endpagecount,
					account,mediacost);
	}

	intstate = ending;

	if (VerboseLog) {
	    VOIDC time(&starttime);
	    fprintf(stderr,"%s: end - %s",prog,ctime(&starttime));
	    VOIDC fflush(stderr);
	}
	RestoreStatus();
	exit(0);

    }  /* END PARENT -- SENDER */
    else {/* START CHILD -- LISTENER */
	/* This process listens while the user job is sent.
	 * It communicates with the parent by signalling a SIGEMT, then
	 * exiting.  The exit code is the only form of communication implemented.
	 * The user job is aborted: A) If we get a status of "idle" back from
	 * the printer (see NextCh() routine), or B) If we get EOF back
	 * while reading the printer (somebody unplugged the comm line?),
	 * or C) If we receive a SIGEMT signal (parent wants us to die).
	 * This process ignores most signals except SIGEMT.
	 *
	 * A signal is used before exiting to make it easy for the parent to
	 * use wait() to get the child status, and for portability to System V.
	 */
	register int r;
	register ChStat i;
	char *outname;	/* file name for job output */
	int havejobout = FALSE; /* flag if jobout != stderr */

	intstate = child;
	ENDCRIT();		/* Child state change and setjmp() */
	prog = "pslisten";      /* Change our name */

	/* get jobout from environment if there, otherwise use stderr */
	if (((outname = envget("JOBOUTPUT")) == NULL)
	|| ((jobout = fopen(outname,"w")) == NULL)) {
	  jobout = stderr;
	}
	else havejobout = TRUE;

	/* listen for the user job */
	NextChInit();
	while (TRUE) {
	    r = getc(psin);
	    if ((r&0377) == 004) break;     /* Printer says end of job */
	    else if (r == EOF) {            /* Printer comm line died? */
		if( feof(psin) ) {
		    VOIDC kill( getppid(),SIGEMT );   /* Tell parent we exit */
		    exit(LIS_EOF);
		    }
		else {
		    if( errno != EINTR ) {	/* Got a random error */
			fprintf(stderr,"%s: Error" );
			perror("");
			fflush(stderr);
			exit(LIS_ERROR);
			}
		    }
		clearerr(psin);		/* Make it so we can read again */
		continue;
		}
	    if( (i=NextCh(r)) != ChOk ) {
		switch( (int)i ) {
		    case ChIdle:
			VOIDC kill( getppid(),SIGEMT );
			exit(LIS_IDLE);
		    case ChTimeout:
			VOIDC kill( getppid(),SIGEMT );
			exit(LIS_TIMEOUT);
		    }
		}
	}

	debugp((stderr,"%s: listener saw eof, done listening\n",prog));
	if (havejobout) VOIDC fclose(jobout);
	VOIDC fclose(psin);
	VOIDC kill( getppid(),SIGEMT );   /* Tell parent we are exiting */
	exit(LIS_NORMAL);
    }    /* END CHILD -- LISTENER */
    /* Can't get here */
}

/* send the file ".banner" */
private SendBanner()
{
    register int banner;
    int cnt;
    char buf[BUFSIZ];

    if ((banner = open(".banner",O_RDONLY|O_NDELAY,0)) < 0) {
	debugp((stderr,"%s: No banner file\n",prog));
	return;
	}
    while ((cnt = read(banner,buf,sizeof buf)) > 0) {
	VOIDC write(fdsend,buf,cnt);
    }
    bannerpages++;
    VOIDC close(banner);
}

/* search backwards from p in start for patt */
private char *FindPattern(p, start, patt)
register char *p;
         char *start;
         char *patt;
{
    int patlen;
    register char c;

    patlen = strlen(patt);
    c = *patt;
    
    p -= patlen;
    for (; p >= start; p--) {
	if (c == *p && strncmp(p, patt, patlen) == 0) return(p);
    }
    return ((char *)NULL);
}

/* Static variables for NextCh() routine */
static char linebuf[BUFSIZ];
static char *cp;
static enum {normal, onep, twop, inmessage,
	     close1, close2, close3, close4} st;

/* Initialize the NextCh routine */
private NextChInit() {
    cp = linebuf;
    st = normal;
}

/* Overflowed the NextCh() buffer */
private NextChErr()
{
    *cp = '\0';
    fprintf(stderr,"%s: Status message too long: (%s)\n",prog,linebuf);
    VOIDC fflush(stderr);
    st = normal;
    cp = linebuf;
}

/* Put one character in the status line buffer. Called by NextCh() */
#define NextChChar(c) if( cp <= linebuf+BUFSIZ-2 ) *cp++ = c; else NextChErr();

/* Process a char from the printer.  This picks out and processes status
 * and PrinterError messages.  The printer status file is handled IN THIS
 * ROUTINE when status messages are encountered -- there is no way to
 * control how these are handled outside this routine.
 */
private ChStat NextCh(c)
register int c;
{
    char *match, *last;

    switch ((int)st) {
	case normal:
	    if (c == '%') {
		st = onep;
		cp = linebuf;
		NextChChar(c);
		break;
	    }
	    putc(c,jobout);
	    VOIDC fflush(jobout);
	    break;
	case onep:
	    if (c == '%') {
		st = twop;
		NextChChar(c);
		break;
	    }
	    putc('%',jobout);
	    putc(c,jobout);
	    VOIDC fflush(jobout);
	    st = normal;
	    break;
	case twop:
	    if (c == '[') {
		st = inmessage;
		NextChChar(c);
		break;
	    }
	    if (c == '%') {
		putc('%',jobout);
		VOIDC fflush(jobout);
		/* don't do anything to cp */
		break;
	    }
	    putc('%',jobout);
	    putc('%',jobout);
	    putc(c,jobout);
	    VOIDC fflush(jobout);
	    st = normal;
	    break;
	case inmessage:
	    NextChChar(c);
	    if (c == ']') st = close1;
	    break;
	case close1:
	    NextChChar(c);
	    switch (c) {
		case '%': st = close2; break;
		case ']': st = close1; break;
		case ' ': break;
		default: st = inmessage; break;
	    }
	    break;
	case close2:
	    NextChChar(c);
	    switch (c) {
		case '%': st = close3; break;
		case ']': st = close1; break;
		default: st = inmessage; break;
	    }
	    break;
	case close3:
	    NextChChar(c);
	    switch (c) {
		case '\r': st = close4; break;
		case ']': st = close1; break;
		default: st = inmessage; break;
	    }
	    break;
	case close4:
	    NextChChar(c);
	    switch(c) {
		case '\n': st = normal; break;
		case ']': st = close1; break;
		default: st = inmessage; break;
	    }
	    if (st == normal) {
		/* parse complete message */
		last = cp;
		*cp = '\0';
		debugp((stderr,">>%s",linebuf));
		if (match = FindPattern(cp, linebuf, " pagecount: ")) {
		    /* Do nothing */
		}
		else if (match = FindPattern(cp, linebuf, " PrinterError: ")) {
		    if (*(match-1) != ':') {
			fprintf(stderr,"%s",linebuf);
			VOIDC fflush(stderr);
			*(last-6) = 0;
			Status(match+15, 2);
		    }
		    else {
			last = index(match,';');
			*last = '\0';
			Status(match+15, 2);
		    }
		}
		/* PrinterError's for certain (rare) printers */
		else if (match = FindPattern(cp, linebuf, " printer: ")) {
		    if (*(match-1) != ':') {
			fprintf(stderr,"%s",linebuf);
			VOIDC fflush(stderr);
			*(last-6) = 0;
			Status(match+10, 2);
		    }
		    else {
			last = index(match,';');
			*last = '\0';
			Status(match+10, 2);
		    }
		}
		else if (match = FindPattern(cp, linebuf, " status: ")) {
		    match += 9;
		    if (strncmp(match,"idle",4) == 0) { /* Printer is idle */
			return(ChIdle);
		    }
		    else {
			/* one of: busy, waiting, printing, initializing */
			/* clear status message */
			RestoreStatus();
		    }
		}
		/* WARNING: Must NOT match "PrinterError: timeout" */
		else if (match = FindPattern(cp, linebuf, " Error: timeout")) {
		    return(ChTimeout);
		}
		else {
		    /* message not for us */
		    fprintf(jobout,"%s",linebuf);
		    VOIDC fflush(jobout);
		    st = normal;
		    break;
		}
	    }
	    break;
	default:
	    fprintf(stderr,"bad case;\n");
    }
    return(ChOk);
}

/* backup "status" message file in ".status", in case there is a PrinterError */
private BackupStatus(file1, file2)
char *file1, *file2;
{
    register int fd1, fd2;
    char buf[BUFSIZ];
    int cnt;

    VOIDC umask(0);
    fd1 = open(file1, O_WRONLY|O_CREAT, 0664);
    if ((fd1 < 0) || (flock(fd1,LOCK_EX) < 0)) {
	VOIDC unlink(file1);
	VOIDC flock(fd1,LOCK_UN);
	VOIDC close(fd1);
	fd1 = open(file1, O_WRONLY|O_CREAT, 0664);
    }
    if ((fd1 < 0) || (flock(fd1,LOCK_EX) <0)) {
	fprintf(stderr, "%s: writing %s",prog,file1);
	perror("");
	VOIDC close(fd1);
	return;
    }
    VOIDC ftruncate(fd1,0);
    if ((fd2 = open(file2, O_RDONLY,0)) < 0) {
	fprintf(stderr, "%s: error reading %s", prog, file2);
	perror("");
	VOIDC close(fd1);
	return;
    }
    cnt = read(fd2,buf,BUFSIZ);
    VOIDC write(fd1,buf,cnt);
    VOIDC flock(fd1,LOCK_UN);
    VOIDC close(fd1);
    VOIDC close(fd2);
}

/* restore the "status" message from the backed-up ".status" copy */
private RestoreStatus() {
    BackupStatus("status",".status");
    newstatmsg = FALSE;		/* Say we went back to the old message */
}

/* report PrinterError via "status" message file */
/* The notify flag is set to notify the user. */
/* 0 - Don't notify user */
/* 1 - Notify user once */
/* 2 - Notify all occurances, but filter repeats */

private Status(msg, notify)
register char *msg;
int notify;
{
    register int fd;
    char msgbuf[100];
    static char lastmsg[100];
    static int lastnotify=0;

    if ((fd = open("status",O_WRONLY|O_CREAT,0664)) < 0) return;
    VOIDC ftruncate(fd,0);
    sprintf(msgbuf,"Printer Error: may need attention! (%s)\n\0",msg);
    VOIDC write(fd,msgbuf,strlen(msgbuf));
    VOIDC close(fd);
    if(notify) {
	if(((lastnotify == 1) && (notify==1)) ||
	   (lastnotify && !strcmp(msgbuf, lastmsg))) {
	    /* Same notification level, or same message, do nothing */
	} else {
	    strcpy(lastmsg, msgbuf);
#ifdef SYSLOG
	    syslog(LOG_INFO, "%s:%s", pname, msgbuf);
#endif
#ifdef ZEPHYR
	    NotifyUser(name, msgbuf);
#endif
	    lastnotify = notify;
        }
    }

    newstatmsg = TRUE;		/* Say we changed the status message */
}

/* Child has exited.  If there is a problem, this routine causes the program
 * to abort.  Otherwise, the routine just returns.
 */
private VOID listenexit(exitstatus)
union wait exitstatus;     /* Status returned by the child */
{
    debugp((stderr,"%s: Listener return status: 0x%x\n",prog,exitstatus));
    if( exitstatus.w_termsig != 0 ) {   /* Some signal got the child */
	fprintf(stderr,"%s: Error: Listener process killed using signal=%d\n",
	    prog,exitstatus.w_termsig);
	VOIDC fflush(stderr);
	croak(TRY_AGAIN);
	}
    else switch( exitstatus.w_retcode ) {   /* Depends on child's exit status */
	case LIS_IDLE:    /* Printer went idle during job. Probably rebooted. */
	    fprintf(stderr,"%s: ERROR: printer is idle. Giving up!\n",prog);
	    VOIDC fflush(stderr);
	    croak(THROW_AWAY);
	case LIS_TIMEOUT:    /* Printer timed out during job. System loaded? */
	    fprintf(stderr,"%s: ERROR: printer timed out. Trying again.\n",prog);
	    VOIDC fflush(stderr);
	    croak(TRY_AGAIN);
	case LIS_EOF:     /* Comm line down.  Somebody unplugged the printer? */
	    fprintf(stderr,
		    "%s: unexpected EOF from printer (listening)!\n",prog);
	    VOIDC fflush(stderr);
	    sleep(10);
	    croak(TRY_AGAIN);
	case LIS_ERROR:		   /* Listener died */
	    fprintf(stderr,
		    "%s: unrecoverable error from printer (listening)!\n",prog);
	    VOIDC fflush(stderr);
	    sleep(30);
	    croak(TRY_AGAIN);
	case LIS_NORMAL:           /* Normal exit. Keep going */
	case LIS_DIE:              /* Parent said to die. */
	default:
	    break;
	}
}

/* Reap the children.  This returns when all children are dead.
 * This routine ASSUMES we are dying.
 */
private VOID reapchildren() {
    intstate = croaking;    /* OK -- we are dying */
    VOIDC setjmp(croaklabel);    /* Get back here when we get an alarm */
    VOIDC unlink(".banner");         /* get rid of banner file */
    VOIDC alarm(CROAKALARM);
    if( cpid != 0 ) VOIDC kill(cpid,SIGEMT);  /* This kills listener */
    if( rpid != 0 ) VOIDC kill(rpid,SIGINT);
    if( fpid != 0 ) VOIDC kill(fpid,SIGINT);
    while (wait((union wait *) 0) > 0);
    VOIDC alarm(0);         /* No more alarms */
    if (VerboseLog) {
	VOIDC time(&starttime);
	fprintf(stderr,"%s: end - %s",prog,ctime(&starttime));
	VOIDC fflush(stderr);
    }
}

/* Reap our children and die */
private VOID croak(exitcode)
int exitcode;
{
    VOIDC reapchildren();
    RestoreStatus();
    exit(exitcode);
}

/* Exit and printer system error message with perror() */
private VOID myexit1(progname,exitcode)
char  *progname;
int    exitcode;
{
    saveerror = errno;
    VOIDC reapchildren();
    RestoreStatus();
    errno = saveerror;
    pexit(progname,exitcode);
}

/* Exit and print system error message with perror() and printer a reason */
private VOID myexit2(progname,reason,exitcode)
char  *progname;
char  *reason;
int    exitcode;
{
    saveerror = errno;
    VOIDC reapchildren();
    RestoreStatus();
    errno = saveerror;
    pexit2(progname,reason,exitcode);
}

/* Close down without having done anything much */
private VOID closedown() {
    fprintf(stderr,"%s: abort (during startup)\n",prog);
    VOIDC fflush(stderr);
    croak(THROW_AWAY);
}

/* On receipt of a job abort, we normally get the printer to abort the job
 * and still do the normal final banner page and accounting entry.  This
 * requires the printer to respond correctly.  If the printer is busted, it
 * will not respond.  In order to prevent this process from sitting around
 * until a working printer is connected to the printer port, this routine
 * gets called went it is deemed that the printer will not respond...
 */
private VOID dieanyway() {
    fprintf(stderr,"%s: No response from printer after abort.  Giving up!\n",
	prog);
    croak(THROW_AWAY);
    }

/* Abort the current job running on the printer */
private VOID abortjob() {
    if( jobaborted ) return;		/* Don't repeat work */
    alarm(ABORTALARM);               /* Limit time for printer to respond */
    jobaborted = TRUE;

    if( resetprt() || write(fdsend,abortbuf,1) != 1 ) {
	fprintf(stderr, "%s: ioctl error (abort job)", prog);
	perror("");
    }
}

/* Got an EMT signal */
private VOID GotEmtSig() {
    debugp((stderr,"%s: Got SIGEMT signal\n",prog));
    /* This signal does not need to obey the critical region rules */
    switch( (int)intstate ) {
	case init:               /* Reverser process says it is done */
	    if(flagsig) {
		gotsig |= EMT_INT;  /* We just say we got one and return */
		return;
		}
	    else {
		longjmp(initlabel,1);
		}
	    break;
	case sending:
	case waiting:
	    while( (wpid=wait(&status)) > 0 )
		if(wpid==cpid) {
		    cpid = 0;
		    listenexit(status);
		    break;
		    }
	    if( intstate == sending ) {
		fprintf(stderr,"WARNING: Check spooled PostScript for control characters.\n");
		fflush(stderr);
		}
	    childdone = TRUE;    /* Child exited somehow */
	    break;
	case child:
	    VOIDC kill( getppid(),SIGEMT );   /* Tell parent we are exiting */
	    exit(LIS_DIE);    /* Parent says die -- we exit early */
	default:
	    break; /* Ignore it */
        }
}

/* Got an alarm signal. */
private VOID GotAlarmSig() {
    char mybuf[BUFSIZ];

    debugp((stderr,"%s: Got alarm signal %d %d %d %d\n",
	prog,intstate,oldprogress,progress,getstatus));
    if(flagsig) {
	gotsig |= ALARM_INT;  /* We just say we got one and return */
	return;
	}
    switch( (int)intstate ) {
	case syncstart:
	case synclast:
	    if( jobaborted ) dieanyway();   /* If already aborted, just croak */
	    sprintf(mybuf, "Not Responding for %ld minutes",
		(time((long*)0)-starttime+30)/60);
	    Status(mybuf,1);
	    longjmp(synclabel,1);
	case sending:
	    if( progress == oldprogress ) { /* Nothing written since last time */
		getstatus = TRUE;
		}
	    else {
		oldprogress = progress;
		getstatus = FALSE;
		}
	    VOIDC alarm(SENDALARM); /* reset the alarm and return */
	    break;
	case waiting:
	    if( jobaborted ) dieanyway();   /* If already aborted, just croak */
	    VOIDC write(fdsend, statusbuf, 1);
	    if( kill(cpid,0) < 0 ) childdone = TRUE;  /* Missed exit somehow */
	    VOIDC alarm(WAITALARM); /* reset the alarm and return */
	    break;
	case lastpart:
	    if( jobaborted ) dieanyway();   /* If already aborted, just croak */
	    break;
	case croaking:
	    longjmp(croaklabel,1);
	case child:
	    if( kill(getppid(),0) < 0 ) {  /* Missed death signal from parent */
		fprintf(stderr,
		    "%s: Error: Parent exited without signalling child\n",prog);
		VOIDC fflush(stderr);
		exit(LIS_NOPARENT);
		}
	    alarm(CHILDWAIT);
	    break;
	default:
	    break; /* Ignore it */
        }
}

/* Got a "die" signal, like SIGINT */
private VOID GotDieSig(sig) int sig; {
    debugp((stderr,"%s: Got 'die' signal=%d\n",prog,sig));
    if(flagsig) {
	gotsig |= DIE_INT;  /* We just say we got one and return */
	return;
	}
    switch( (int)intstate ) {
	case init:
	    VOIDC closedown();
	case syncstart:
	    fprintf(stderr,"%s: abort (start communications)\n",prog);
	    VOIDC fflush(stderr);
	    abortjob();
	    VOIDC write(fdsend, eofbuf, 1);
	    croak(THROW_AWAY);
	case sending:
	    fprintf(stderr,"%s: abort (sending job)\n",prog);
	    VOIDC fflush(stderr);
	    abortjob();
	    longjmp(sendlabel,1);
	case waiting:
	    if( !jobaborted ) {
	        fprintf(stderr,"%s: abort (waiting for job end)\n",prog);
	        VOIDC fflush(stderr);
		}
	    abortjob();
	    break;
	case lastpart:
	case synclast:
	    if( jobaborted ) dieanyway();   /* If already aborted, just croak */
	    fprintf(stderr,"%s: abort (post-job processing)\n",prog);
	    VOIDC fflush(stderr);
	    alarm(ABORTALARM);
	    jobaborted = TRUE;
	    break;
	case child:
	    alarm(CHILDWAIT);   /* Wait a while, then see if parent is alive */
	    break;
	default:
	    break; /* Ignore it */
        }
}

/* Open the printer for listening.
 * This fcloses the old file as well, so it may be used to throw away
 * any buffered input.
 * NOTE: The printcap entry specifies "rw" and we get invoked with
 * stdout == the device, so we dup stdout, and reopen it for reading;
 * this seems to work fine...
 */
private VOID openprtread() {
#ifdef BRIDGE
    if (psin == NULL && (psin=fdopen(fdsend, "r")) == NULL) {
	myexit1(prog, THROW_AWAY);
    }
#else
    if (psin != NULL ) VOIDC fclose(psin);  /* Close old one */
    if ((fdlisten = dup(fdsend)) < 0) /* the printer (read) */
        myexit1(prog, THROW_AWAY);
    if ((psin = fdopen(fdlisten, "r")) == NULL) {
	myexit1(prog, THROW_AWAY);
    }
#endif BRIDGE
}

/* Flush the I/O queues for the printer, and restart output to the
 * printer if it has been XOFF'ed.
 * Returns correct error stuff for perror() if a system call fails.
 * NOTE: If this routine does nothing, one can get into a state where
 * syncprinter() loops forever reading OLD responses to the acct message.
 */
private int resetprt() {
#ifdef BRIDGE
    static struct timeval t = {0,0};    /* Zero time */
    int  rmask;				/* Hope fdsend bit fits in here... */
    int c;

    rmask = 1<<fdsend;
    debugp((stderr,"%s: Start reset\n",prog));
    while( select(sizeof(int)*8,&rmask,(int*)0,(int*)0,&t) == 1 ) {
        debugp((stderr,"%s: Read another buffer cnt=%d rmask=0x%x\n",prog,psin->_cnt,rmask));
	while( psin->_cnt > 0 ) c = getc(psin);  /* Empty the buffer */
	c = getc(psin);   /* Fill the buffer */
	if (c == EOF) {
	    if( feof(psin) ) {
	        flagsig = 1;
	        fprintf(stderr,"%s: unexpected EOF from printer (resetprinter)!\n",
		        prog);
	        VOIDC fflush(stderr);
	        sleep(10);
	        croak(TRY_AGAIN);
	        }
	    else {
	        if( errno != EINTR ) {	/* Got a random error */
		    fprintf(stderr,"%s: Error" );
		    perror("");
		    fflush(stderr);
		    sleep(30);
		    croak(TRY_AGAIN);
		    }
	        }
	    clearerr(psin);		/* Make it so we can read again */
	    continue;
	    }
	}
    debugp((stderr,"%s: Read last buffer cnt=%d rmask=0x%x\n",prog,psin->_cnt,rmask));
    while( psin->_cnt > 0 ) c = getc(psin);  /* Empty the buffer */
#else
    int flg = FREAD|FWRITE;	 /* ioctl FLUSH arg */

    VOIDC openprtread();    /* Re-open the printer */
    if (ioctl(fdsend,TIOCFLUSH,&flg) || ioctl(fdsend,TIOCSTART,&flg) )
	return(-1);
#endif BRIDGE

    return(0);
}

/* Synchronize the input and output of the printer.
 * We use the accounting message, and include our process ID and
 * a sequence number.  If the output doesn't match what we expect, we
 * sleep a bit, flush the terminal buffers, and try again.
 * WARNING: Make sure there are no pending alarms before calling this routine.
 */
private VOID syncprinter(pagecount)
long    *pagecount;        /* The current page count in the printer */
{
    static int synccount=0; /* Unique number for acct output.  This is static
			       so ALL calls will produce different output */
    unsigned int sleeptime; /* Number of seconds to sleep */
    char *mp;         /* Current pointer into mybuf */
    int gotpid;       /* The process ID returned from the printer */
    int gotnum;       /* The synccount returned from the printer */
    register int r;   /* Place to put the chars we get */
    int sc;           /* Results from sscanf() */
    int errcnt;       /* Number of errors we have output */
    char mybuf[BUFSIZ];

    VOIDC time(&starttime);    /* Get current time for status warnings */
    errcnt = 0;
    sleeptime = 2;    /* Initial sleep interval */
    jobout = stderr;  /* Write extra stuff to the log file */
    NextChInit();
    openprtread();    /* Open the printer for reading */
    while(TRUE) {
	synccount++;
	if( setjmp(synclabel) ) goto tryagain;   /* Got an alarm */
	VOIDC alarm(SYNCALARM); /* schedule an alarm/timeout */
	VOIDC sprintf(mybuf, getpages, mpid, synccount, "\004");
	VOIDC write(fdsend, mybuf, strlen(mybuf)); /* Send program */
        mp = mybuf; *mp = '\0';
	while (TRUE) {  /* Listen for return string */
	    r = getc(psin);
	    if (r == EOF) {
		if( feof(psin) ) {
		    flagsig = 1;
		    fprintf(stderr,
			"%s: unexpected EOF from printer (sync)!\n",prog);
	            VOIDC fflush(stderr);
	            sleep(10);
	            croak(TRY_AGAIN);
		    }
		else {
		    if( errno != EINTR ) {	/* Got a random error */
			fprintf(stderr,"%s: Error" );
			perror("");
			fflush(stderr);
		        sleep(10);
		        croak(TRY_AGAIN);
			}
		    }
		clearerr(psin);		/* Make it so we can read again */
		continue;
	        }
	    if ((r&0377) == 004) break; /* Echoed back end of job */
	    VOIDC NextCh(r);   /* Ignore return values */
	    if( mp >= mybuf+sizeof(mybuf)-1 ) {  /* Overflow? */
		mp = '\0';
		strcpy( mybuf,mybuf+sizeof(mybuf)/2 );
		mp = mybuf+sizeof(mybuf)/2;
		}
	    *mp++ = r;
	    }
	*mp = '\0';
	debugp((stderr,"%s: sync reply (%s)\n",prog,mybuf));
	if (mp = FindPattern(mp, mybuf, "%%[ pagecount: ")) {
	    sc = sscanf(mp, "%%%%[ pagecount: %ld, %d %d ]%%%%\r",
		pagecount,&gotpid,&gotnum);
	    }
	if( mp!=NULL && sc==3 && gotpid==mpid && gotnum==synccount ) break;
	errcnt++;
	if( errcnt <= 3 || errcnt%10==0 ) {   /* Only give a few errors */
	    fprintf(stderr, "%s: printer sync problem [%d] (%s)\n",
		prog,synccount,mybuf);
	    VOIDC fflush(stderr);
	    }
	VOIDC alarm(0);    /* Don't want alarms anymore */
	sleep(sleeptime);    /* Sleep for a while */
	sleeptime *= 2;      /* Wait for longer next time */
	if( sleeptime > MAXSLEEP ) {
	    if( jobaborted ) dieanyway();   /* If already aborted, just croak */
	    sleeptime = MAXSLEEP;
	    }
	tryagain:
	if(resetprt()) {      /* Reset the printer I/O queues */
	    fprintf(stderr, "%s: ioctl error (sync)", prog);
	    perror("");
	    }
	}
    VOIDC alarm(0);    /* Make sure we don't longjmp() after exiting */
}

/* Make an entry in the accounting file */
private VOID acctentry(start,end,account,mediacost)
long start,end;         /* Starting and ending page counts for job */
char *mediacost, account;
{
    struct timeval timestamp;
    debugp((stderr,"%s: Make acct entry s=%ld, e=%ld\n",prog,start,end));

    /* The following is cause you might not really print the banner page if
       the job is aborted and you get a negative number. Ugly.... */
    if(end-start-bannerpages < 0) bannerpages = 0;
    if( start > end || start < 0 || end < 0 ) {
	fprintf(stderr,"%s: accounting error 3, %ld %ld\n",prog, start,end);
	fflush(stderr);
	}
    else if( freopen(accountingfile,"a",stdout) != NULL ) {
	gettimeofday(&timestamp, NULL);
	if (account == NULL) 
	    printf("%d\t%s:%s\t%ld\t0\t%s\t%d\n",(end-start),host,name,
		   timestamp.tv_sec,mediacost, (end-start-bannerpages));
	else if (mediacost == NULL) 
	    printf("%d\t%s:%s\t%ld\t%s\t0\t%d\n",(end-start),host,name,
		   timestamp.tv_sec,account, (end-start-bannerpages));
	else printf("%d\t%s:%s\t%ld\t%s\t%s\t%d\n",(end-start),host,name,
		    timestamp.tv_sec,account,mediacost, (end-start-bannerpages));
	VOIDC fclose(stdout);
	}
}

#ifdef ZEPHYR
NotifyUser(user, message)
     char *user, *message;
{
    ZNotice_t notice;
    char fromprinter[100];

    /* Return if NULL message */
    if (message[0] == '\0')
	return;

    bzero((char *)&notice, sizeof(notice));
    
    if (ZInitialize() != ZERR_NONE) 
	return;
    /* Something wrong, oh well... */

    notice.z_kind = UNACKED;
    notice.z_port = 0;
    notice.z_class = "MESSAGE";
    notice.z_class_inst = "PERSONAL";
    notice.z_opcode = "";
    sprintf(fromprinter, "Athena Print Server for printer %s",pname);
    notice.z_sender = fromprinter;
    notice.z_recipient = user;
    notice.z_default_format = "";
    notice.z_message = message;
    notice.z_message_len = strlen(notice.z_message)+1;

    if (ZSendNotice(&notice, ZNOAUTH) != ZERR_NONE) {
	return;
    }
    return;
}
#endif ZEPHYR
