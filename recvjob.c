/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)recvjob.c	5.4 (Berkeley) 6/6/86";
static char *rcsid_recvjob_c = "$Id: recvjob.c,v 1.21 1999-01-25 19:13:06 ghudson Exp $";
#endif

/*
 * Receive printer jobs from the network, queue them and
 * start the printer daemon.
 */

#include "lp.h"
#include <limits.h>

#ifdef _AUX_SOURCE
#include <sys/sysmacros.h>
#include <ufs/ufsparam.h>
#endif

#if defined(SYSV)
#define USE_USTAT
#include <ustat.h>
#elif defined(__NetBSD__)
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#elif defined(__linux__)
#define USE_STATFS
#include <sys/vfs.h>
#else
#include <ufs/fs.h>
#endif

#ifdef PQUOTA
#include "quota.h"
#include <time.h>
#include <sys/time.h>
#endif

#ifdef _IBMR2
#include <sys/select.h>
#endif

#if BUFSIZ != 1024
#undef BUFSIZ
#define BUFSIZ 1024
#endif

char	*sp = "";
#define ack()	(void) write(1, sp, 1);

int 	lflag;			/* should we log a trace? */
char    tfname[40];		/* tmp copy of cf before linking */
char    dfname[40];		/* data files */
char    cfname[40];             /* control fle - fix compiler bug */
int	minfree;		/* keep at least minfree blocks available */

#ifdef USE_USTAT
dev_t	ddev;			/* disk device (for checking free space) */
#else
char	*ddev;			/* disk device (for checking free space) */
int	dfd;			/* file system device descriptor */
#endif

#ifdef KERBEROS
char    tempfile[40];           /* Same size as used for cfname and tfname */
extern int kflag;
#endif

#ifdef _AUX_SOURCE
/* They defined fds_bits correctly, but lose by not defining this */
#define FD_ZERO(p)  ((p)->fds_bits[0] = 0)
#define FD_SET(n, p)   ((p)->fds_bits[0] |= (1 << (n)))
#define FD_ISSET(n, p)   ((p)->fds_bits[0] & (1 << (n)))
#endif

char	*find_dev();

recvjob()
{
	struct stat stb;
	char *bp = pbuf;
	int status, rcleanup();
	void *old_term, *old_pipe;

	/*
	 * Perform lookup for printer name or abbreviation
	 */
	if(lflag) syslog(LOG_INFO, "in recvjob");
#ifdef HESIOD
	if ((status = pgetent(line, printer)) <= 0) {
		if (pralias(alibuf, printer))
			printer = alibuf;
		if ((status = hpgetent(line, printer)) < 1)
			frecverr("unknown printer %s", printer);
	}
#else
	if ((status = pgetent(line, printer)) < 0) {
		frecverr("cannot open printer description file");
	}
 	else if (status == 0)
		frecverr("unknown printer %s", printer);
#endif
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;

	if (((RM = pgetstr("rm", &bp)) != NULL) && strcasecmp(RM, host)) {
#ifdef KERBEROS
	        if (require_kerberos(printer) > 0)
		       /* They are not for you */
		       frecverr("%s: cannot forward to kerberized spooler", 
				printer);
#endif KERBEROS
		SD = DEFSPOOL;
	}

#ifdef PQUOTA
	RQ = pgetstr("rq", &bp);
	QS = pgetstr("qs", &bp);
#endif
#ifdef LACL
	AC = pgetstr("ac", &bp);
	PA = pgetflag("pa");
	RA = pgetflag("ra");
#endif

	(void) close(2);			/* set up log file */
	if (open(LF, O_WRONLY|O_APPEND, 0664) < 0) {
		syslog(LOG_ERR, "%s: %m", LF);
		(void) open("/dev/null", O_WRONLY);
	}

	if (chdir(SD) < 0)
		frecverr("%s: %s: %m", printer, SD);
	if (stat(LO, &stb) == 0) {
		if (stb.st_mode & 010) {
			/* queue is disabled */
			putchar('\1');		/* return error code */
			exit(1);
		}
	} else if (stat(SD, &stb) < 0)
		frecverr("%s: %s: %m", printer, SD);
	minfree = read_number("minfree");

#ifdef USE_USTAT
	ddev = stb.st_dev;
#elif defined(USE_STATFS)
	ddev = SD;
#else
	ddev = find_dev(stb.st_dev, S_IFBLK);
	if ((dfd = open(ddev, O_RDONLY)) < 0)
		syslog(LOG_WARNING, "%s: %s: %m", printer, ddev);
#endif


	old_term = signal(SIGTERM, rcleanup);
	old_pipe = signal(SIGPIPE, rcleanup);

	if(lflag) syslog(LOG_INFO, "Reading job");

	if (readjob())
	  {
	    signal(SIGTERM, old_term);
	    signal(SIGPIPE, old_pipe);

	    if (lflag) syslog(LOG_INFO, "Printing job..");

	    printjob();
	  }
	
}

