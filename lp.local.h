/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lp.local.h	5.1 (Berkeley) 6/6/85
 */

/*
 * Possibly, local parameters to the spooling system
 */

/*
 * Magic number mapping for binary files, used by lpr to avoid
 *   printing objects files.
 */
#include <sys/types.h>

#ifndef SOLARIS
#include <a.out.h>
#else
#include <sys/exechdr.h>
#endif
#include <ar.h>

#if defined(AIX) && defined(_I386) && !defined(OMAGIC)
#	define OMAGIC	MAG_OVERLAY
#	define NMAGIC	MAG_NSHWRT
#	define ZMAGIC	MAG_SHROT
#	define A_MAGIC4 MAG_SHROTSEP
/* We still need more? Or lib's? */
#endif

#if defined(_IBMR2) && !defined(OMAGIC)
#define LP_COFF_DEFINES
#define ISCOFF(x) (((x) == U802WRMAGIC) || ((x) == U802ROMAGIC) || ((x) == U802TOCMAGIC))
#endif

#if defined(_AUX_SOURCE) 
#define LP_COFF_DEFINES
#define ISCOFF(x) (((x) == M68TVMAGIC) || ((x) == M68MAGIC) || \
	((x) == MC68TVMAGIC) || ((x) == MC68MAGIC) || ((x) == M68NSMAGIC))
#endif

#if defined(ultrix) && defined(mips)
#define LP_COFF_DEFINES
#endif


#ifndef A_MAGIC1	/* must be a VM/UNIX system */
#	define A_MAGIC1	OMAGIC
#	define A_MAGIC2	NMAGIC
#	define A_MAGIC3	ZMAGIC
#	undef ARMAG
#	define ARMAG	0177545
#endif

/*
 * Defaults for line printer capabilities data base
 */
#define	DEFLP		"default"
#define DEFLOCK		"lock"
#define DEFSTAT		"status"
#define DEFSPOOL        "/var/spool/printer"
#define	DEFDAEMON	"/usr/lib/lpd"
#define	DEFLOGF		"/dev/console"
#define	DEFDEVLP	"/dev/lp"
#define DEFRLPR		"/usr/lib/rlpr"
#define DEFBINDIR	"/usr/ucb"
#define	DEFMX		0
#define DEFMAXCOPIES	0
#define DEFFF		"\f"
#define DEFWIDTH	132
#define DEFLENGTH	66
#define DEFUID		1
#define DEFPC		0	/* Default page cost */
/*
 * When files are created in the spooling area, they are normally
 *   readable only by their owner and the spooling group.  If you
 *   want otherwise, change this mode.
 */
#define FILMOD		0660

/*
 * Printer is assumed to support LINELEN (for block chars)
 *   and background character (blank) is a space
 */
#define LINELEN		132
#define BACKGND		' '

#define HEIGHT	9		/* height of characters */
#define WIDTH	8		/* width of characters */
#define DROP	3		/* offset to drop characters with descenders */

/*
 * path name of files created by lpd.
 */
#define MASTERLOCK "/var/spool/lpd.lock"
#define SOCKETNAME "/dev/printer"

/*
 * Some utilities used by printjob.
 */
#define PR		"/bin/pr"
#define MAIL		"/usr/lib/sendmail"

/*
 * Define TERMCAP if the terminal capabilites are to be used for lpq.
 */
#define TERMCAP

/*
 * Maximum number of user and job requests for lpq and lprm.
 */
#define MAXUSERS	50
#define MAXREQUESTS	50
