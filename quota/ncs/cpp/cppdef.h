/*
 *		   S y s t e m   D e p e n d e n t
 *		D e f i n i t i o n s    f o r   C P P
 *
 * Definitions in this file may be edited to configure CPP for particular
 * host operating systems and target configurations.
 *
 * NOTE: cpp assumes it is compiled by a compiler that supports macros
 * with arguments.  If this is not the case (as for Decus C), #define
 * nomacarg -- and provide function equivalents for all macros.
 *
 * cpp also assumes the host and target implement the Ascii character set.
 * If this is not the case, you will have to do some editing here and there.
 */

/*
 * This redundant definition of TRUE and FALSE works around
 * a limitation of Decus C.
 */
#ifndef	TRUE
#define	TRUE			1
#define	FALSE			0
#endif

/*
 * Define the HOST operating system.  This is needed so that
 * cpp can use appropriate filename conventions.
 */
#define	SYS_UNKNOWN		0
#define	SYS_UNIX		1
#define	SYS_VMS			2
#define	SYS_RSX			3
#define	SYS_RT11		4
#define	SYS_LATTICE		5
#define	SYS_ONYX		6
#define	SYS_68000		7
#define SYS_MSDOS		8

#if defined(apple_aux) || defined(_AUX_SOURCE) || defined(_IBMR2)
/* since the current apple_aux cpp doesn't predefine anything! */
#define HOST                    SYS_UNIX
#endif

#ifndef	HOST
#ifdef	unix
#define	HOST			SYS_UNIX
#else
#ifdef	vms
#define	HOST			SYS_VMS
#else
#ifdef	rsx
#define	HOST			SYS_RSX
#else
#ifdef	rt11
#define	HOST			SYS_RT11
#else
#ifdef MSDOS
#define HOST			SYS_MSDOS
#endif
#endif
#endif
#endif
#endif
#endif

#ifndef	HOST
#define	HOST			SYS_UNKNOWN
#endif

/*
 * We assume that the target is the same as the host system
 */
#ifndef	TARGET
#define	TARGET			HOST
#endif

/*
 * In order to predefine machine-dependent constants,
 * several strings are defined here:
 *
 * MACHINE	defines the target cpu (by name)
 * SYSTEM	defines the target operating system
 * COMPILER	defines the target compiler
 *
 *	The above may be #defined as "" if they are not wanted.
 *	They should not be #defined as NULL.
 *
 * LINE_PREFIX	defines the # output line prefix, if not "line"
 *		This should be defined as "" if cpp is to replace
 *		the "standard" C pre-processor.
 *
 * FILE_LOCAL	marks functions which are referenced only in the
 *		file they reside.  Some C compilers allow these
 *		to be marked "static" even though they are referenced
 *		by "extern" statements elsewhere.
 *
 * OK_DOLLAR	Should be set TRUE if $ is a valid alphabetic character
 *		in identifiers (default), or zero if $ is invalid.
 *		Default is TRUE.
 *
 * OK_CONCAT	Should be set TRUE if # may be used to concatenate
 *		tokens in macros (per the Ansi Draft Standard) or
 *		FALSE for old-style # processing (needed if cpp is
 *		to process assembler source code).
 *
 * OK_DATE	Predefines the compilation date if set TRUE.
 *		Not permitted by the Nov. 12, 1984 Draft Standard.
 *
 * S_CHAR etc.	Define the sizeof the basic TARGET machine word types.
 *		By default, sizes are set to the values for the HOST
 *		computer.  If this is inappropriate, see the code in
 *		cpp3.c for details on what to change.  Also, if you
 *		have a machine where sizeof (signed int) differs from
 *		sizeof (unsigned int), you will have to edit code and
 *		tables in cpp3.c (and extend the -S option definition.)
 *
 * CPP_LIBRARY	May be defined if you have a site-specific include directory
 *		which is to be searched *before* the operating-system
 *		specific directories.
 */

#if TARGET == SYS_MSDOS
#define MACHINE			"i8086", "M_I86"
#define SYSTEM			"MSDOS"
#endif

#if TARGET == SYS_LATTICE
/*
 * We assume the operating system is pcdos for the IBM-PC.
 * We also assume the small model (just like the PDP-11)
 */
#define MACHINE			"i8086"
#define	SYSTEM			"pcdos"
#endif

#if TARGET == SYS_ONYX
#define	MACHINE			"z8000"
#define	SYSTEM			"unix"
#endif

