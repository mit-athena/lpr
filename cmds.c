/*
 *	$Id: cmds.c,v 1.12 1999-01-22 23:10:36 ghudson Exp $
 */

#ifndef lint
static char *rcsid_cmds_c = "$Id: cmds.c,v 1.12 1999-01-22 23:10:36 ghudson Exp $";
#endif lint

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cmds.c	5.2 (Berkeley) 3/30/86";
#endif /* not lint */

/*
 * lpc -- line printer control program -- commands:
 */

#include "lp.h"
#include <string.h>
#if defined(POSIX) && !defined(ultrix)
#include "posix.h" /* for flock() */
#endif
#include <sys/time.h>

char	*user[MAXUSERS];		/* users to process */
int	users;				/* # of users in user array */
int	requ[MAXREQUESTS];		/* job number of spool entries */
int	requests;			/* # of spool requests */

/* Currently "lpc" runs only off information in /etc/printcap.
 * If you want it to query Hesiod, #define HESIOD_LPC here.
 */

/*
 * kill an existing daemon and disable printing.
 */
cmd_abort(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: abort {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			abortpr(1);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		abortpr(1);
	}
}

abortpr(dis)
{
	register FILE *fp;
	struct stat stbuf;
	int pid, fd;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	if (dis) {
		if (stat(line, &stbuf) >= 0) {
			if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
				printf("\tcannot disable printing\n");
			else
				printf("\tprinting disabled\n");
		} else if (errno == ENOENT) {
			if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
				printf("\tcannot create lock file\n");
			else {
				(void) close(fd);
				printf("\tprinting disabled\n");
				printf("\tno daemon to abort\n");
			}
			return;
		} else {
			printf("\tcannot stat lock file\n");
			return;
		}
	}
	/*
	 * Kill the current daemon to stop printing now.
	 */
	if ((fp = fopen(line, "r+")) == NULL) {
		printf("\tcannot open lock file\n");
		return;
	}
	if (!getline(fp) || flock(fileno(fp), LOCK_SH|LOCK_NB) == 0) {
		(void) fclose(fp);	/* unlocks as well */
		printf("\tno daemon to abort\n");
		return;
	}
	(void) fclose(fp);
	if (kill(pid = atoi(line), SIGTERM) < 0)
		printf("\tWarning: daemon (pid %d) not killed\n", pid);
	else
		printf("\tdaemon (pid %d) killed\n", pid);
}

/*
 * Remove all spool files and temporaries from the spooling area.
 */
clean(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: clean {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			cleanpr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		cleanpr();
	}
}



static
cselect(d)
#ifdef POSIX
struct dirent *d;
#else
struct direct *d;
#endif
{
	int c = d->d_name[0];

	if ((c == 't' || c == 'c' || c == 'd') && d->d_name[1] == 'f')
		return(1);
	return(0);
}

/*
 * Comparison routine for scandir. Sort by job number and machine, then
 * by `cf', `tf', or `df', then by the sequence letter A-Z, a-z.
 */
sortq(d1, d2)
#ifdef POSIX
struct dirent **d1, **d2;
#else
struct direct **d1, **d2;
#endif
{
	int c1, c2;

	if (c1 = strcmp((*d1)->d_name + 3, (*d2)->d_name + 3))
		return(c1);
	c1 = (*d1)->d_name[0];
	c2 = (*d2)->d_name[0];
	if (c1 == c2)
		return((*d1)->d_name[2] - (*d2)->d_name[2]);
	if (c1 == 'c')
		return(-1);
	if (c1 == 'd' || c2 == 'c')
		return(1);
	return(-1);
}

/*
 * Remove incomplete jobs from spooling area.
 */
cleanpr()
{
	register int i, n;
	register char *cp, *cp1, *lp;
#ifdef POSIX
	struct dirent **queue;
#else
	struct direct **queue;
#endif
	int nitems;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	printf("%s:\n", printer);

	for (lp = line, cp = SD; *lp++ = *cp++; )
		;
	lp[-1] = '/';

	nitems = scandir(SD, &queue, cselect, sortq);
	if (nitems < 0) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	if (nitems == 0)
		return;
	i = 0;
	do {
		cp = queue[i]->d_name;
		if (*cp == 'c') {
			n = 0;
			while (i + 1 < nitems) {
				cp1 = queue[i + 1]->d_name;
				if (*cp1 != 'd' || strcmp(cp + 3, cp1 + 3))
					break;
				i++;
				n++;
			}
			if (n == 0) {
				strcpy(lp, cp);
				unlinkf(line);
			}
		} else {
			/*
			 * Must be a df with no cf (otherwise, it would have
			 * been skipped above) or a tf file (which can always
			 * be removed).
			 */
			strcpy(lp, cp);
			unlinkf(line);
		}
     	} while (++i < nitems);
}
 
