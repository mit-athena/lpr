#ifndef SERVER

#define TMPDIR "/tmp"
#include "lp.h"

/*
 * Error tokens
 */
#define REPRINT		-2
#define ERROR		-1
#define	OK		0
#define	FATALERR	1
#define	NOACCT		2
#define	FILTERERR	3
#define	ACCESS		4
#endif

/*
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/lpr/lpr.c,v $
 *	$Author: epeisach $
 *	$Locker:  $
 *	$Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/lpr.c,v 1.6 1990-07-05 14:41:33 epeisach Exp $
 */

#ifndef lint
static char *rcsid_lpr_c = "$Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/lpr.c,v 1.6 1990-07-05 14:41:33 epeisach Exp $";
#endif lint

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)lpr.c	5.2 (Berkeley) 11/17/85";
#endif not lint

/*
 *      lpr -- off line print
 *
 * Allows multiple printers and printers on remote machines by
 * using information from a printer data base.
 */

#include <stdio.h>
#include <grp.h>
#include <syslog.h>
#include <sys/types.h>
#ifdef SERVER
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <signal.h>
#include <ctype.h>
#include <netdb.h>
#include "lp.local.h"
#endif SERVER
	
#define UNLINK unlink

#ifndef SERVER
char  *RG;  			/* Restricted group */
	
#else SERVER
char    *tfname;		/* tmp copy of cf before linking */
#endif SERVER
char    *cfname;		/* daemon control files, linked from tf's */
char    *dfname;		/* data files */

int	nact;			/* number of jobs to act on */
#ifndef SERVER
int	pfd;			/* Printer file descriptor */
#endif SERVER
int	tfd;			/* control file descriptor */
int     mailflg;		/* send mail */
int	qflag;			/* q job, but don't exec daemon */
char	format = 'f';		/* format char for printing files */
int	rflag;			/* remove files upon completion */	
#ifdef SERVER
int	sflag;			/* symbolic link flag */
#endif SERVER
int	inchar;			/* location to increment char in file names */
int     ncopies = 1;		/* # of copies to make */
int	iflag;			/* indentation wanted */
int	indent;			/* amount to indent */
int 	noendpage;  	    	/* file contains own form feeds */
int	hdr = 1;		/* print header or not (default is yes) */
int     userid;			/* user id */
char	*person;		/* user name */
char	*title;			/* pr'ing title */
char	*fonts[4];		/* troff font names */
char	*width;			/* width for versatec printing */
char	host[32];		/* host name */
char	*class = host;		/* class title on header page */
char    *jobname;		/* job name on header page */
char	*name;			/* program name */
char	*printer;		/* printer name */
char	*forms;			/* printer forms (for Multics) */
struct	stat statb;
#ifdef HESIOD
char	alibuf[BUFSIZ/2];	/* buffer for printer alias */
#endif

#ifdef KERBEROS
int use_kerberos;
int kerberos_override = -1;
int account = 0;
#endif KERBEROS

#ifdef ZEPHYR
int zephyrflag = 0;
#endif ZEPHYR

#ifdef SERVER
int	MX;			/* maximum number of blocks to copy */
int	MC;			/* maximum number of copies allowed */
int	DU;			/* daemon user-id */
char	*SD;			/* spool directory */
char	*LO;			/* lock file name */
char	*RG;			/* restrict group */
short	SC;			/* suppress multiple copies */

#endif SERVER
char	*getenv();
#ifndef _AUX_SOURCE
char	*rindex();
#endif
char	*linked();
int	cleanup();

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	extern struct passwd *getpwuid();
	struct passwd *pw;
	struct group *gptr;
	extern char *itoa();
	register char *arg, *cp;
	char buf[BUFSIZ];
#ifndef SERVER
	int i, f, retry;
#else SERVER
	int i, f;
	struct stat stb;
#endif SERVER
	struct hostent *hp;

#ifndef SERVER
	pfd = -1;		/* Printer isn't open yet */

#endif SERVER
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, cleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, cleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, cleanup);

	name = argv[0];
	gethostname(host, sizeof (host));
#ifndef SERVER
	if (hp = gethostbyname(host))
		(void) strcpy(host, hp -> h_name);
#else SERVER
	if (hp = gethostbyname(host)) strcpy(host, hp -> h_name);
#endif SERVER
#ifdef LOG_LPR
	openlog("lpd", 0, LOG_LPR);
#else
	openlog("lpd", 0);