#if TARGET == SYS_VMS
#define	MACHINE			"vax"
#define	SYSTEM			"vms"
#define	COMPILER		"vax11c"
#endif

#if TARGET == SYS_RSX
#define	MACHINE			"pdp11"
#define	SYSTEM			"rsx"
#define	COMPILER		"decus"
#endif

#if TARGET == SYS_RT11
#define	MACHINE			"pdp11"
#define	SYSTEM			"rt11"
#define	COMPILER		"decus"
#endif

#if TARGET == SYS_68000
/*
 * All three machine designators have been seen in various systems.
 * Warning -- compilers differ as to sizeof (int).  cpp3 assumes that
 * sizeof (int) == 2
 */
#define	MACHINE			"M68000", "m68000", "m68k"
#define	SYSTEM			"unix"
#endif

#if	TARGET == SYS_UNIX
#define	SYSTEM			"unix"
#ifdef	pdp11
#define	MACHINE			"pdp11"
#endif
#ifdef  ibm032
#define MACHINE			"ibm032"
#endif
#ifdef	vax
#define	MACHINE			"vax"
#endif
#if defined(ultrix) && defined(vax)
#define MACHINE			"ultrix", "vax"
#endif
#if defined(ultrix) && defined(mips)
/* Hack because of way we build clients */
#define MACHINE			"ultrix", "mips", "LANGUAGE_C", "MIPSEL"
#endif
#ifdef	apollo
#define	MACHINE			"m68000"
#endif
#ifdef	exl
#define	MACHINE			"i8086"
#endif
/* this is just for good luck - don't think any UNICOS's just defined "cray" */
#if defined(cray) && !defined(CRAY1) && !defined(CRAY2)
#define MACHINE                 "cray, CRAY, CRAY1"
#endif
/* think pre UNICOS 5.0 cray1's didn't have CRAY1; just CRAY */
#if defined(CRAY) && !defined(CRAY1) && !defined(CRAY2)
#define MACHINE                 "CRAY, CRAY1"
#endif
#ifdef CRAY1
#define MACHINE                 "CRAY, CRAY1"
#endif
#ifdef CRAY2
#define MACHINE                 "CRAY, CRAY2"
#endif
#ifdef SOLARIS
#define MACHINE		"SOLARIS","sparc","sun,"
#endif
#if defined(AIX) && defined(_I386)
#define MACHINE		"_AIX", "_I386", "NLS", "AIX", "i386"
#endif
#if defined(_IBMR2) && defined(_AIX)
#define MACHINE		"_AIX", "_IBMR2"
#endif
#if defined(apple_aux) || defined(_AUX_SOURCE)
#define	MACHINE			"M68000", "m68000", "m68k"
#undef  SYSTEM
#define	SYSTEM			"unix", "apple_aux", "_AUX_SOURCE", "_SYSV_SOURCE", "_BSD_SOURCE"
#endif
#endif

/*
 * defaults
 */

#ifndef MSG_PREFIX
#define MSG_PREFIX		"cpp: "
#endif

#ifndef LINE_PREFIX
#ifdef	decus
#define	LINE_PREFIX		""
#else
#define LINE_PREFIX		"line"
#endif
#endif

/*
 * OLD_PREPROCESSOR forces the definition of OK_DOLLAR, OK_CONCAT,
 * COMMENT_INVISIBLE, and STRING_FORMAL to values appropriate for
 * an old-style preprocessor.
 */
 
#ifndef	OLD_PREPROCESSOR
#define	OLD_PREPROCESSOR	FALSE
#endif

#if	OLD_PREPROCESSOR
#define	OK_DOLLAR		FALSE
#define	OK_CONCAT		FALSE
#define	COMMENT_INVISIBLE	TRUE
#define	STRING_FORMAL		TRUE
#define ALLOW_CONTROL_JUNK	TRUE
#endif

/*
 * RECURSION_LIMIT may be set to -1 to disable the macro recursion test.
 */
#ifndef	RECURSION_LIMIT
#define	RECURSION_LIMIT	1000
#endif

/*
 * BITS_CHAR may be defined to set the number of bits per character.
 * it is needed only for multi-byte character constants.
 */
#ifndef	BITS_CHAR
#define	BITS_CHAR		8
#endif