unlinkf(name)
	char	*name;
{
	if (UNLINK(name) < 0)
		printf("\tcannot remove %s\n", name);
	else
		printf("\tremoved %s\n", name);
}

/*
 * Enable queuing to the printer (allow lpr's).
 */
enable(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: enable {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			enablepr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		enablepr();
	}
}

enablepr()
{
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn off the group execute bit of the lock file to enable queuing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & 0767) < 0)
			printf("\tcannot enable queuing\n");
		else
			printf("\tqueuing enabled\n");
	}
}

/*
 * Disable queuing.
 */
disable(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: disable {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			disablepr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		disablepr();
	}
}

disablepr()
{
	register int fd;
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 010) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tqueuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0670)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tqueuing disabled\n");
		}
		return;
	} else
		printf("\tcannot stat lock file\n");
}

/*
 * Disable queuing and printing and put a message into the status file
 * (reason for being down).
 */
down(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: down {all | printer} [message ...]\n");
		return;
	}
	if (!strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			aputmsg(argc - 2, argv + 2);
		}
		return;
	}
	printer = argv[1];
#ifdef HESIOD_LPC
	if ((status = pgetent(line, printer)) <= 0) {
		if(pralias(alibuf, printer))
				printer = alibuf;
		if ((status = hpgetent(line, printer)) < 1) {
			printf("unknown printer %s\n", printer);
			return;
		}
	}
#else
	if ((status = pgetent(line, printer)) < 0) {
		printf("cannot open printer description file\n");
		return;
	} else if (status == 0) {
		printf("unknown printer %s\n", printer);
		return;
	}
#endif HESIOD_LPC
	aputmsg(argc - 2, argv + 2);
}

aputmsg(argc, argv)
	char **argv;
{
	register int fd;
	register char *cp1, *cp2;
	char buf[1024];
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing and
	 * turn on the owner execute bit of the lock file to disable printing.
	 */
	(void) sprintf(line, "%s/%s", SD, LO);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0110) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tprinter and queuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0770)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tprinter and queuing disabled\n");
		}
		return;
	} else
		printf("\tcannot stat lock file\n");
	/*
	 * Write the message into the status file.
	 */
	(void) sprintf(line, "%s/%s", SD, ST);
	fd = open(line, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		printf("\tcannot create status file\n");
		return;
	}
	(void) ftruncate(fd, 0);
	if (argc <= 0) {
		(void) write(fd, "\n", 1);
		(void) close(fd);
		return;
	}
	cp1 = buf;
	while (--argc >= 0) {
		cp2 = *argv++;
		while (*cp1++ = *cp2++)
			;
		cp1[-1] = ' ';
	}
	cp1[-1] = '\n';
	*cp1 = '\0';
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);
}

/*
 * Exit lpc
 */
quit(argc, argv)
	char *argv[];
{
	exit(0);
}

/*
 * Kill and restart the daemon.
 */
restart(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	struct hostent *hp;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: restart {all | printer ...}\n");
		return;
	}
  	gethostname(host, sizeof(host));
	if (hp = gethostbyname(host)) strcpy(host, hp->h_name);

	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			abortpr(0);
			startpr(0);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		abortpr(0);
		startpr(0);
	}
}

/*
 * Enable printing on the specified printer and startup the daemon.
 */
start(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];
	struct hostent *hp;

	if (argc == 1) {
		printf("Usage: start {all | printer ...}\n");
		return;
	}
  	gethostname(host, sizeof(host));
	if (hp = gethostbyname(host)) strcpy (host, hp -> h_name);

	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			startpr(1);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		startpr(1);
	}
}

startpr(enable)
{
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn off the owner execute bit of the lock file to enable printing.
	 */
	if (enable && stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & (enable==2 ? 0666 : 0677)) < 0)
			printf("\tcannot enable printing\n");
		else
			printf("\tprinting enabled\n");
	}
	if (!startdaemon(printer))
		printf("\tcouldn't start daemon\n");
	else
		printf("\tdaemon started\n");
}

/*
 * Print the status of each queue listed or all the queues.
 */