#if !defined(USE_USTAT) && !defined(USE_STATFS)
char *
find_dev(dev, type)
	register dev_t dev;
	register int type;
{
	register DIR *dfd = opendir("/dev");
#ifdef POSIX
	struct dirent *dir;
#else
	struct direct *dir;
#endif
	struct stat stb;
	char devname[MAXNAMLEN+6];
	char *dp;

	strcpy(devname, "/dev/");
	while ((dir = readdir(dfd))) {
		strcpy(devname + 5, dir->d_name);
		if (stat(devname, &stb))
			continue;
		if ((stb.st_mode & S_IFMT) != type)
			continue;
		if (dev == stb.st_rdev) {
			closedir(dfd);
			dp = (char *)malloc(strlen(devname)+1);
			if (!dp)
				frecverr("out of memory while finding device %d, %d", major(dev), minor(dev));
			strcpy(dp, devname);
			return(dp);
		}
	}
	closedir(dfd);
	frecverr("cannot find device %d, %d", major(dev), minor(dev));
	/*NOTREACHED*/
}
#endif /* USE_USTAT */

/* Compute the next job number */
jobnum()
{
    FILE *jfp;
    int job = 0;

    jfp = fopen(".seq", "r");
    if (jfp) {
      fscanf(jfp, "%d", &job);
      job = (job+1) % 1000;
      fclose(jfp);
    }

    jfp = fopen(".seq", "w");
    if (jfp) {
      fprintf(jfp, "%d\n", job);
      fclose(jfp);
    }
    return job;
}

/*
 * Read printer jobs sent by lpd and copy them to the spooling directory.
 * Return the number of jobs successfully transfered.
 */