#endif

	while (argc > 1 && argv[1][0] == '-') {
		argc--;
		arg = *++argv;
		switch (arg[1]) {

		case 'P':		/* specify printer name */
			if (arg[2])
				printer = &arg[2];
			else if (argc > 1) {
				argc--;
				printer = *++argv;
			}
			break;

		case 'C':		/* classification spec */
			hdr++;
			if (arg[2])
				class = &arg[2];
			else if (argc > 1) {
				argc--;
				class = *++argv;
			}
			break;

		case 'J':		/* job name */
			hdr++;
			if (arg[2])
				jobname = &arg[2];
			else if (argc > 1) {
				argc--;
				jobname = *++argv;
			}
			break;

		case 'T':		/* pr's title line */
			if (arg[2])
				title = &arg[2];
			else if (argc > 1) {
				argc--;
				title = *++argv;
			}
			break;

		case 'l':		/* literal output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output (cat files) */
		case 'n':		/* print ditroff output */
		case 'd':		/* print tex output (dvi files) */
		case 'g':		/* print graph(1G) output */
		case 'c':		/* print cifplot output */
		case 'v':		/* print vplot output */
			format = arg[1];
			break;

		case 'f':		/* print fortran output */
			format = 'r';
			break;

		case '4':		/* troff fonts */
		case '3':
		case '2':
		case '1':
			if (argc > 1) {
				argc--;
				fonts[arg[1] - '1'] = *++argv;
			}
			break;

		case 'w':		/* versatec page width */
			width = arg+2;
			break;

		case 'r':		/* remove file when done */
			rflag++;
			break;

		case 'm':		/* send mail when done */
			mailflg++;
			break;

#ifdef ZEPHYR
		case 'z':
			zephyrflag++;
			break;
#endif ZEPHYR

		case 'h':		/* toggle want of header page */
			hdr = !hdr;
			break;

#ifdef SERVER
		case 's':		/* try to link files */
			sflag++;
			break;

#endif SERVER
		case 'q':		/* just q job */
			qflag++;
			break;

		case 'i':		/* indent output */
			iflag++;
			indent = arg[2] ? atoi(&arg[2]) : 8;
			break;

		case '#':		/* n copies */
			if (isdigit(arg[2])) {
				i = atoi(&arg[2]);
				if (i > 0)
					ncopies = i;
			}
		case 'E':
			noendpage++;	/* form feeds there already */
			break;	/* for multics */

		case 'F':		/* printer forms */
			if (arg[2]) /* for multics */
				forms = &arg[2];
			else if (argc > 1) {
				argc--;
				forms = *++argv;
			}
			break;
#ifdef KERBEROS
		case 'u':
			kerberos_override = 0;
			break;
		case 'k':
			kerberos_override = 1;
			break;
		case 'a':
			if (isdigit(arg[2])) {
				i = atoi(&arg[2]);
				if (i > 0)
					account = i;
			}
			break;
#endif KERBEROS
		}
	}
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;
	chkprinter();
	if (SC && ncopies > 1)
		fatal("multiple copies are not allowed");
	if (MC > 0 && ncopies > MC)
		fatal("only %d copies are allowed", MC);
	/*
	 * Get the identity of the person doing the lpr using the same
	 * algorithm as lprm. 
	 */
	userid = getuid();
	if ((pw = getpwuid(userid)) == NULL)
		fatal("Who are you?");
	person = pw->pw_name;
	/*
	 * Check for restricted group access.
	 */
	if (RG != NULL) {
		if ((gptr = getgrnam(RG)) == NULL)
			fatal("Restricted group specified incorrectly");
		if (gptr->gr_gid != getgid()) {
			while (*gptr->gr_mem != NULL) {
				if ((strcmp(person, *gptr->gr_mem)) == 0)
					break;
				gptr->gr_mem++;
			}
			if (*gptr->gr_mem == NULL)
				fatal("Not a member of the restricted group");
		}
	}
#ifndef SERVER
	openpr();
	mktemps();
	(void) setuid(userid);		/* Drop root privs */
#else
	/*
	 * Check to make sure queuing is enabled if userid is not root.
	 */
	(void) sprintf(buf, "%s/%s", SD, LO);
	if (userid && stat(buf, &stb) == 0 && (stb.st_mode & 010))
		fatal("Printer queue is disabled");
#endif SERVER
	/*
	 * Initialize the control file.
	 */
#ifndef SERVER
	tfd = creat(cfname, FILMOD);
#else SERVER
	mktemps();
	tfd = nfile(tfname);
	(void) fchown(tfd, DU, -1);	/* owned by daemon for protection */
#endif SERVER
	card('H', host);
	card('P', person);
#ifdef KERBEROS
	if (account)
		card('A', itoa(account));