status(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			prstat();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		prstat();
	}
}

/*
 * Print the status of the printer queue.
 */
prstat()
{
	struct stat stbuf;
	register int fd, i;
#ifdef POSIX
	register struct dirent *dp;
#else
	register struct direct *dp;
#endif
	DIR *dirp;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	printf("%s:\n", printer);
	(void) sprintf(line, "%s/%s", SD, LO);
	if (stat(line, &stbuf) >= 0) {
		printf("\tqueuing is %s\n",
			(stbuf.st_mode & 010) ? "disabled" : "enabled");
		printf("\tprinting is %s\n",
			(stbuf.st_mode & 0100) ? "disabled" : "enabled");
	} else {
		printf("\tqueuing is enabled\n");
		printf("\tprinting is enabled\n");
	}
	if ((dirp = opendir(SD)) == NULL) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	i = 0;
	while ((dp = readdir(dirp)) != NULL) {
		if (*dp->d_name == 'c' && dp->d_name[1] == 'f')
			i++;
	}
	closedir(dirp);
	if (i == 0)
		printf("\tno entries\n");
	else if (i == 1)
		printf("\t1 entry in spool area\n");
	else
		printf("\t%d entries in spool area\n", i);
	fd = open(line, O_RDONLY);
	if (fd < 0 || flock(fd, LOCK_SH|LOCK_NB) == 0) {
		(void) close(fd);	/* unlocks as well */
		printf("\tno daemon present\n");
		return;
	}
	(void) close(fd);
	putchar('\t');
	(void) sprintf(line, "%s/%s", SD, ST);
	fd = open(line, O_RDONLY);
	if (fd >= 0) {
		(void) flock(fd, LOCK_SH);
		while ((i = read(fd, line, sizeof(line))) > 0)
			(void) fwrite(line, 1, i, stdout);
		(void) close(fd);	/* unlocks as well */
	}
	putchar('\n');
}

/*
 * Stop the specified daemon after completing the current job and disable
 * printing.
 */
stop(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: stop {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			stoppr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		stoppr();
	}
}

stoppr()
{
	register int fd;
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
			printf("\tcannot disable printing\n");
		else
			printf("\tprinting disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tprinting disabled\n");
		}
	} else
		printf("\tcannot stat lock file\n");
}

struct	queue_ **queue;
int	nitems;
time_t	mtime;

/*
 * Put the specified jobs at the top of printer queue.
 */
topq(argc, argv)
	char *argv[];
{
	register int i;
	struct stat stbuf;
	int status, changed;

	if (argc < 3) {
		printf("Usage: topq printer [jobnum ...] [user ...]\n");
		return;
	}

	--argc;
	printer = *++argv;
#ifdef HESIOD_LPC
	if ((status = pgetent(line, printer)) <= 0) {
		if(pralias(alibuf, printer))
			printer = alibuf;
		if ((status = hpgetent(line, printer)) < 1) {
			printf("unknown printer %s\n", printer);
			return;
		}
	}
#else
	if ((status = pgetent(line, printer)) < 0) {
		printf("cannot open printer description file\n");
		return;
	} else if (status == 0) {
		printf("unknown printer %s\n", printer);
		return;
	}
#endif HESIOD_LPC
	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	printf("%s:\n", printer);

	if (chdir(SD) < 0) {
		printf("\tcannot chdir to %s\n", SD);
		return;
	}
	nitems = getq_(&queue);
	if (nitems == 0)
		return;
	changed = 0;
	mtime = queue[0]->q_time;
	for (i = argc; --i; ) {
		if (doarg(argv[i]) == 0) {
			printf("\tjob %s is not in the queue\n", argv[i]);
			continue;
		} else
			changed++;
	}
	for (i = 0; i < nitems; i++)
		free(queue[i]);
	free(queue);
	if (!changed) {
		printf("\tqueue order unchanged\n");
		return;
	}
	/*
	 * Turn on the public execute bit of the lock file to
	 * get lpd to rebuild the queue after the current job.
	 */
	if (changed && stat(LO, &stbuf) >= 0)
		(void) chmod(LO, (stbuf.st_mode & 0777) | 01);
} 
#ifdef POSIX
/*
 * Reposition the job by changing the modification time of
 * the control file.
 */
#include <utime.h>
 
touch(q)
	struct queue_ *q;
{
	struct utimbuf tvp;

	tvp.modtime = --mtime;
	tvp.actime = 0;
	return(utime(q->q_name, &tvp));
}
#else