readjob()
{
	register int size, nfiles;
	int n, toobig;
	register char *cp;
#if defined(PQUOTA) || defined(LACL)
	char *cret;
#endif
#ifdef PQUOTA
	char *check_quota();
#endif
#ifdef LACL
	char *check_lacl(), *check_remhost();
#endif

	if (lflag) syslog(LOG_INFO, "In readjob");

	ack();
	nfiles = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		n = 0;
		toobig = 0;
		do {
			if (++n >= sizeof(line)) {
				frecverr("%s: line too long", printer);
				return(nfiles);
			}
			if ((size = read(1, cp, 1)) != 1) {
				if (size < 0)
					frecverr("%s: Lost connection",printer);
				if (lflag) syslog(LOG_INFO, "Returning from readjobs");
				return(nfiles);
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			rcleanup();
			continue;

		case '\2':	/* read cf file */
			size = 0;
			while (*cp >= '0' && *cp <= '9') {
				if (size > INT_MAX/10)
					toobig = 1;
				size = size * 10 + (*cp++ - '0');
			}
			if (*cp++ != ' ' || toobig)
				break;

			if (!isalnum(cp[0]) || !isalnum(cp[1]) ||
			    !isalnum(cp[2]) || !isalnum(cp[3]) ||
			    !isalnum(cp[4]) || !isalnum(cp[5])) {
				syslog(LOG_ERR,
			   "unexpected job character in cf %d.%d.%d.%d.%d.%d",
				       cp[0], cp[1], cp[2], cp[3],
				       cp[4], cp[5]);
				break;
			}
			if (cp[0] != 'c' || cp[1] != 'f' ||
			    cp[2] != 'A' || !isxdigit(cp[3]) ||
			    !isxdigit(cp[4]) || !isxdigit(cp[5])) {
				syslog(LOG_NOTICE,
			   "nonstandard job character in cf %d.%d.%d.%d.%d.%d",
				       cp[0], cp[1], cp[2], cp[3],
				       cp[4], cp[5]);
			}

			/* Compute local filenames. */
			sprintf(cfname, "cfA%03d%s", jobnum(), from);
			strncpy(tfname, cp, sizeof(tfname));
			tfname[sizeof(tfname) - 1] = '\0';
                        if (strchr(cfname, '/') || strchr(tfname, '/'))
                          {
                            syslog(LOG_ERR,
                                   "/ found in control or temporary file name");
                            break;
                          }
			tfname[0] = 't';
#ifdef KERBEROS
			strcpy(tempfile, tfname);
			tempfile[0] = 'T';
#endif
			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			} 
			    
			/* Don't send final acknowledge beacuse we may wish to 
			   send error below */
			if (!readfile(tfname, size, 0, 't')) {
			    syslog(LOG_DEBUG, "Failed read");
				rcleanup();
				continue;
			}

			if (!fixup_cf(tfname, tempfile)) {
				rcleanup();
				continue;
			}

#ifdef LACL
			if(RA && ((cret = check_remhost()) != 0)) {
			    (void) write(1, cret, 1);
			    rcleanup();
			    continue;
			}

			if(AC && (cret = check_lacl(tfname)) != 0) {
			    /* We return !=0 for error. Old clients stupidly
			     * don't print any error in this situation.
			     * We do a cleanup cause we can't expect 
			     * client to do so. */
			    (void) write(1, cret, 1);
#ifdef DEBUG
			    syslog(LOG_DEBUG, "Got %s", cret);
#endif
			    rcleanup();
			    continue;
			}
#endif /*LACL*/

#ifdef PQUOTA
			if(kerberos_cf && (RQ != NULL) && 
			   (cret = check_quota(tfname)) != 0) {
			    /* We return !=0 for error. Old clients
			       stupidly don't print any error in this sit.
			       We do a cleanup cause we can't expect 
			       client to do so. */
			    (void) write(1, cret, 1);
#ifdef DEBUG
			    syslog(LOG_DEBUG, "Got %s", cret);
#endif
			    rcleanup();
			    continue;
			}
#endif /* PQUOTA */

			/* Send acknowldege, cause we didn't before */
			ack();

			if (link(tfname, cfname) < 0) 
				frecverr("%s: %m", tfname);

			(void) spool_unlink(tfname, 't', 1);  
			tfname[0] = '\0';
			nfiles++;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (*cp >= '0' && *cp <= '9') {
                        	if (size > INT_MAX/10) toobig = 1;
				size = size * 10 + (*cp++ - '0');
			}
			if (*cp++ != ' ' || toobig)
				break;

			if (!isalnum(cp[0]) || !isalnum(cp[1]) ||
			    !isalnum(cp[2]) || !isalnum(cp[3]) ||
			    !isalnum(cp[4]) || !isalnum(cp[5])) {
				syslog(LOG_ERR,
			   "unexpected job character in df %d.%d.%d.%d.%d.%d",
				       cp[0], cp[1], cp[2], cp[3],
				       cp[4], cp[5]);
				break;
			}
                        if (cp[0] != 'd' || cp[1] != 'f' ||
                            cp[2] != 'A' || !isxdigit(cp[3]) ||
                            !isxdigit(cp[4]) || !isxdigit(cp[5])) {
				syslog(LOG_NOTICE,
                           "nonstandard job character in df %d.%d.%d.%d.%d.%d",
                                       cp[0], cp[1], cp[2], cp[3],
                                       cp[4], cp[5]);
			}

			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			}

			(void) strncpy(dfname, cp, sizeof(dfname));
			dfname[sizeof(dfname) - 1] = '\0';
			if (strchr(dfname, '/'))
				frecverr("illegal path name");
			(void) readfile(dfname, size, 1, 'd');
			continue;
		}
		frecverr("protocol screwup");
	}
}

/*
 * Read files send by lpd and copy them to the spooling directory.
 */
readfile(file, size, acknowledge, flag)
	char *file;
	int size;
        int acknowledge;
	int flag;
{
	register char *cp;
	char buf[BUFSIZ];
	register int i, j, amt;
	int fd, err;

	fd = open(file, O_WRONLY|O_CREAT, FILMOD);
	if (fd < 0)
		frecverr("%s: %m", file);
	ack();
	err = 0;
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0)
				frecverr("Lost connection");
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
		}
	}
	(void) close(fd);
	if (err)
		frecverr("%s: write error", file);
	if (noresponse()) {		/* file sent had bad data in it */
		(void) spool_unlink(file, flag, 1); 
		return(0);	
	    }
	if(acknowledge)
	    ack();

	return(1);
}

