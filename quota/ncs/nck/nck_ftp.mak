# ========================================================================== 
# Confidential and Proprietary.  Copyright 1987 by Apollo Computer Inc.,
# Chelmsford, Massachusetts.  Unpublished -- All Rights Reserved Under
# Copyright Laws Of The United States.
# 
# Apollo Computer Inc. reserves all rights, title and interest with respect
# to copying, modification or the distribution of such software programs
# and associated documentation, except those rights specifically granted
# by Apollo in a Product Software Program License, Source Code License
# or Commercial License Agreement (APOLLO NETWORK COMPUTING SYSTEM) between
# Apollo and Licensee.  Without such license agreements, such software
# programs may not be used, copied, modified or distributed in source
# or object code form.  Further, the copyright notice must appear on the
# media, the supporting documentation and packaging as set forth in such
# agreements.  Such License Agreements do not grant any rights to use
# Apollo Computer's name or trademarks in advertising or publicity, with
# respect to the distribution of the software programs without the specific
# prior written permission of Apollo.  Trademark agreements may be obtained
# in a separate Trademark License Agreement.
# ========================================================================== 


#
# MS/DOS makefile for NCK, INET-family, FTP Software -specific library.  
# Depends on [make] entry in TOOLS.INI.
#

CFLAGS = $(BASECFLAGS) /DFTP /DINET

O = ftp

# *************************************************************************        

$(O)\ms_ftpa.obj: $*.asm
	$(BUILD_ASM)

$(O)\ms_ftp.obj: $*.c
	$(BUILD_C)

$(O)\socket.obj: $*.c 
	$(BUILD_C)

$(O)\socket_i.obj: $*.c 
	$(BUILD_C)

# *************************************************************************        

nck_ftp.lib: $(O)\ms_ftp.obj $(O)\ms_ftpa.obj $(O)\socket.obj $(O)\socket_i.obj 
	del nck_ftp.lib
	$(LIB) nck_ftp.lib +$(O)\ms_ftp+$(O)\ms_ftpa+$(O)\socket+$(O)\socket_i;
