/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)displayq.c	5.1 (Berkeley) 6/6/85";
#endif not lint

/*
 * Routines to display the state of the queue.
 */

#include "lp.h"
#if defined(POSIX) && !defined(ultrix)
#include "posix.h"   /* for flock() */
#endif
#define JOBCOL	40		/* column for job # in -l format */
#define OWNCOL	7		/* start of Owner column in normal */
#define SIZCOL	62		/* start of Size column in normal */

/*
 * Stuff for handling job specifications
 */
extern char	*user[];	/* users to process */
extern int	users;		/* # of users in user array */
extern int	requ[];		/* job number of spool entries */
extern int	requests;	/* # of spool requests */

int	lflag;		/* long output option */
char	current[40];	/* current file being printed */
int	garbage;	/* # of garbage cf files */
int	rank;		/* order to be printed (-1=none, 0=active) */
long	totsize;	/* total print job size in bytes */
int	first;		/* first file in ``files'' column? */
int	col;		/* column on screen */
int	sendtorem;	/* are we sending to a remote? */
char	file[132];	/* print file name */

char	*head0 = "Rank   Owner      Job  Files";
char	*head1 = "Total Size\n";
char	restart_succeed[] =
	"%s daemon %d does not exist; restarting a new one....";
char	restart_fail[] =
	"%s daemon %d does not exist; could not start a new daemon!";

/*
 * Display the current state of the queue. Format = 1 if long format.
 */