fixup_cf(file, tfile)
char *file, *tfile;
{
	FILE *cfp, *tfp;
	char kname[ANAME_SZ + INST_SZ + REALM_SZ + 3];
	char oldname[ANAME_SZ + INST_SZ + REALM_SZ + 3];

	oldname[0] = '\0';

	/* Form a complete string name consisting of principal, 
	 * instance and realm
	 */
	make_kname(kprincipal, kinstance, krealm, kname);

	/* If we cannot open tf file, then return error */
	if ((cfp = fopen(file, "r")) == NULL)
		return (0);

	/* Read the control file for the person sending the job */
	while (getline(cfp)) {
		if (line[0] == 'P') {
			strncpy(oldname, line+1, sizeof(oldname)-1);
			break;
		}
	}
	fclose(cfp);

	/* Have we got a name in oldname, if not, then return error */
	if (oldname[0] == '\0')
		return(0);

	/* Does kname match oldname. If so do nothing */
	if (!strcmp(kname, oldname))
		return(1); /* all a-okay */

	/* hmm, doesnt match, guess we have to change the name in
	 * the control file by doing the following :
	 *
	 * (1) Move 'file' to 'tfile'
	 * (2) Copy all of 'tfile' back to 'file' but
	 *     changing the persons name
	 */
	if (link(file, tfile) < 0)
		return(0);
	(void) spool_unlink(file, 't', 1); 

	/* If we cannot open tf file, then return error */
	if ((tfp = fopen(tfile, "r")) == NULL)
		return (0);
	if ((cfp = fopen(file, "w")) == NULL) {
		(void) fclose(tfp);
		return (0);
	}

	while (getline(tfp)) {
		/* Ignore any queue name that was specified in the original
		 * .cf file -- we'll be adding it in shortly...
		 */
		if (line[0] == 'q') continue;

#ifdef KERBEROS
		if (kerberos_cf) {
		    if (line[0] == 'P')
			strcpy(&line[1], kname);
		    else if (line[0] == 'L')
			strcpy(&line[1], kname);
		}
#endif
		fprintf(cfp, "%s\n", line);

	}
	fprintf(cfp, "q%s\n", printer);

	(void) fclose(cfp);
	(void) fclose(tfp);
	(void) spool_unlink(tfile, 'T', 1); 

	return(1);
}

noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		frecverr("Lost connection");
	if (resp == '\0')
		return(0);
	return(1);
}

/*
 * Check to see if there is enough space on the disk for size bytes.
 * 1 == OK, 0 == Not OK.
 */