#endif KERBEROS
	if (forms != NULL)
	        card('F', forms);
	if (hdr) {
		if (jobname == NULL) {
			if (argc == 1)
				jobname = "stdin";
			else
				jobname = (arg = rindex(argv[1], '/')) ? arg+1 : argv[1];
		}
		card('J', jobname);
		card('C', class);
		card('L', person);
	}
 	if (noendpage)
		card('E', "");
	if (iflag)
		card('I', itoa(indent));
	if (mailflg)
		card('M', person);

#ifdef ZEPHYR
	if (zephyrflag)
		card('Z', person);
#endif ZEPHYR

	if (format == 't' || format == 'n' || format == 'd')
		for (i = 0; i < 4; i++)
			if (fonts[i] != NULL)
				card('1'+i, fonts[i]);
	if (width != NULL)
		card('W', width);	

	/*
	 * Read the files and spool them.
	 */
	if (argc == 1)
		copy(0, " ");
	else while (--argc) {
		if ((f = test(arg = *++argv)) < 0)
			continue;	/* file unreasonable */

#ifndef SERVER
		if ((cp = linked(arg)) != NULL) {
#else SERVER
		if (sflag && (cp = linked(arg)) != NULL) {
#endif SERVER
			(void) sprintf(buf, "%d %d", statb.st_dev, statb.st_ino);
			card('S', buf);
			if (format == 'p')
				card('T', title ? title : arg);
			for (i = 0; i < ncopies; i++)
				card(format, &dfname[inchar-2]);
			card('U', &dfname[inchar-2]);
			if (f)
				card('U', cp);
			card('N', arg);
			dfname[inchar]++;
			nact++;
			continue;
		}
#ifndef SERVER
		printf("%s: %s: not linked, copying instead\n", name, arg);
#else SERVER
		if (sflag)
			printf("%s: %s: not linked, copying instead\n", name, arg);
#endif SERVER
		if ((i = open(arg, O_RDONLY)) < 0) {
			printf("%s: cannot open %s\n", name, arg);
			continue;
		}
		copy(i, arg);
		(void) close(i);
		if (f && UNLINK(arg) < 0)
			printf("%s: %s: not removed\n", name, arg);
	}

	if (nact) {
		(void) close(tfd);
#ifdef SERVER
		tfname[inchar]--;
		/*
		 * Touch the control file to fix position in the queue.
		 */
		if ((tfd = open(tfname, O_RDWR)) >= 0) {
			char c;

			if (read(tfd, &c, 1) == 1 && lseek(tfd, 0L, 0) == 0 &&
			    write(tfd, &c, 1) != 1) {
				printf("%s: cannot touch %s\n", name, tfname);
				tfname[inchar]++;
				cleanup();
			}
			(void) close(tfd);
		}
		if (link(tfname, cfname) < 0) {
			printf("%s: cannot rename %s\n", name, cfname);
			tfname[inchar]++;
			cleanup();
		}
		UNLINK(tfname);
#endif SERVER
		if (qflag)		/* just q things up */
			exit(0);
#ifndef SERVER
		retry=0;		/* Retry counter */
		while (((i = sendit(&cfname[inchar-2])) == REPRINT) &&
		       (retry<3))
			retry++;
		if (i != OK) {
			fprintf(stderr,"I can't seem to be able to send ");
			fprintf(stderr,"your request to the printer.\n");
			fprintf(stderr,"Please try again later or try ");
			fprintf(stderr,"using another printer.\n");
			exit(1);
		}
#else SERVER
		if (!startdaemon(printer))
			printf("jobs queued, but cannot start daemon.\n");
#endif SERVER
		exit(0);
	}
	cleanup();
	/* NOTREACHED */
}

/*
 * Create the file n and copy from file descriptor f.
 */
copy(f, n)
	int f;
	char n[];
{
	register int fd, i, nr, nc;
	char buf[BUFSIZ];

	if (format == 'p')
		card('T', title ? title : n);
	for (i = 0; i < ncopies; i++)
		card(format, &dfname[inchar-2]);
	card('U', &dfname[inchar-2]);
	card('N', n);
	fd = nfile(dfname);
	nr = nc = 0;
	while ((i = read(f, buf, BUFSIZ)) > 0) {
		if (write(fd, buf, i) != i) {
			printf("%s: %s: temp file write error\n", name, n);
			break;
		}
		nc += i;
		if (nc >= BUFSIZ) {
			nc -= BUFSIZ;
			nr++;
			if (MX > 0 && nr > MX) {
				printf("%s: %s: copy file is too large\n", name, n);
				break;
			}
		}
	}
	(void) close(fd);
	if (nc==0 && nr==0) 
		printf("%s: %s: empty input file\n", name, f ? n : "stdin");
	else
		nact++;
}

