#include <stdio.h>
#include "logger.h"
#include <sys/types.h>
#ifdef _IBMR2
#include <sys/select.h>
#endif

#define ISSET(n) FD_ISSET(n, bitmap)
#define SET(n)	FD_SET(n, bitmap)
#define CLR(n)	FD_CLR(n, bitmap)
fd_set *bitmap; 
int bitmap_size;

#ifdef _AUX_SOURCE
typedef long	fd_mask;
#define NBBY	8
#define NFDBITS	(sizeof(fd_mask) * NBBY)
#ifndef howmany
#define howmany(x, y)	 (((x)+((y)-1))/(y))
#endif
#define FD_SET(n, p)   ((p)->fds_bits[0] |= (1 << (n)))
#define FD_ISSET(n, p)   ((p)->fds_bits[0] & (1 << (n)))
#endif

main(argc, argv) 
int argc;
char *argv[];
{
    int i;
    char *name;
    char *user;
    log_header head;
    int error=0;
    Pointer end;
    
    if (argc < 2) {
	name = "/usr/spool/quota/journal.db";
    }
    if (argc < 3) {
	user = "/usr/spool/quota/user.db";
    }
    else {
	if (argc > 3) {
	    fprintf(stderr, "Usage: consist [[journal_db] [user_db]\n");
	    exit(1);
	}
	if(argc > 1) name = argv[1];
	if(argc > 2) user = argv[2];
    }

    if(logger_journal_set_name(name)) {
	printf("Could not set name\n");
	exit(1);
    }
    if(logger_user_set_name(user)) {
	printf("Could not set name for udb\n");
	exit(1);
    }

    if(logger_journal_get_header(&head)) {
	printf("Could not get head\n");
	exit(1);
    }


#ifdef DEBUG
    printf("ver %d, num_ent %d, last_q_time %d, pos %d\n", 
	   head.version, head.num_ent, head.last_q_time, head.quota_pos);
#endif
    if(head.version != LOGGER_VERSION) {
	printf("Version mismatch on logs database: %d != %d\n", 
	       head.version, LOGGER_VERSION);
	exit(1);
    }


    /* Cheat and create the bitmap that we want */
    bitmap_size = sizeof(fd_set) + howmany(head.num_ent, NFDBITS) * sizeof(long);
    bitmap = (fd_set *) malloc(bitmap_size);
    bzero(bitmap, bitmap_size);

    while((scanf("%d\n", &i) == 1) && i != 0) {
	if(ISSET(i)) continue; /* Skip entries already found */
#ifdef DEBUG
	fprintf(stderr,"Checking %d\n", i);
#endif
	if(follow_chain(i)) {
	    fprintf(stderr, "Chain starting at %d failed\n", i);
	    exit(1);
	}
    }

#if 0
    for(i=1; i < (int)head.num_ent; i++) {
	if(ISSET(i)) continue;
	fprintf(stderr, "Entry %d not in chains\n");
	error = -1;
    }
#endif

    /* All good */
    exit(error);

}


void PROTECT(){}   
void UNPROTECT(){}   
	


int follow_chain(start)
Pointer start;
{

    log_entity *lent;
    Pointer n = start, prev=0;
    User_db db;
    User_str user;

    /* Start off the progression */
    if(!(lent = logger_journal_get_line(n))) {
	fprintf(stderr, "Could not read line %d\n", n);
	return -1;
    }

    if(lent->prev != 0) {
	fprintf(stderr,"Not starting at head of chain %d\n", start); 
    }

    prev = n;
    SET(n);
    while(lent->next != 0) {
	prev = n;
	n = lent->next;

	if(ISSET(n)) {
	    fprintf(stderr, "Already referenced - failure %d\n", n);
	    return -1;
	}

	if(!(lent = logger_journal_get_line(n))) {
	    fprintf(stderr, "Could not read line %d\n", n);
	    return -1;
	}
	if(lent->prev != prev) {
	    fprintf(stderr, "prev not match in line %d %d !=% d\n", n,
		    prev, lent->prev);
	    return -1;
	}
	
	SET(n);
    }

    /* See if is a deleted user */
    if((lent->user.name == 0) && (lent->user.instance == 0) && 
       (lent->user.realm == 0)) return 0;

    db.user.name = lent->user.name;
    db.user.instance = lent->user.instance;
    db.user.realm = lent->user.realm;
    user.name = lent->user.name;
    user.instance = lent->user.instance;
    user.realm = lent->user.realm;
    db.first = start;
    db.last = n;
    if(logger_set_user(&user, &db)) printf("Could not add user %d\n", db.user.name);
    return 0;
}