chksize(size)
	int size;
{
#ifdef USE_USTAT
	struct ustat ubuf;

	if (ustat(ddev, &ubuf)) return 1;
	size = (size+1023)/1024;
	if (minfree + size > ubuf.f_tfree) return 0;
	return 1;
#elif defined(USE_STATFS)
	struct statfs buf;
	if (statfs(ddev, &buf)) return 1;
	size = (size+1023)/1024;
	if (minfree + size > buf.f_bavail) return 0;
	return 1;
#else
	int spacefree;
	struct fs fs;

	if (dfd < 0 || lseek(dfd, (long)(SBLOCK * DEV_BSIZE), 0) < 0)
		return(1);
	if (read(dfd, (char *)&fs, sizeof fs) != sizeof fs)
		return(1);
	spacefree = (fs.fs_cstotal.cs_nbfree * fs.fs_frag +
		fs.fs_cstotal.cs_nffree - fs.fs_dsize * fs.fs_minfree / 100) *
			fs.fs_fsize / 1024;
	size = (size + 1023) / 1024;
	if (minfree + size > spacefree)
		return(0);
	return(1);
#endif
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(fn, "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}

/*
 * Remove all the files associated with the current job being transfered.
 */
rcleanup()
{

	/* This was cretinous code.. which regularly walked off the end
	 * of the name space...  I changed the != to a >=..
	 */

	if (tfname[0])
		(void) spool_unlink(tfname, 't', 1); 
#ifdef KERBEROS
	if (tempfile[0])
		(void) spool_unlink(tempfile, 'T', 1); 
#endif
	if (dfname[0])
		do {
			do
				if ((spool_unlink(dfname, 'd', 0)) == -2)
					return;
			while (dfname[2]-- >= 'A');
			dfname[2] = 'z';
		} while (dfname[0]-- >= 'd');
	dfname[0] = '\0';
}

/* VARARGS1 */
frecverr(msg, a1, a2)
	char *msg;
{
	rcleanup();
	syslog(LOG_ERR, msg, a1, a2);
	putchar('\1');		/* return error code */
	exit(1);
}

#ifdef PQUOTA

char* check_quota(file)
char file[];
    {
        struct hostent *hp;
	char outbuf[BUFSIZ], inbuf[BUFSIZ];
	int t, act=0, s1;
	FILE *cfp;
	struct sockaddr_in sin_c;
	int fd, retry;
	struct servent *servname;
	struct timeval tp;
	fd_set set;

	if(RQ == NULL) 
	    return 0;
	if((hp = gethostbyname(RQ)) == NULL) {
	    syslog(LOG_WARNING, "Cannot resolve quota servername %s", RQ);
	    return 0;
	}

	/* Setup output buffer.... */
	outbuf[0] = QUOTA_QUERY_VERSION;
	outbuf[1] = (char) UDPPROTOCOL;

	/* Generate a sequence number... Since we fork the only realistic
	   thing to use is the time... */
	t = htonl(time((char *) 0));
	memcpy(outbuf + 2, &t,  4);
	strncpy(outbuf + 5, printer, 30);


	if(QS == NULL) 
	    outbuf[40] = '\0';
	else 
	    strncpy(outbuf + 40, QS, 20);
	/* If can't open the control file, then there is some error...
	   We'll return allowed to print, but somewhere else it will be caught.
	   Is this proper? XXX
	   */

	if ((cfp = fopen(file, "r")) == NULL)
		return 0;

	/* Read the control file for the person sending the job */
	while (getline(cfp)) {
		if (line[0] == 'Q' || line[0] == 'A') { /* 'A' for old clients */
		    if(sscanf(line + 1, "%d", &act) != 1) act=0;
		    break;
		}
	}
	fclose(cfp);

       	act = htonl(act);
	memcpy(outbuf + 36, &act,  4);

	strncpy(outbuf + 60, kprincipal, ANAME_SZ);
	strncpy(outbuf + 60 + ANAME_SZ, kinstance, INST_SZ);
	strncpy(outbuf + 60 + ANAME_SZ + INST_SZ, krealm, REALM_SZ);

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	    syslog(LOG_WARNING, "Could not create UDP socket\n");
	    /* Allow print */
	    return 0;
	}

	memset((char *)&sin_c, 0, sizeof(sin_c));
	sin_c.sin_family = AF_INET;
	servname = getservbyname(QUOTASERVENTNAME,"udp");
	if(!servname) 
	    sin_c.sin_port = htons(QUOTASERVENT);
	else 
	    sin_c.sin_port = servname->s_port;

	memcpy(&sin_c.sin_addr,hp->h_addr_list[0], sizeof(sin_c.sin_addr));

	if(connect(fd, &sin_c, sizeof(sin_c)) < 0) {
	    syslog(LOG_WARNING, "Could not connect with UDP - quota server down?");
	    /* This means that the quota serve is down */
	    /* Allow printing */
	    return 0;
	}

	for(retry = 0; retry < RETRY_COUNT; retry++) {
	    if(send(fd, outbuf, 59+ANAME_SZ+REALM_SZ+INST_SZ+1,0)<
	       59+ANAME_SZ+REALM_SZ+INST_SZ+1) {
		syslog(LOG_WARNING, "Send failed to quota");
		continue;
	    }

	    FD_ZERO(&set);
	    FD_SET(fd, &set);
	    tp.tv_sec = UDPTIMEOUT;
	    tp.tv_usec = 0;

	    /* So, select and wait for reply */
	    if((s1=select(fd+1, &set, 0, 0, &tp))==0) {
		/*Time out, retry */
		continue;
	    }

	    if(s1 < 0) {
		/* Error, which makes no sense. Oh well, display */
		syslog(LOG_WARNING, "Error in UDP return errno=%d", errno);
		/* Allow print */
		return 0;
	    }

	    if((s1=recv(fd, inbuf, 36, 0)) != 36) {
		syslog(LOG_WARNING, "Receive error in UDP contacting quota");
		/* Retry */
		continue;
	    }

	    if(memcmp(inbuf, outbuf, 36)) {
		/* Wrong packet */
#ifdef DEBUG
		syslog(LOG_DEBUG, "Packet not for me on UDP");
#endif
		continue;
	    }

	    /* Packet good, send response */
	    switch ((int) inbuf[36]) {
	    case ALLOWEDTOPRINT:
#ifdef DEBUG
		syslog(LOG_DEBUG, "Allowed to print!!");
#endif
		return 0;
	    case NOALLOWEDTOPRINT:
		return "\4";
	    case UNKNOWNUSER:
		return "\3";
	    case UNKNOWNGROUP:
		return "\5";
	    case USERNOTINGROUP:
		return "\6";
	    case USERDELETED:
		return "\7";
	    case GROUPDELETED:
		return "\10";
	    default:
		break;
		/* Bogus, retry */
	    }
	}

	if(retry == RETRY_COUNT) {
	    /* We timed out in contacting... Allow printing*/
	    return 0;
	}
	return 0;
    }

#endif

#ifdef LACL
char *check_lacl(file)
char *file;
    {
	FILE *cfp;
	char person[BUFSIZ];
#ifdef KERBEROS
	extern char local_realm[];
#endif
	person[0] = '\0';

	if(!AC) {
	    syslog(LOG_ERR, "ACL file not set in printcap");
	    return NULL;
	}
	if(access(AC, R_OK)) {
	    syslog(LOG_ERR, "Could not find ACL file %s", AC);
	    return NULL;
	}

	/* Read the control file for the person sending the job */
	if ((cfp = fopen(file, "r")) == NULL)
		return 0;
	while (getline(cfp)) {
		if (line[0] == 'P' && line[1]) {
		    strcpy(person, line + 1);
		    break;
		}
	}
	fclose(cfp);

	if(!person[0]) {
#ifdef DEBUG
	    syslog(LOG_DEBUG, "Person not found :%s", line);
#endif
	    goto notfound;
	}
#ifdef DEBUG
	else 
	    syslog(LOG_DEBUG, "Found person :%s:%s", line, person);
#endif

#ifdef KERBEROS
	/* Now to tack the realm on */
	if(kerberos_cf && !strchr(person, '@')) {
	    strcat(person,"@");
	    strcat(person, local_realm);
	}

#endif

#ifdef DEBUG
	syslog(LOG_DEBUG, "Checking on :%s: ", person);
#endif

	/* Now see if the person is in AC */
	if ((cfp = fopen(AC, "r")) == NULL)
		return 0;
	while(getline(cfp)) {
	    if(!strcasecmp(person, line)) {
		fclose(cfp);
		goto found;
	    }
	}
	fclose(cfp);

    notfound:
	if(PA) return "\4"; /* NOALLOWEDTOPRINT */
	else return NULL;

    found:
	if(PA) return NULL;
	else return "\4"; /* NOALLOWEDTOPRINT */
    }

char *
check_remhost()
{
    register char *cp, *sp;
    extern char from_host[];
    register FILE *hostf;
    char ahost[MAXHOSTNAMELEN + 1];
    int baselen = -1;

    if(!strcasecmp(from_host, host)) return NULL;
#if 0
    syslog(LOG_DEBUG, "About to check on %s\n", from_host);
#endif
    sp = from_host;
    cp = ahost;
    while (*sp) {
	if (*sp == '.') {
	    if (baselen == -1)
		baselen = sp - from_host;
	    *cp++ = *sp++;
	} else {
	    *cp++ = isupper(*sp) ? tolower(*sp++) : *sp++;
	}
    }
    *cp = '\0';
    hostf = fopen("/etc/hosts.lpd", "r");
#define DUMMY ":nobody::"
    if (hostf) {
	if (!validuser(hostf, ahost, DUMMY, DUMMY, baselen)) {
	    (void) fclose(hostf);
	    return NULL;
	}
	(void) fclose(hostf);
	return "\4";
    } else {
	/* Could not open hosts.lpd file */
	return NULL;
    }
}
#endif /* LACL */
