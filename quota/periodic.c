
/* $Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/quota/periodic.c,v 1.1 1990-04-16 16:28:43 epeisach Exp $ */
/* $Source: /afs/dev.mit.edu/source/repository/athena/bin/lpr/quota/periodic.c,v $ */
/* $Author: epeisach $ */

#ifndef lint
static char periodic_rcs_id[] = "$Header: /afs/dev.mit.edu/source/repository/athena/bin/lpr/quota/periodic.c,v 1.1 1990-04-16 16:28:43 epeisach Exp $";
#endif lint

#include <stdio.h>
#include "quota.h"
#include "logger.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
void get_new_data();
/* This routine is the "Heart" of the communication between quota server and 
   journal server.
*/

static int cleartime;	/* Specify the tie to clear the acct file. */
static int ateof = 0;

static log_header jhead;

logger_periodic()
{
    static int inited = 0;
    struct stat sbuf;
    static char tmpname[MAXPATHLEN];

#if 0
    if(shutdown_requested) {
	/* Well then, shutdown... We don'tneed to save anything */
	syslog(LOG_INFO, "%s Logger shutdown\n", loggerSrvrName);
	graceful_exit();
    }
#endif

    if(inited == 0) {
	strcpy(tmpname, LOGTRANSFILE);
	strcat(tmpname, ".tmp");
	cleartime = time((long *) 0) - SAVETIME; /* Delete asap */
	if(stat(LOGTRANSFILE, &sbuf) < 0) {
	    cleartime += SAVETIME; /* No file, wait*/
	}
	if(logger_journal_get_header(&jhead)) {
	    syslog(LOG_INFO, "Unable to read header from journal db.");
	    exit(1);
	}
        inited = 1;
    }

    /* See if the file exists, if not, clear appropriate flags and return.*/
    if(stat(LOGTRANSFILE, &sbuf) < 0) {
	if(time((long *) 0) > cleartime) cleartime = time((long *) 0) + SAVETIME;
	ateof = 0;
#ifdef DEBUG
	if(logger_debug) printf("Returning noaction\n");
#endif
	return 0;
    }

    /* Ok, file exists. See if it has been modified since last readtime */
    /* We have this info cached, and on restart, get the info from the 
       header */
    if(sbuf.st_mtime > jhead.last_q_time) {
	if(jhead.quota_pos < sbuf.st_size)
	    (void) get_new_data(sbuf.st_mtime, LOGTRANSFILE);
    }

    if(jhead.quota_pos == sbuf.st_size) ateof = 1;

    /* If the time limit has expired, remove an empty log file */
    if(ateof && time((long *) 0) > cleartime) {
	/* Step 1 - move the database... */
	if(rename(LOGTRANSFILE, tmpname)) {
	    syslog(LOG_INFO, "Unable to rename old acctFile %s to %s deleting. Errno = %d", LOGTRANSFILE, tmpname, errno);
	    if(unlink(LOGTRANSFILE)) {
		syslog(LOG_INFO, "Could not delete old acctFile %s - FATAL", LOGTRANSFILE);
		exit(1);
	    }
	    goto removed;
	}

	/* Give time for any mods to be finished by the quota server... */
	sleep(2);

	/* Now flush any entries that might have been entered in move */
	if(stat(tmpname, &sbuf) < 0) {
	    syslog(LOG_INFO, "Unable to stat tmpfile %s", tmpname);
	    goto removed;
	}
	while(jhead.quota_pos < sbuf.st_size) {
	    (void) get_new_data(sbuf.st_mtime, tmpname);
	    if(stat(tmpname, &sbuf) < 0) {
		syslog(LOG_INFO, "Unable to stat tmpfile %s", tmpname);
		goto removed;
	    }
	}

	/* Ok the file is now empty of extra cruft that might have been
	   placed during the move. Remove the tmpfile */

    removed:	/* The file is gone - reset internal pointers... */
	(void) unlink(tmpname); 
	ateof = 0;
	/* Since we know jhead is up to date, reset vectors and store */
	jhead.quota_pos = 0;
	if(logger_journal_put_header(&jhead)) {
		syslog(LOG_INFO, "Periodic: Unable to write header - fatal");
		exit(1);
	    }
	/* Update cleartime */
	cleartime = time((long *) 0 ) + SAVETIME;
	/* Done with clear */
    }



#ifdef DEBUG
    if(ateof && logger_debug) {printf("EOF\n");
				 printf("Periodic %d\n", time(0));
			     }
#endif

    return 0; /* Must return 0 or dispatch blows up */
}

void get_new_data(t, name)
Time t;
char *name;
{
    FILE *acct;
    char buf[BUFSIZ];
    log_entity out;
    Time upt = 0;	/* Set up time change */
    long pos;
    
    if((acct=fopen(name, "r")) == NULL) {
	/* Must be a problem - just log and continue */	
	syslog(LOG_INFO, "Unable to open acct file %s.", LOGTRANSFILE);
	return;
    }

    if(fseek(acct, jhead.quota_pos, L_SET)) {
	syslog(LOG_INFO, "Unable to seek in acct file %s.", LOGTRANSFILE);
	fclose(acct);
	return;
    }

    /* Only update the file time if at eof... */
    if(fgets(buf, BUFSIZ, acct) == NULL) {
	syslog(LOG_INFO, "Unable to read acct file %s", LOGTRANSFILE);
	fclose(acct);
	return;
    }

    if(logger_parse_quota(buf, &out)) {
	syslog(LOG_INFO, "Unable to parse line %s", buf);
	fclose(acct);
	return;
    }

    ateof = 0;
    if(feof(acct)) {
	upt = t;
	ateof = 1;
    }
    if((pos = ftell(acct))< 0) {
	syslog(LOG_INFO, "Unable to find current pos in acct file %s", LOGTRANSFILE);
	fclose(acct);
	return;
    }

    if(logger_journal_add_entry(&out, upt, pos)) {
	syslog(LOG_INFO, "Error in updating journal file");
	fclose(acct);
	return;
    }
    
    /* If we succeeded, we need to get te header again */
    if(logger_journal_get_header(&jhead)) {
	syslog(LOG_INFO, "Unable to read header from journal db.");
	fclose(acct);
	exit(1);
    }

    (void) fclose(acct);

    return;
}