displayq(format)
	int format;
{
	register struct queue_ *q;
	register int i, nitems, fd;
	struct queue_ **queue;
	struct stat statb;
	int rem_fils;
	char *tmpptr;
	FILE *fp;
#ifdef KERBEROS
	short KA;
#endif
#if defined(PQUOTA) && defined(KERBEROS)
	int pagecost;
#endif
	lflag = format;
	totsize = 0;
	rank = -1;
	rem_fils = 0;

#ifdef HESIOD
	if ((i = pgetent(line, printer)) <= 0) {
		if (pralias(alibuf, printer))
			printer = alibuf;
		if ((i = hpgetent(line, printer)) < 1)
			fatal("unknown printer");
	}
#else
	if ((i = pgetent(line, printer)) < 0) {
		fatal("cannot open printer description file");
	} else if (i == 0)
		fatal("unknown printer");
#endif HESIOD
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	RM = pgetstr("rm", &bp);
#ifdef KERBEROS
	KA = pgetnum("ka");
#endif
#ifdef PQUOTA
	RQ = pgetstr("rq", &bp);
#endif PQUOTA
#if defined(PQUOTA) && defined(KERBEROS)
	pagecost = pgetnum("pc");
#endif
	/*
	 * Figure out whether the local machine is the same as the remote 
	 * machine entry (if it exists).  If not, then ignore the local
	 * queue information.
	 */

	 if (RM != (char *) NULL) {

		char name[MAXHOSTNAMELEN + 1];
		struct hostent *hp;

			/* get the name of the local host */
		gethostname (name, sizeof(name) - 1);
		name[sizeof(name)-1] = '\0';
			/* get the network standard name of the local host */
		hp = gethostbyname (name);
		if (hp == (struct hostent *) NULL) {
		    printf ("unable to get hostname for local machine %s\n",
				name);
		} 
		else {
		  strncpy (name, hp->h_name, sizeof(name));
		  name[sizeof(name) - 1] = '\0';
		  /* get the network standard name of RM */
		  hp = gethostbyname (RM);
		  if (hp == (struct hostent *) NULL) {
		    printf ("unable to get hostname for remote machine %s\n",
			    RM);
		  }
		  /* if printer is not on local machine, ignore LP */
		  else if (strcasecmp (name, hp->h_name) != 0)
		      LP = "";
		}
	}

	/*
	 * If there is no local printer, then print the queue on
	 * the remote machine and then what's in the queue here.
	 * Note that a file in transit may not show up in either queue.
	 */
	if (*LP == '\0') {
		register char *cp;

		sendtorem++;
		(void) sprintf(line, "%c%s", format + '\3', RP);
		cp = line;
		for (i = 0; i < requests; i++) {
			cp += strlen(cp);
			(void) sprintf(cp, " %d", requ[i]);
		}
		for (i = 0; i < users; i++) {
			cp += strlen(cp);
			*cp++ = ' ';
			strcpy(cp, user[i]);
		}
		strcat(line, "\n");
		fd = getport(RM);
		if (fd < 0) {
			if (from != host)
				printf("%s: ", host);
			printf("unable to connect to %s (for %s)\n", RM, RP);
		} else {
		        printf("%s...  ", RM); fflush(stdout);
			i = strlen(line);
			if (write(fd, line, i) != i)
				fatal("Lost connection");
			rem_fils = -1;
			while ((i = read(fd, line, sizeof(line))) > 0) {
				(void) fwrite(line, 1, i, stdout);
				for (tmpptr = line;
				     tmpptr = strchr(tmpptr,'\n'); ) {
					rem_fils++;
					tmpptr++;
				}
			}
			(void) close(fd);
		}
	}
	/*
	 * Allow lpq -l info about printing
	 */
	else if (lflag) {
#ifdef KERBEROS
	    if(KA > 0) {
		printf("\nKerberos authenticated");
#ifndef PQUOTA
		putchar('\n');
#endif PQUOTA	
	    }    
#endif KERBEROS

#ifdef PQUOTA
	    if((RQ != (char *) NULL)) {
		printf("\nQuota server: %s\n", RQ);
	    }
#ifdef KERBEROS
	    else putchar('\n');
#endif KERBEROS
#if defined(PQUOTA) && defined(KERBEROS)
	    if (pagecost > 0) printf("Page cost: %d cents\n", pagecost);
#endif
#endif
	}
	/*
	 * Find all the control files in the spooling directory
	 */
	if (chdir(SD) < 0) {
	        char msgbuf[(2 * BUFSIZ) + 255];

		if (RM) return(rem_fils);
		sprintf(msgbuf,
			"Cannot chdir to spooling directory %s for %s",
			SD, printer);
		fatal(msgbuf);
		}
	if ((nitems = getq_(&queue)) < 0) {
	  	char msgbuf[(2 * BUFSIZ) + 255];
		sprintf(
			msgbuf,
			"Cannot examine spooling area %s for %s",
			SD, printer);
		fatal (msgbuf);
	        }
	if (stat(LO, &statb) >= 0) {
		if ((statb.st_mode & 0110) && sendtorem)
			printf("\n");
		if (statb.st_mode & 0100) {
			if (sendtorem)
				printf("%s: ", host);
			printf("Warning: %s is down: ", printer);
			fd = open(ST, O_RDWR);
			if (fd >= 0) {
			        char tmp[1024];
				(void) flock(fd, LOCK_SH);
				while ((i = read(fd, line, sizeof(line))) > 0){
				     strncpy(tmp, printer, sizeof(tmp));
				     tmp[sizeof(tmp) - 1] = '\0';
                                     if (strncmp(line, strcat(tmp," is ready and printing"), 24) != 0)
					(void) fwrite(line, 1, i, stdout);
				     else
					putchar('\n');
				 }
			       	(void) close(fd);	/* unlocks as well */
			} else
				putchar('\n');
		}
		if (statb.st_mode & 010) {
			if (sendtorem)
				printf("%s: ", host);
			printf("Warning: %s queue is turned off\n", printer);
		}
	}
	if (nitems == 0) {
		if (!sendtorem)
			printf("no entries in %s\n", printer);
		return(rem_fils);
	}
	fp = fopen(LO, "r");
	if (fp == NULL) {
	        char msgbuf[(2 * BUFSIZ) + 255];
		sprintf(msgbuf, "Unable to lock file %s/%s for %s",
			SD, LO, printer);
		warn(msgbuf);
	        }
	else {
		register char *cp;
		int icp;

		/* get daemon pid */
		cp = current;
		icp = 0;
		while ((*cp = getc(fp)) != EOF && *cp != '\n') {
			if (icp == sizeof(current) - 1) break;
			cp++;
			icp++;
		}
		*cp = '\0';
		i = atoi(current);
		if (i <= 0 || kill(i, 0) < 0) {
			char	msg[256];

			if (startdaemon(printer))
				sprintf(msg, restart_succeed, printer, i);
			else
				sprintf(msg, restart_fail, printer, i);
			warn(msg);
		}
		else {
			/* read current file name */
			cp = current;
			icp = 0;
			while ((*cp = getc(fp)) != EOF && *cp != '\n') {
				if (icp == sizeof(current) - 1) break;
				cp++;
				icp++;
			}
			*cp = '\0';
			/*
			 * Print the status file.
			 */
			if (sendtorem)
				printf("\n%s: ", host);
			fd = open(ST, O_RDWR);
			if (fd >= 0) {
				(void) flock(fd, LOCK_SH);
				while ((i = read(fd, line, sizeof(line))) > 0)
					(void) fwrite(line, 1, i, stdout);
				(void) close(fd);	/* unlocks as well */
			} else
				putchar('\n');
		}
		(void) fclose(fp);
	}
	/*
	 * Now, examine the control files and print out the jobs to
	 * be done for each user.
	 */
	if (!lflag)
		header();

	for (i = 0; i < nitems; i++) {
		q = queue[i];
		inform(q->q_name);
		free(q);
	}
	free(queue);
	return(nitems-garbage+rem_fils);
}