/*
 * Try and link the file to dfname. Return a pointer to the full
 * path name if successful.
 */
char *
linked(file)
	register char *file;
{
	register char *cp;
	static char buf[BUFSIZ];

	if (*file != '/') {
		if (getwd(buf) == NULL)
			return(NULL);
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':
				file += 2;
				continue;
			case '.':
				if (file[2] == '/') {
					if ((cp = rindex(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		(void) strcat(buf, "/");
		(void) strcat(buf, file);
		file = buf;
	}
	return(symlink(file, dfname) ? NULL : file);
}

/*
 * Put a line into the control file.
 */
card(c, p2)
	register char c, *p2;
{
	char buf[BUFSIZ];
	register char *p1 = buf;
	register int len = 2;

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = c;
		len++;
	}
	*p1++ = '\n';
	write(tfd, buf, len);
}

/*
 * Create a new file in the spool directory.
 */
nfile(n)
	char *n;
{
	register int f;
	int oldumask = umask(0);		/* should block signals */

	f = creat(n, FILMOD);
	(void) umask(oldumask);
	if (f < 0) {
		printf("%s: cannot create %s\n", name, n);
		cleanup();
	}
#ifdef SERVER
	if (fchown(f, userid, -1) < 0) {
		printf("%s: cannot chown %s\n", name, n);
		cleanup();
	}
#endif SERVER
	if (++n[inchar] > 'z') {
		if (++n[inchar-2] == 't') {
			printf("too many files - break up the job\n");
			cleanup();
		}
		n[inchar] = 'A';
	} else if (n[inchar] == '[')
		n[inchar] = 'a';
	return(f);
}

/*
 * Cleanup after interrupts and errors.
 */
cleanup()
{
	register int i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	i = inchar;
#ifdef SERVER
	if (tfname)
		do
			UNLINK(tfname);
		while (tfname[i]-- != 'A');
#endif SERVER
	if (cfname)
#ifdef SERVER
		do {
			UNLINK(cfname);
		} while (cfname[i]-- != 'A');
#else
		UNLINK(cfname);
#endif SERVER
	if (dfname)
		do {
			do
				UNLINK(dfname);
			while (dfname[i]-- != 'A');
			dfname[i] = 'z';
		} while (dfname[i-2]-- != 'd');
#ifndef SERVER
	if (pfd > 0)
		(void) close(pfd);	/* Shutdown printer connection */
#endif SERVER
	exit(1);
}

/*
 * Test to see if this is a printable file.
 * Return -1 if it is not, 0 if its printable, and 1 if
 * we should remove it after printing.
 */
test(file)
	char *file;
{
#if !defined(mips) && !(defined_AUX_SOURCE)
	struct exec execb;
#endif
	register int fd;
	register char *cp;

#ifdef SERVER
	if (access(file, 4) < 0) {
		printf("%s: cannot access %s\n", name, file);
		return(-1);
	}
#endif SERVER
	if (stat(file, &statb) < 0) {
		printf("%s: cannot stat %s\n", name, file);
		return(-1);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		printf("%s: %s is a directory\n", name, file);
		return(-1);
	}
	if (statb.st_size == 0) {
		printf("%s: %s is an empty file\n", name, file);
		return(-1);
 	}
	if ((fd = open(file, O_RDONLY)) < 0) {
		printf("%s: cannot open %s\n", name, file);
		return(-1);
	}
#if !defined(mips) && !defined(_AUX_SOURCE)
	if (read(fd, &execb, sizeof(execb)) == sizeof(execb))
		switch((int) execb.a_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
#ifdef A_MAGIC4
		case A_MAGIC4:
#endif
			printf("%s: %s is an executable program", name, file);
			goto error1;
		case ARMAG:
			printf("%s: %s is an archive file", name, file);
			goto error1;
		}
#endif /* mips */
	(void) close(fd);
	if (rflag) {
		if ((cp = rindex(file, '/')) == NULL) {
			if (access(".", 2) == 0)
				return(1);
		} else {
			*cp = '\0';
			fd = access(file, 2);
			*cp = '/';
			if (fd == 0)
				return(1);
		}
		printf("%s: %s: is not removable by you\n", name, file);
	}
	return(0);

error1:
	printf(" and is unprintable\n");
	(void) close(fd);
	return(-1);
}

/*
 * itoa - integer to string conversion
 */
char *
itoa(i)
	register int i;
{
	static char b[10] = "########";
	register char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

/*
 * Get printcap entry for printer.
 */
chkprinter()
{
	int status;
	char buf[BUFSIZ];
	static char pbuf[BUFSIZ/2];
	char *bp = pbuf;
	extern char *pgetstr();

#if defined(KERBEROS) && !defined(SERVER)
	short KA;
#endif /* KERBEROS */

#ifdef HESIOD
	if ((status = pgetent(buf, printer)) <= 0) {
		if (pralias(alibuf, printer))
			printer = alibuf;
		if ((status = hpgetent(buf, printer)) < 1)
			fatal("%s: unknown printer", printer);
	}
#else
	if ((status = pgetent(buf, printer)) < 0)
		fatal("cannot open printer description file");
	else if (status == 0)
		fatal("%s: unknown printer", printer);
#endif HESIOD
#ifdef SERVER
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
#endif SERVER
	RG = pgetstr("rg", &bp);
	if ((MX = pgetnum("mx")) < 0)
		MX = DEFMX;
	if ((MC = pgetnum("mc")) < 0)
		MC = DEFMAXCOPIES;
#ifdef SERVER
	if ((DU = pgetnum("du")) < 0)
		DU = DEFUID;
#endif SERVER
	SC = pgetflag("sc");
#ifndef SERVER
	if ((RM = pgetstr("rm",&bp)) == NULL)
		RM = host;	/* If no remote machine name, assume */
				/* its local */
	if ((RP = pgetstr("rp",&bp)) == NULL)
		RP = printer;	/* Use same name if no remote printer */
				/* name */
#ifdef KERBEROS
	KA = pgetnum("ka");
	if (KA > 0)
	    use_kerberos = 1;
	else
	    use_kerberos = 0;
	if (kerberos_override > -1)
	    use_kerberos = kerberos_override;
#endif KERBEROS
#endif SERVER
}

/*
 * Make the temp files.
 */
mktemps()
{
#ifndef SERVER
	register int len, fd, n;
 	register char *cp;
#else SERVER
	register int len, fd, n;
	register char *cp;
#endif SERVER
	char buf[BUFSIZ];
#ifndef SERVER
	char *maketemp();
#else SERVER
	char *mktemp();
#endif SERVER

#ifndef SERVER
	(void) sprintf(buf, "%s/.seq", TMPDIR);
#else SERVER
	(void) sprintf(buf, "%s/.seq", SD);
#endif SERVER
	if ((fd = open(buf, O_RDWR|O_CREAT, 0661)) < 0) {
		printf("%s: cannot create %s\n", name, buf);
#ifndef SERVER
		cleanup();
#else SERVER
		exit(1);
#endif SERVER
	}
	if (flock(fd, LOCK_EX)) {
		printf("%s: cannot lock %s\n", name, buf);
#ifndef SERVER
		cleanup();
#else SERVER
		exit(1);
#endif SERVER
	}
	n = 0;
	if ((len = read(fd, buf, sizeof(buf))) > 0) {
		for (cp = buf; len--; ) {
			if (*cp < '0' || *cp > '9')
				break;
			n = n * 10 + (*cp++ - '0');
		}
	}
#ifndef SERVER
	len = strlen(TMPDIR) + strlen(host) + 8;
	cfname = maketemp("cf", n, len);
	dfname = maketemp("df", n, len);
	inchar = strlen(TMPDIR) + 3;
#else SERVER
	len = strlen(SD) + strlen(host) + 8;
	tfname = mktemp("tf", n, len);
	cfname = mktemp("cf", n, len);
	dfname = mktemp("df", n, len);
	inchar = strlen(SD) + 3;
#endif SERVER
	n = (n + 1) % 1000;
	(void) lseek(fd, 0L, 0);
	sprintf(buf, "%03d\n", n);
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);	/* unlocks as well */
}

/*
 * Make a temp file name.
 */
char *
#ifndef SERVER
maketemp(id, num, len)
#else SERVER
mktemp(id, num, len)
#endif SERVER
	char	*id;
	int	num, len;
{
	register char *s;
	extern char *malloc();

	if ((s = malloc(len)) == NULL)
		fatal("out of memory");
#ifndef SERVER
	(void) sprintf(s, "%s/%sA%03d%s", TMPDIR, id, num, host);
#else SERVER
	(void) sprintf(s, "%s/%sA%03d%s", SD, id, num, host);
#endif SERVER
	return(s);
}

#ifdef SERVER
/*VARARGS1*/
fatal(msg, a1, a2, a3)
	char *msg;
{
	printf("%s: ", name);
	printf(msg, a1, a2, a3);
	putchar('\n');
	exit(1);
}
#endif SERVER

