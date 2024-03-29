/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)startdaemon.c	5.1 (Berkeley) 6/6/85";
#endif not lint

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include "lp.local.h"

startdaemon(printer)
	char *printer;
{
	struct sockaddr_un sockun;
	register int s, n;
	char buf[BUFSIZ];

	syslog(LOG_DEBUG, "Attempting to restart printer %s", printer);
	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		perr("socket");
		return(0);
	}
	sockun.sun_family = AF_UNIX;
	strcpy(sockun.sun_path, SOCKETNAME);
	if (connect(s, &sockun, strlen(sockun.sun_path) + 2) < 0) {
		perr("connect");
		(void) close(s);
		return(0);
	}
	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return(0);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void) close(s);
			return(1);
		}
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof(buf))) > 0)
		fwrite(buf, 1, n, stdout);
	(void) close(s);
	return(0);
}

static
perr(msg)
	char *msg;
{
	extern char *name;

	syslog(LOG_DEBUG, "%s: %s: %m", name, msg);
	printf("%s: %s: ", name, msg);
	puts(strerror(errno));
}