/*
 * Print a warning message if there is no daemon present.
 */
warn(msgbuf)
     char *msgbuf;
{
	if (sendtorem)
		printf("\n%s: ", host);
	printf("Warning: no daemon present\n[%s]\n", msgbuf);
	current[0] = '\0';
}

/*
 * Print the header for the short listing format
 */
header()
{
	printf(head0);
	col = strlen(head0)+1;
	blankfill(SIZCOL);
	printf(head1);
}

inform(cf)
	char *cf;
{
	register int j;
	FILE *cfp;
	char jobnum[4];

	/*
	 * There's a chance the control file has gone away
	 * in the meantime; if this is the case just keep going
	 */
	if ((cfp = fopen(cf, "r")) == NULL)
		return;

	if (rank < 0)
		rank = 0;
	if (sendtorem || garbage || strcmp(cf, current))
		rank++;
	j = 0;
	strncpy(jobnum,cf+3,3);
	jobnum[3] = '\0';
	while (getline(cfp)) {
		switch (line[0]) {
		case 'P': /* Was this file specified in the user's list? */
			if (!inlist(line+1, cf)) {
				fclose(cfp);
				return;
			}
			if (lflag) {
				printf("\n%s: ", line+1);
				col = strlen(line+1) + 2;
				prank(rank);
				blankfill(JOBCOL);
				printf(" [job %s@%s]\n", jobnum, cf+6);
			} else {
				col = 0;
			    	prank(rank);
				blankfill(OWNCOL);
				printf("%-10s %-3d  ", line+1, atoi(jobnum));
				col += 16;
				first = 1;
			}
			continue;
		default: /* some format specifer and file name? */
			if (line[0] < 'a' || line[0] > 'z')
				continue;
			if (j == 0 || strcmp(file, line+1) != 0) {
				strncpy(file, line+1, sizeof(file));
				file[sizeof(file) - 1] = '\0';
			}
			j++;
			continue;
		case 'N':
			show(line+1, file, j);
			file[0] = '\0';
			j = 0;
		}
	}
	fclose(cfp);
	if (!lflag) {
		blankfill(SIZCOL);
		printf("%ld bytes\n", totsize);
		totsize = 0;
	}
}

inlist(name, file)
	char *name, *file;
{
	register int *r, n;
	register char **u, *cp;

	if (users == 0 && requests == 0)
		return(1);
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, name))
			return(1);
	/*
	 * Check the request list
	 */
	for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && !strcmp(cp, from))
			return(1);
	return(0);
}

show(nfile, file, copies)
	register char *nfile, *file;
{
	if (strcmp(nfile, " ") == 0)
		nfile = "(standard input)";
	if (lflag)
		ldump(nfile, file, copies);
	else
		dump(nfile, file, copies);
}

/*
 * Fill the line with blanks to the specified column
 */
blankfill(n)
	register int n;
{
	while (col++ < n)
		putchar(' ');
}

/*
 * Give the abbreviated dump of the file names
 */
dump(nfile, file, copies)
	char *nfile, *file;
{
	register short n, fill;
	struct stat lbuf;

	/*
	 * Print as many files as will fit
	 *  (leaving room for the total size)
	 */
	 fill = first ? 0 : 2;	/* fill space for ``, '' */
	 if (((n = strlen(nfile)) + col + fill) >= SIZCOL-4) {
		if (col < SIZCOL) {
			printf(" ..."), col += 4;
			blankfill(SIZCOL);
		}
	} else {
		if (first)
			first = 0;
		else
			printf(", ");
		printf("%s", nfile);
		col += n+fill;
	}
	if (*file && !stat(file, &lbuf))
		totsize += copies * lbuf.st_size;
}

/*
 * Print the long info about the file
 */
ldump(nfile, file, copies)
	char *nfile, *file;
{
	struct stat lbuf;

	putchar('\t');
	if (copies > 1)
		printf("%d copies of %-19s%s", copies, nfile,
		       copies<10?" ":"");
	else
		printf("%-32s", nfile);
	if (*file && !stat(file, &lbuf))
		printf(" %ld bytes", lbuf.st_size);
	else
		printf(" ??? bytes");
	putchar('\n');
}

/*
 * Print the job's rank in the queue,
 *   update col for screen management
 */
prank(n)
{
	char line[100];
	static char *r[] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	if (n == 0) {
		printf("active");
		col += 6;
		return;
	}
	if ((n/10) == 1)
		(void) sprintf(line, "%dth", n);
	else
		(void) sprintf(line, "%d%s", n, r[n%10]);
	col += strlen(line);
	printf("%s", line);
}
