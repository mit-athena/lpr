/*
 *	$Source: /afs/dev.mit.edu/source/repository/athena/bin/lpr/quota/quota.h,v $
 *	$Author: epeisach $
 *	$Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/quota/quota.h,v 1.6 1991-01-23 15:12:24 epeisach Exp $
 */

/*
 * Copyright (c) 1990 by the Massachusetts Institute of Technology.
 * For copying and distribution information, see the file "mit-copyright.h."
 */

#include "mit-copyright.h"
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <strings.h>

#define UDPPROTOCOL	1
#define RETRY_COUNT	2
#define UDPTIMEOUT	3

extern char *progname;

extern short KA;    /* Kerberos Authentication */
extern short MA;    /* Mutual Authentication   */
extern int   DQ;    /* Default quota */
extern char *AF;    /* Acl File */
extern char *QF;    /* Master quota file */
extern char *GF;    /* Group quota file */
extern char *RF;    /* Report file for logger to grok thru */
extern char *QC;    /* Quota currency */
extern int   QD;    /* Quota server "shutdown" for maintainence */

extern char pbuf[]; /* Dont ask :) -Ilham */
extern char *bp;

char *malloc();
extern char 	*error_text();

#define TRUE 1
#define FALSE 0

#define ALLOWEDTOPRINT     0
#define NOALLOWEDTOPRINT   1
#define UNKNOWNUSER        2
#define UNKNOWNGROUP       3
#define USERNOTINGROUP     4
#define USERDELETED        5
#define GROUPDELETED       6

#ifdef __STDC__
extern void	make_kname(char *, char *, char *, char *);
extern int	is_suser(char *);
extern int	is_sacct(char *, char *);
extern int	parse_username(unsigned char *, char *, char *, char *);
extern char	*set_service(char *);
#else
extern void	make_kname();
extern int	is_suser();
extern int	is_sacct();
extern int	parse_username();
extern char	*set_service();
#endif