touch(q)
	struct queue_ *q;
{
	struct timeval tvp[2];

	tvp[0].tv_sec = tvp[1].tv_sec = --mtime;
	tvp[0].tv_usec = tvp[1].tv_usec = 0;
	return(utimes(q->q_name, tvp));
}
#endif
/*
 * Checks if specified job name is in the printer's queue.
 * Returns:  negative (-1) if argument name is not in the queue.
 */
doarg(job)
	char *job;
{
	register struct queue_ **qq;
	register int jobnum, n;
	register char *cp, *machine;
	char *jobtmp;
	int cnt = 0;
	FILE *fp;

	/*
	 * Look for a job item consisting of system name, colon, number 
	 * (example: ucbarpa:114)  
	 */
	if ((cp = strchr(job, ':')) != NULL) {
		machine = job;
		*cp++ = '\0';
		job = cp;
	} else
		machine = NULL;

	/*
	 * Check for job specified by number (example: 112 or 235ucbarpa).
	 */
	jobtmp = job;
	if (isdigit(*job)) {
		jobnum = 0;
		do
			jobnum = jobnum * 10 + (*job++ - '0');
		while (isdigit(*job) && job != jobtmp+3);
		if (*job == '@')
			job++;
		for (qq = queue + nitems; --qq >= queue; ) {
			n = 0;
			for (cp = (*qq)->q_name+3; isdigit(*cp) &&
			     cp != (*qq)->q_name+6; )
				n = n * 10 + (*cp++ - '0');
			if (jobnum != n)
				continue;
			if (*job && strcmp(job, cp) != 0)
				continue;
			if (machine != NULL && strcmp(machine, cp) != 0)
				continue;
			if (touch(*qq) == 0) {
				printf("\tmoved %s\n", (*qq)->q_name);
				cnt++;
			}
		}
		return(cnt);
	}
	/*
	 * Process item consisting of owner's name (example: henry).
	 */
	for (qq = queue + nitems; --qq >= queue; ) {
		if ((fp = fopen((*qq)->q_name, "r")) == NULL)
			continue;
		while (getline(fp) > 0)
			if (line[0] == 'P')
				break;
		(void) fclose(fp);
		if (line[0] != 'P' || strcmp(job, line+1) != 0)
			continue;
		if (touch(*qq) == 0) {
			printf("\tmoved %s\n", (*qq)->q_name);
			cnt++;
		}
	}
	return(cnt);
}

/*
 * Enable everything and start printer (undo `down').
 */
up(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: up {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			startpr(2);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		startpr(2);
	}
}


/*
 * examine queue using /usr/ucb/lpq
 */
lookat(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: lookat {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;	/* hack, get printer name */
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			displayq(0);
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif /* HESIOD_LPC */
		displayq(0);
	}
}

/*
 * Remove all spool files and temporaries from the spooling area.  What
 * clean used to do.
 */
flushq_(argc, argv)
	char *argv[];
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (argc == 1) {
		printf("Usage: flush {all | printer ...}\n");
		return;
	}
	if (argc == 2 && !strcmp(argv[1], "all")) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			flushpr();
		}
		return;
	}
	while (--argc) {
		printer = *++argv;
#ifdef HESIOD_LPC
		if ((status = pgetent(line, printer)) <= 0) {
			if(pralias(alibuf, printer))
				printer = alibuf;
			if ((status = hpgetent(line, printer)) < 1) {
				printf("unknown printer %s\n", printer);
				continue;
			}
		}
#else
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
#endif HESIOD_LPC
		flushpr();
	}
}

/*
 * flush the queue (cleanpr from old version of lpc)
 */

flushpr()
{
	register int c;
	register DIR *dirp;
#ifdef POSIX
	register struct dirent *dp;
#else
	register struct direct *dp;
#endif
	char *cp, *cp1;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	for (cp = line, cp1 = SD; *cp++ = *cp1++; );
	cp[-1] = '/';
	printf("%s:\n", printer);

	if ((dirp = opendir(SD)) == NULL) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	while ((dp = readdir(dirp)) != NULL) {
		c = dp->d_name[0];
		if ((c == 'c' || c == 't' || c == 'd') && dp->d_name[1]=='f') {
			strcpy(cp, dp->d_name);
			if (UNLINK(line) < 0)
				printf("\tcannot remove %s\n", line);
			else
				printf("\tremoved %s\n", line);
		}
	}
	closedir(dirp);
  }
