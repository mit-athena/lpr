
/*
 * ========================================================================== 
 * Confidential and Proprietary.  Copyright 1989 by Apollo Computer Inc.,
 * Chelmsford, Massachusetts.  Unpublished -- All Rights Reserved Under
 * Copyright Laws Of The United States.
 *
 * Apollo Computer Inc. reserves all rights, title and interest with respect 
 * to copying, modification or the distribution of such software programs and
 * associated documentation, except those rights specifically granted by Apollo
 * in a Product Software Program License, Source Code License or Commercial
 * License Agreement (APOLLO NETWORK COMPUTING SYSTEM) between Apollo and 
 * Licensee.  Without such license agreements, such software programs may not
 * be used, copied, modified or distributed in source or object code form.
 * Further, the copyright notice must appear on the media, the supporting
 * documentation and packaging as set forth in such agreements.  Such License
 * Agreements do not grant any rights to use Apollo Computer's name or trademarks
 * in advertising or publicity, with respect to the distribution of the software
 * programs without the specific prior written permission of Apollo.  Trademark 
 * agreements may be obtained in a separate Trademark License Agreement.
 * ========================================================================== 
 */

#include "nametbl.h"

#ifndef __PROTOTYPE
#ifdef __STDC__
#define __PROTOTYPE(x) x
#else
#define __PROTOTYPE(x) ()
#endif
#endif

extern void error __PROTOTYPE((char *message));
extern void warning __PROTOTYPE((char *message));

extern void log_error __PROTOTYPE((int lineno, char *format_string, char *message));
extern void log_warning __PROTOTYPE((int lineno, char *format_string, char *message));
    
extern void inq_name_for_errors __PROTOTYPE((char *name));
extern void set_name_for_errors __PROTOTYPE((char *name));
    
extern boolean print_errors __PROTOTYPE((STRTAB_str_t source));
extern void yyerror __PROTOTYPE((char *message));