/*
 * BIG_ENDIAN is set TRUE on machines (such as the IBM 360 series)
 * where 'ab' stores 'a' in the high-bits and 'b' in the low-bits.
 * It is set FALSE on machines (such as the PDP-11 and Vax-11)
 * where 'ab' stores 'a' in the low-bits and 'b' in the high-bits.
 * (Or is it the other way around?) -- Warning: BIG_ENDIAN code is untested.
 */
#ifndef	BIG_ENDIAN
#define	BIG_ENDIAN 		FALSE
#endif

/*
 * COMMENT_INVISIBLE may be defined to allow "old-style" comment
 * processing, whereby the comment becomes a zero-length token
 * delimiter.  This permitted tokens to be concatenated in macro
 * expansions.  This was removed from the Draft Ansi Standard.
 */
#ifndef	COMMENT_INVISIBLE
#define	COMMENT_INVISIBLE	FALSE
#endif

/*
 * STRING_FORMAL may be defined to allow recognition of macro parameters
 * anywhere in replacement strings.  This was removed from the Draft Ansi
 * Standard and a limited recognition capability added.
 */
#ifndef	STRING_FORMAL
#define	STRING_FORMAL		FALSE
#endif

/*
 * OK_DOLLAR enables use of $ as a valid "letter" in identifiers.
 * This is a permitted extension to the Ansi Standard and is required
 * for e.g., VMS, RSX-11M, etc.   It should be set FALSE if cpp is
 * used to preprocess assembler source on Unix systems.  OLD_PREPROCESSOR
 * sets OK_DOLLAR FALSE for that reason.
 */
#ifndef	OK_DOLLAR
#define	OK_DOLLAR		TRUE
#endif

/*
 * OK_CONCAT enables (one possible implementation of) token concatenation.
 * If cpp is used to preprocess Unix assembler source, this should be
 * set FALSE as the concatenation character, #, is used by the assembler.
 */
#ifndef	OK_CONCAT
#define	OK_CONCAT		TRUE
#endif

/*
 * OK_DATE may be enabled to predefine today's date as a string
 * at the start of each compilation.  This is apparently not permitted
 * by the Draft Ansi Standard.
 */
#ifndef	OK_DATE
#define	OK_DATE		TRUE
#endif

#ifndef ALLOW_CONTROL_JUNK
#define ALLOW_CONTROL_JUNK TRUE
#endif

/*
 * Some common definitions.
 */

#ifndef	DEBUG
#define	DEBUG			FALSE
#endif

/*
 * The following definitions are used to allocate memory for
 * work buffers.  In general, they should not be modified
 * by implementors.
 *
 * PAR_MAC	The maximum number of #define parameters (31 per Standard)
 *		Note: we need another one for strings.
 * IDMAX	The longest identifier, 31 per Ansi Standard
 * NBUFF	Input buffer size
 * NWORK	Work buffer size -- the longest macro
 *		must fit here after expansion.
 * NEXP		The nesting depth of #if expressions
 * NINCLUDE	The number of directories that may be specified
 *		on a per-system basis, or by the -I option.
 * BLK_NEST	The number of nested #if's permitted.
 */

#define	IDMAX			 31
#define	PAR_MAC		   (31 + 1)
#define	NBUFF			512
#define	NWORK			1024
#define	NEXP			128
#define	NINCLUDE		  7
#define	NPARMWORK		(NWORK * 2)
#define	BLK_NEST		32

/*
 * Some special constants.  These may need to be changed if cpp
 * is ported to a wierd machine.
 *
 * NOTE: if cpp is run on a non-ascii machine, ALERT and VT may
 * need to be changed.  They are used to implement the proposed
 * ANSI standard C control characters '\a' and '\v' only.
 * DEL is used to tag macro tokens to prevent #define foo foo
 * from looping.  Note that we don't try to prevent more elaborate
 * #define loops from occurring.
 */

#ifndef	ALERT
#define	ALERT			'\007'		/* '\a' is "Bell"	*/
#endif

#ifndef	VT
#define	VT			'\013'		/* Vertical Tab CTRL/K	*/
#endif


#ifndef	FILE_LOCAL
#ifdef	decus
#define	FILE_LOCAL		static
#else
#ifdef	vax11c
#define	FILE_LOCAL		static
#else
#define	FILE_LOCAL				/* Others are global	*/
#endif
#endif
#endif

/*
 *  Define ELIDE_DOLLAR if $'s appearing in tokens are to
 *  be elided.
 */

#if defined(_AUX_SOURCE) || (defined(AIX) && defined(_I386))
#define ELIDE_DOLLAR 1
#endif
