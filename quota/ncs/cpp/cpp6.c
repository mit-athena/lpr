/*
 *			    C P P 6 . C
 *		S u p p o r t   R o u t i n e s
 *
 * Edit History
 * 12-Sep-87 KA		Don't complain about nested comments
 * 02-Sep-87 NWM	Make "elide $" mean "change $ to _"
 * 02-Jun-87 GLW	Added support to elide '$' in input tokens
 * 25-May-84 MM		Added 8-bit support to type table.
 * 30-May-84 ARF	sharp() should output filename in quotes
 * 02-Aug-84 MM		Newline and #line hacking.  sharp() now in cpp1.c
 * 31-Aug-84 MM		USENET net.sources release
 * 11-Sep-84 ado/MM	Keepcomments, also line number pathological
 * 12-Sep-84 ado/MM	bug if comment changes to space and we unget later.
 * 03-Oct-84 gkr/MM	Fixed scannumber bug for '.e' (as in struct.element).
 * 04-Oct-84 MM		Added ungetstring() for token concatenation
 * 08-Oct-84 MM		Yet another attack on number scanning
 * 31-Oct-84 ado	Parameterized $ in identifiers
 *  2-Nov-84 MM		Token concatenation is messier than I thought
 *  6-Dec-84 MM		\<nl> is everywhere invisible.
 */

#include	<stdio.h>
#include	<ctype.h>
#include	"cppdef.h"
#include	"cpp.h"

/*
 * skipnl()	skips over input text to the end of the line.
 * skipws()	skips over "whitespace" (spaces or tabs), but
 *		not skip over the end of the line.  It skips over
 *		TOK_SEP, however (though that shouldn't happen).
 * scanid()	reads the next token (C identifier) into token[].
 *		The caller has already read the first character of
 *		the identifier.  Unlike macroid(), the token is
 *		never expanded.
 * macroid()	reads the next token (C identifier) into token[].
 *		If it is a #defined macro, it is expanded, and
 *		macroid() returns TRUE, otherwise, FALSE.
 * catenate()	Does the dirty work of token concatenation, TRUE if it did.
 * scanstring()	Reads a string from the input stream, calling
 *		a user-supplied function for each character.
 *		This function may be output() to write the
 *		string to the output file, or save() to save
 *		the string in the work buffer.
 * scannumber()	Reads a C numeric constant from the input stream,
 *		calling the user-supplied function for each
 *		character.  (output() or save() as noted above.)
 * save()	Save one character in the work[] buffer.
 * savestring()	Saves a string in malloc() memory.
 * getfile()	Initialize a new FILEINFO structure, called when
 *		#include opens a new file, or a macro is to be
 *		expanded.
 * getmem()	Get a specified number of bytes from malloc memory.
 * output()	Write one character to stdout (calling putchar) --
 *		implemented as a function so its address may be
 *		passed to scanstring() and scannumber().
 * lookid()	Scans the next token (identifier) from the input
 *		stream.  Looks for it in the #defined symbol table.
 *		Returns a pointer to the definition, if found, or NULL
 *		if not present.  The identifier is stored in token[].
 * defnedel()	Define enter/delete subroutine.  Updates the
 *		symbol table.
 * get()	Read the next byte from the current input stream,
 *		handling end of (macro/file) input and embedded
 *		comments appropriately.  Note that the global
 *		instring is -- essentially -- a parameter to get().
 * cget()	Like get(), but skip over TOK_SEP.
 * unget()	Push last gotten character back on the input stream.
 * cerror(), cwarn(), cfatal(), cierror(), ciwarn()
 *		These routines format an print messages to the user.
 *		cerror & cwarn take a format and a single string argument.
 *		cierror & ciwarn take a format and a single int (char) argument.
 *		cfatal takes a format and a single string argument.
 */

/*
 * This table must be rewritten for a non-Ascii machine.
 *
 * Note that several "non-visible" characters have special meaning:
 * Hex 1D DEF_MAGIC -- a flag to prevent #define recursion.
 * Hex 1E TOK_SEP   -- a delimiter for token concatenation
 * Hex 1F COM_SEP   -- a zero-width whitespace for comment concatenation
 */
#if TOK_SEP != 0x1E || COM_SEP != 0x1F || DEF_MAGIC != 0x1D
	<< error type table isn't correct >>
#endif

#define DOLLAR '$'

#if OK_DOLLAR
#define	DOL	LET
#else
#define	DOL	000
#endif

char type[256] = {		/* Character type codes    Hex		*/
   END,   000,   000,   000,   000,   000,   000,   000, /* 00		*/
   000,   SPA,   000,   000,   000,   000,   000,   000, /* 08		*/
   000,   000,   000,   000,   000,   000,   000,   000, /* 10		*/
   000,   000,   000,   000,   000,   LET,   000,   SPA, /* 18		*/
   SPA,OP_NOT,   QUO,   000,   DOL,OP_MOD,OP_AND,   QUO, /* 20  !"#$%&'	*/
OP_LPA,OP_RPA,OP_MUL,OP_ADD,   000,OP_SUB,   DOT,OP_DIV, /* 28 ()*+,-./	*/
   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG,   DIG, /* 30 01234567	*/
   DIG,   DIG,OP_COL,   000, OP_LT, OP_EQ, OP_GT,OP_QUE, /* 38 89:;<=>?	*/
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 40 @ABCDEFG	*/
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 48 HIJKLMNO	*/
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 50 PQRSTUVW	*/
   LET,   LET,   LET,   000,   BSH,   000,OP_XOR,   LET, /* 58 XYZ[\]^_	*/
   000,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 60 `abcdefg	*/
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 68 hijklmno	*/
   LET,   LET,   LET,   LET,   LET,   LET,   LET,   LET, /* 70 pqrstuvw	*/
   LET,   LET,   LET,   000, OP_OR,   000,OP_NOT,   000, /* 78 xyz{|}~	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
   000,   000,   000,   000,   000,   000,   000,   000, /*   80 .. FF	*/
};

skipnl()
/*
 * Skip to the end of the current input line.
 */
{
	register int		c;

	do {				/* Skip to newline	*/
	    c = get();
	} while (c != '\n' && c != EOF_CHAR);
}

int
skipws()
/*
 * Skip over whitespace
 */
{
	register int		c;

	do {				/* Skip whitespace	*/
	    c = get();
#if COMMENT_INVISIBLE
	} while (type[c] == SPA || c == COM_SEP);
#else
	} while (type[c] == SPA);
#endif
	return (c);
}

scanid(c)
register int	c;				/* First char of id	*/
/*
 * Get the next token (an id) into the token buffer.
 * Note: this code is duplicated in lookid().
 * Change one, change both.
 */
{
	register char	*bp;

	if (c == DEF_MAGIC)			/* Eat the magic token	*/
	    c = get();				/* undefiner.		*/

#ifdef ELIDE_DOLLAR
	if (c == DOLLAR)			/* Convert $ to _ */
	    c = '_';
#endif

	bp = token;
	do {
	    if (bp < &token[IDMAX])		/* token dim is IDMAX+1	*/
		*bp++ = c;
	    c = get();
#ifdef ELIDE_DOLLAR
	if (c == DOLLAR)
	    c = '_';   
#endif
	} while (type[c] == LET || type[c] == DIG);
	unget();
	*bp = EOS;
}

int
macroid(c)
register int		c;
/*
 * If c is a letter, scan the id.  if it's #defined, expand it and scan
 * the next character and try again.
 *
 * Else, return the character.  If type[c] is a LET, the token is in token.
 */
{
	register DEFBUF	*dp;

	if (infile != NULL && infile->fp != NULL)
	    recursion = 0;
	while (type[c] == LET && (dp = lookid(c)) != NULL) {
	    expand(dp);
	    c = get();
	}
	return (c);
}

int
catenate()
/*
 * A token was just read (via macroid).
 * If the next character is TOK_SEP, concatenate the next token
 * return TRUE -- which should recall macroid after refreshing
 * macroid's argument.  If it is not TOK_SEP, unget() the character
 * and return FALSE.
 */
{
	register int		c;
	register char		*token1;

#if OK_CONCAT
	if (get() != TOK_SEP) {			/* Token concatenation	*/
	    unget();
	    return (FALSE);
	}
	else {
	    token1 = savestring(token);		/* Save first token	*/
	    c = macroid(get());			/* Scan next token	*/
	    switch(type[c]) {			/* What was it?		*/
	    case LET:				/* An identifier, ...	*/
		if (strlen(token1) + strlen(token) >= NWORK)
		    cfatal("work buffer overflow doing %s #", token1);
		sprintf(work, "%s%s", token1, token);
		break;

	    case DIG:				/* A digit string	*/
		strcpy(work, token1);
		workp = work + strlen(work);
		do {
		    save(c);
		} while ((c = get()) != TOK_SEP);
		/*
		 * The trailing TOK_SEP is no longer needed.
		 */
		save(EOS);
		break;

	    default:				/* An error, ...	*/
		if (isprint(c))
		    cierror("Strange character '%c' after #", c);
		else
		    cierror("Strange character (%d.) after #", c);
		strcpy(work, token1);
		unget();
		break;
	    }
	    /*
	     * work has the concatenated token and token1 has
	     * the first token (no longer needed).  Unget the
	     * new (concatenated) token after freeing token1.
	     * Finally, setup to read the new token.
	     */
	    free(token1);			/* Free up memory	*/
	    ungetstring(work);			/* Unget the new thing,	*/
	    return (TRUE);
	}
#else
	return (FALSE);				/* Not supported	*/
#endif
}

int
scanstring(delim, outfun)
register int	delim;			/* ' or "			*/
int		(*outfun)();		/* Output function		*/
/*
 * Scan off a string.  Warning if terminated by newline or EOF.
 * outfun() outputs the character -- to a buffer if in a macro.
 * TRUE if ok, FALSE if error.
 */
{
	register int		c;

	instring = TRUE;		/* Don't strip comments		*/
	(*outfun)(delim);
	while ((c = get()) != delim
	     && c != '\n'
	     && c != EOF_CHAR) {
	    (*outfun)(c);
	    if (c == '\\')
		(*outfun)(get());
	}
	instring = FALSE;
	if (c == delim) {
	    (*outfun)(c);
	    return (TRUE);
	}
	else {
	    cerror("Unterminated string", NULLST);
	    unget();
	    return (FALSE);
	}
}

scannumber(c, outfun)
register int	c;				/* First char of number	*/
register int	(*outfun)();			/* Output/store func	*/
/*
 * Process a number.  We know that c is from 0 to 9 or dot.
 * Algorithm from Dave Conroy's Decus C.
 */
{
	register int	radix;			/* 8, 10, or 16		*/
	int		expseen;		/* 'e' seen in floater	*/
	int		signseen;		/* '+' or '-' seen	*/
	int		octal89;		/* For bad octal test	*/
	int		dotflag;		/* TRUE if '.' was seen	*/

	expseen = FALSE;			/* No exponent seen yet	*/
	signseen = TRUE;			/* No +/- allowed yet	*/
	octal89 = FALSE;			/* No bad octal yet	*/
	radix = 10;				/* Assume decimal	*/
	if ((dotflag = (c == '.')) != FALSE) {	/* . something?		*/
	    (*outfun)('.');			/* Always out the dot	*/
	    if (type[(c = get())] != DIG) {	/* If not a float numb,	*/
		unget();			/* Rescan strange char	*/
		return;				/* All done for now	*/
	    }
	}					/* End of float test	*/
	else if (c == '0') {			/* Octal or hex?	*/
	    (*outfun)(c);			/* Stuff initial zero	*/
	    radix = 8;				/* Assume it's octal	*/
	    c = get();				/* Look for an 'x'	*/
	    if (c == 'x' || c == 'X') {		/* Did we get one?	*/
		radix = 16;			/* Remember new radix	*/
		(*outfun)(c);			/* Stuff the 'x'	*/
		c = get();			/* Get next character	*/
	    }
	}
	for (;;) {				/* Process curr. char.	*/
	    /*
	     * Note that this algorithm accepts "012e4" and "03.4"
	     * as legitimate floating-point numbers.
	     */
	    if (radix != 16 && (c == 'e' || c == 'E')) {
		if (expseen)			/* Already saw 'E'?	*/
		    break;			/* Exit loop, bad nbr.	*/
		expseen = TRUE;			/* Set exponent seen	*/
		signseen = FALSE;		/* We can read '+' now	*/
		radix = 10;			/* Decimal exponent	*/
	    }
	    else if (radix != 16 && c == '.') {
		if (dotflag)			/* Saw dot already?	*/
		    break;			/* Exit loop, two dots	*/
		dotflag = TRUE;			/* Remember the dot	*/
		radix = 10;			/* Decimal fraction	*/
	    }
	    else if (c == '+' || c == '-') {	/* 1.0e+10		*/
		if (signseen)			/* Sign in wrong place?	*/
		    break;			/* Exit loop, not nbr.	*/
		/* signseen = TRUE; */		/* Remember we saw it	*/
	    }
	    else {				/* Check the digit	*/
		switch (c) {
		case '8': case '9':		/* Sometimes wrong	*/
		    octal89 = TRUE;		/* Do check later	*/
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		    break;			/* Always ok		*/

		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		    if (radix == 16)		/* Alpha's are ok only 	*/
			break;			/* if reading hex.	*/
		default:			/* At number end	*/
		    goto done;			/* Break from for loop	*/
		}				/* End of switch	*/
	    }					/* End general case	*/
	    (*outfun)(c);			/* Accept the character	*/
	    signseen = TRUE;			/* Don't read sign now	*/
	    c = get();				/* Read another char	*/
	}					/* End of scan loop	*/
	/*
	 * When we break out of the scan loop, c contains the first
	 * character (maybe) not in the number.  If the number is an
	 * integer, allow a trailing 'L' for long and/or a trailing 'U'
	 * for unsigned.  If not those, push the trailing character back
	 * on the input stream.  Floating point numbers accept a trailing
	 * 'L' for "long double".
	 */
done:	if (dotflag || expseen) {		/* Floating point?	*/
	    if (c == 'l' || c == 'L') {
		(*outfun)(c);
		c = get();			/* Ungotten later	*/
	    }
	}
	else {					/* Else it's an integer	*/
	    /*
	     * We know that dotflag and expseen are both zero, now:
	     * dotflag signals "saw 'L'", and
	     * expseen signals "saw 'U'".
	     */
	    for (;;) {
		switch (c) {
		case 'l':
		case 'L':
		    if (dotflag)
			goto nomore;
		    dotflag = TRUE;
		    break;

		case 'u':
		case 'U':
		    if (expseen)
			goto nomore;
		    expseen = TRUE;
		    break;

		default:
		    goto nomore;
		}
		(*outfun)(c);			/* Got 'L' or 'U'.	*/
		c = get();			/* Look at next, too.	*/
	    }
	}
nomore:	unget();				/* Not part of a number	*/
	if (octal89 && radix == 8)
	    cwarn("Illegal digit in octal number", NULLST);
}

save(c)
register int	c;
{
	if (workp >= &work[NWORK])
	    cfatal("Work buffer overflow", NULLST);
	else *workp++ = c;
}

char *
savestring(text)
char		*text;
/*
 * Store a string into free memory.
 */
{
	register char	*result;

	result = getmem(strlen(text) + 1);
	strcpy(result, text);
	return (result);
}

FILEINFO	*
getfile(bufsize, name)
int		bufsize;		/* Line or define buffer size	*/
char		*name;			/* File or macro name string	*/
/*
 * Common FILEINFO buffer initialization for a new file or macro.
 */
{
	register FILEINFO	*file;
	register int		size;

	size = strlen(name);			/* File/macro name	*/
	file = (FILEINFO *) getmem(sizeof (FILEINFO) + bufsize + size);
	file->parent = infile;			/* Chain files together	*/
	file->fp = NULL;			/* No file yet		*/
	file->filename = savestring(name);	/* Save file/macro name	*/
	file->progname = NULL;			/* No #line seen yet	*/
	file->unrecur = 0;			/* No macro fixup	*/
	file->bptr = file->buffer;		/* Initialize line ptr	*/
	file->buffer[0] = EOS;			/* Force first read	*/
	file->line = 0;				/* (Not used just yet)	*/
	if (infile != NULL)			/* If #include file	*/
	    infile->line = line;		/* Save current line	*/
	infile = file;				/* New current file	*/
	line = 1;				/* Note first line	*/
	return (file);				/* All done.		*/
}

char *
getmem(size)
int		size;
/*
 * Get a block of free memory.
 */
{
	register char	*result;
	extern char	*malloc();

	if ((result = malloc((unsigned) size)) == NULL)
	    cfatal("Out of memory", NULLST);
	return (result);
}

/*
 *			C P P   S y m b o l   T a b l e s
 */

/*
 * SBSIZE defines the number of hash-table slots for the symbol table.
 * It must be a power of 2.
 */
#ifndef	SBSIZE
#define	SBSIZE	64
#endif
#define	SBMASK	(SBSIZE - 1)
#if (SBSIZE ^ SBMASK) != ((SBSIZE * 2) - 1)
	<< error, SBSIZE must be a power of 2 >>
#endif

static DEFBUF	*symtab[SBSIZE];	/* Symbol table queue headers	*/

DEFBUF *
lookid(c)
int	c;				/* First character of token	*/
/*
 * Look for the next token in the symbol table.  Returns token in "token".
 * If found, returns the table pointer;  Else returns NULL.
 */
{
	register int		nhash;
	register DEFBUF		*dp;
	register char		*np;
	int			temp;
	int			isrecurse;	/* For #define foo foo	*/

	np = token;
	nhash = 0;
	if ((isrecurse = (c == DEF_MAGIC)))	/* If recursive macro	*/
	    c = get();				/* hack, skip DEF_MAGIC	*/
#ifdef ELIDE_DOLLAR
	if (c == DOLLAR)
	    c = '_';
#endif
	do {
	    if (np < &token[IDMAX]) {		/* token dim is IDMAX+1	*/
		*np++ = c;			/* Store token byte	*/
		nhash += c;			/* Update hash value	*/
	    }
	    c = get();				/* And get another byte	*/
#ifdef ELIDE_DOLLAR
	if (c == DOLLAR)
	    c = '_';
#endif
	} while (type[c] == LET || type[c] == DIG);
	unget();				/* Rescan terminator	*/
	*np = EOS;				/* Terminate token	*/
	if (isrecurse)				/* Recursive definition	*/
	    return (NULL);			/* undefined just now	*/
	nhash += (np - token);			/* Fix hash value	*/
	dp = symtab[nhash & SBMASK];		/* Starting bucket	*/
	while (dp != (DEFBUF *) NULL) {		/* Search symbol table	*/
	    if (dp->hash == nhash		/* Fast precheck	*/
	     && (temp = strcmp(dp->name, token)) >= 0)
		break;
	    dp = dp->link;			/* Nope, try next one	*/
	}
	return ((temp == 0) ? dp : NULL);
}

DEFBUF *
defendel(name, delete)
char		*name;
int		delete;			/* TRUE to delete a symbol	*/
/*
 * Enter this name in the lookup table (delete = FALSE)
 * or delete this name (delete = TRUE).
 * Returns a pointer to the define block (delete = FALSE)
 * Returns NULL if the symbol wasn't defined (delete = TRUE).
 */
{
	register DEFBUF		*dp;
	register DEFBUF		**prevp;
	register char		*np;
	int			nhash;
	int			temp;
	int			size;

	for (nhash = 0, np = name; *np != EOS;)
	    nhash += *np++;
	size = (np - name);
	nhash += size;
	prevp = &symtab[nhash & SBMASK];
	while ((dp = *prevp) != (DEFBUF *) NULL) {
	    if (dp->hash == nhash
	     && (temp = strcmp(dp->name, name)) >= 0) {
		if (temp > 0)
		    dp = NULL;			/* Not found		*/
		else {
		    *prevp = dp->link;		/* Found, unlink and	*/
		    if (dp->repl != NULL)	/* Free the replacement	*/
			free(dp->repl);		/* if any, and then	*/
		    free((char *) dp);		/* Free the symbol	*/
		}
		break;
	    }
	    prevp = &dp->link;
	}
	if (!delete) {
	    dp = (DEFBUF *) getmem(sizeof (DEFBUF) + size);
	    dp->link = *prevp;
	    *prevp = dp;
	    dp->hash = nhash;
	    dp->repl = NULL;
	    dp->nargs = 0;
	    strcpy(dp->name, name);
	}
	return (dp);
}

#if DEBUG

dumpdef(why)
char		*why;
{
	register DEFBUF		*dp;
	register DEFBUF		**syp;

	printf("CPP symbol table dump %s\n", why);
	for (syp = symtab; syp < &symtab[SBSIZE]; syp++) {
	    if ((dp = *syp) != (DEFBUF *) NULL) {
		printf("symtab[%d]\n", (syp - symtab));
		do {
		    dumpadef((char *) NULL, dp);
		} while ((dp = dp->link) != (DEFBUF *) NULL);
	    }
	}
}

dumpadef(why, dp)
char		*why;			/* Notation			*/
register DEFBUF	*dp;
{
	register char		*cp;
	register int		c;

	printf(" \"%s\" [%d]", dp->name, dp->nargs);
	if (why != NULL)
	    printf(" (%s)", why);
	if (dp->repl != NULL) {
	    printf(" => ");
	    for (cp = dp->repl; (c = *cp++ & 0xFF) != EOS;) {
		if (c >= MAC_PARM && c <= (MAC_PARM + PAR_MAC))
		    printf("<%d>", c - MAC_PARM);
		else if (isprint(c) || c == '\n' || c == '\t')
		    putchar(c);
		else if (c < ' ')
		    printf("<^%c>", c + '@');
		else
		    printf("<\\0%o>", c);
	    }
	}
	else {
	    printf(", no replacement.");
	}
	putchar('\n');
}
#endif

/*
 *			G E T
 */

int
get()
/*
 * Return the next character from a macro or the current file.
 * Handle end of file from #include files.
 */
{
	register int		c;
	register FILEINFO	*file;
	register int		popped;		/* Recursion fixup	*/

	popped = 0;
get_from_file:
	if ((file = infile) == NULL)
	    return (EOF_CHAR);
newline:
#if 0
	printf("get(%s), recursion %d, line %d, bptr = %d, buffer \"%s\"\n",
	    file->filename, recursion, line,
	    file->bptr - file->buffer, file->buffer);
#endif
	/*
	 * Read a character from the current input line or macro.
	 * At EOS, either finish the current macro (freeing temp.
	 * storage) or read another line from the current input file.
	 * At EOF, exit the current file (#include) or, at EOF from
	 * the cpp input file, return EOF_CHAR to finish processing.
	 */
	if ((c = *file->bptr++ & 0xFF) == EOS) {
	    /*
	     * Nothing in current line or macro.  Get next line (if
	     * input from a file), or do end of file/macro processing.
	     * In the latter case, jump back to restart from the top.
	     */
	    if (file->fp == NULL) {		/* NULL if macro	*/
		popped++;
		recursion -= file->unrecur;
		if (recursion < 0)
		    recursion = 0;
		infile = file->parent;		/* Unwind file chain	*/
	    }
	    else {				/* Else get from a file	*/
		if ((file->bptr = fgets(file->buffer, NBUFF, file->fp))
			!= NULL) {
#if DEBUG
		    if (debug > 1) {		/* Dump it to stdout	*/
			printf("\n#line %d (%s), %s",
			    line, file->filename, file->buffer);
		    }
#endif
		    goto newline;		/* process the line	*/
		}
		else {
		    fclose(file->fp);		/* Close finished file	*/
		    if ((infile = file->parent) != NULL) {
			/*
			 * There is an "ungotten" newline in the current
			 * infile buffer (set there by doinclude() in
			 * cpp1.c).  Thus, we know that the mainline code
			 * is skipping over blank lines and will do a
			 * #line at its convenience.
			 */
			wrongline = TRUE;	/* Need a #line now	*/
		    }
		}
	    }
	    /*
	     * Free up space used by the (finished) file or macro and
	     * restart input from the parent file/macro, if any.
	     */
	    free(file->filename);		/* Free name and	*/
	    if (file->progname != NULL)		/* if a #line was seen,	*/
		free(file->progname);		/* free it, too.	*/
	    free((char *) file);		/* Free file space	*/
	    if (infile == NULL)			/* If at end of file	*/
		return (EOF_CHAR);		/* Return end of file	*/
	    line = infile->line; 		/* Reset line number	*/
	    goto get_from_file;			/* Get from the top.	*/
	}
	/*
	 * Common processing for the new character.
	 */
	if (c == DEF_MAGIC && file->fp != NULL)	/* Don't allow delete	*/
	    goto newline;			/* from a file		*/
	if (file->parent != NULL) {		/* Macro or #include	*/
	    if (popped != 0)
		file->parent->unrecur += popped;
	    else {
		recursion -= file->parent->unrecur;
		if (recursion < 0)
		    recursion = 0;
		file->parent->unrecur = 0;
	    }
	}
	if (c == '\n')				/* Maintain current	*/
	    ++line;				/* line counter		*/
	if (instring)				/* Strings just return	*/
	    return (c);				/* the character.	*/
	else if (c == '/') {			/* Comment?		*/
	    instring = TRUE;			/* So get() won't loop	*/
	    if ((c = get()) != '*') {		/* Next byte '*'?	*/
		instring = FALSE;		/* Nope, no comment	*/
		unget();			/* Push the char. back	*/
		return ('/');			/* Return the slash	*/
	    }
	    if (keepcomments) {			/* If writing comments	*/
		putchar('/');			/* Write out the	*/
		putchar('*');			/*   initializer	*/
	    }
	    for (;;) {				/* Eat a comment	*/
		c = get();
test:		if (keepcomments && c != EOF_CHAR)
		    cput(c);
		switch (c) {
		case EOF_CHAR:
		    cerror("EOF in comment", NULLST);
		    return (EOF_CHAR);

		case '/':
		    if ((c = get()) != '*')	/* Don't let comments	*/
			goto test;		/* Nest.		*/
#ifdef NOTDEF
		    cwarn("Nested comments", NULLST);
#endif
						/* Fall into * stuff	*/
		case '*':
		    if ((c = get()) != '/')	/* If comment doesn't	*/
			goto test;		/* end, look at next	*/
		    instring = FALSE;		/* End of comment,	*/
		    if (keepcomments) {		/* Put out the comment	*/
			cput(c);		/* terminator, too	*/
		    }
		    /*
		     * A comment is syntactically "whitespace" --
		     * however, there are certain strange sequences
		     * such as
		     *		#define foo(x)	(something)
		     *			foo|* comment *|(123)
		     *       these are '/' ^           ^
		     * where just returning space (or COM_SEP) will cause
		     * problems.  This can be "fixed" by overwriting the
		     * '/' in the input line buffer with ' ' (or COM_SEP)
		     * but that may mess up an error message.
		     * So, we peek ahead -- if the next character is
		     * "whitespace" we just get another character, if not,
		     * we modify the buffer.  All in the name of purity.
		     */
		    if (*file->bptr == '\n'
		     || type[*file->bptr & 0xFF] == SPA)
			goto newline;
#if COMMENT_INVISIBLE
		    /*
		     * Return magic (old-fashioned) syntactic space.
		     */
		    return ((file->bptr[-1] = COM_SEP));
#else
		    return ((file->bptr[-1] = ' '));
#endif

		case '\n':			/* we'll need a #line	*/
		    if (!keepcomments)
			wrongline = TRUE;	/* later...		*/
		default:			/* Anything else is	*/
		    break;			/* Just a character	*/
		}				/* End switch		*/
	    }					/* End comment loop	*/
	}					/* End if in comment	*/
	else if (!inmacro && c == '\\') {	/* If backslash, peek 	*/
	    if ((c = get()) == '\n') {		/* for a <nl>.  If so,	*/
		wrongline = TRUE;
		goto newline;
	    }
	    else {				/* Backslash anything	*/
		unget();			/* Get it later		*/
		return ('\\');			/* Return the backslash	*/
	    }
	}
	else if (c == '\f' || c == VT)		/* Form Feed, Vertical	*/
	    c = ' ';				/* Tab are whitespace	*/
	return (c);				/* Just return the char	*/
}

unget()
/*
 * Backup the pointer to reread the last character.  Fatal error
 * (code bug) if we backup too far.  unget() may be called,
 * without problems, at end of file.  Only one character may
 * be ungotten.  If you need to unget more, call ungetstring().
 */
{
	register FILEINFO	*file;

	if ((file = infile) == NULL)
	    return;			/* Unget after EOF		*/
	if (--file->bptr < file->buffer)
	    cfatal("Too much pushback", NULLST);
	if (*file->bptr == '\n')	/* Ungetting a newline?		*/
	    --line;			/* Unget the line number, too	*/
}

ungetstring(text)
char		*text;
/*
 * Push a string back on the input stream.  This is done by treating
 * the text as if it were a macro.
 */
{
	register FILEINFO	*file;
	extern FILEINFO		*getfile();

	file = getfile(strlen(text) + 1, "");
	strcpy(file->buffer, text);
}

int
cget()
/*
 * Get one character, absorb "funny space" after comments or
 * token concatenation
 */
{
	register int	c;

	do {
	    c = get();
#if COMMENT_INVISIBLE
	} while (c == TOK_SEP || c == COM_SEP);
#else
	} while (c == TOK_SEP);
#endif
	return (c);
}

/*
 * Error messages and other hacks.  The first byte of severity
 * is 'S' for string arguments and 'I' for int arguments.  This
 * is needed for portability with machines that have int's that
 * are shorter than  char *'s.
 */

static
domsg(severity, format, arg)
char		*severity;		/* "Error", "Warning", "Fatal"	*/
char		*format;		/* Format for the error message	*/
char		*arg;			/* Something for the message	*/
/*
 * Print filenames, macro names, and line numbers for error messages.
 */
{
	register char		*tp;
	register FILEINFO	*file;

	fprintf(stderr, "%sline %d, %s: ", MSG_PREFIX, line, &severity[1]);
	if (*severity == 'S')
	    fprintf(stderr, format, arg);
	else
	    fprintf(stderr, format, (int) arg);
	putc('\n', stderr);
	if ((file = infile) == NULL)
	    return;				/* At end of file	*/
	if (file->fp != NULL) {
	    tp = file->buffer;			/* Print current file	*/
	    fprintf(stderr, "%s", tp);		/* name, making sure	*/
	    if (tp[strlen(tp) - 1] != '\n')	/* there's a newline	*/
		putc('\n', stderr);
	}
#if 0
	while ((file = file->parent) != NULL) {	/* Print #includes, too	*/
	    if (file->fp == NULL)
		fprintf(stderr, "from macro %s\n", file->filename);
	    else {
		tp = file->buffer;
		fprintf(stderr, "from file %s, line %d:\n%s",
		    (file->progname != NULL)
			? file->progname : file->filename,
		    file->line, tp);
		if (tp[strlen(tp) - 1] != '\n')
		    putc('\n', stderr);
	    }
	}
#endif
}

cerror(format, sarg)
char		*format;
char		*sarg;		/* Single string argument		*/
/*
 * Print a normal error message, string argument.
 */
{
	domsg("SError", format, sarg);
	errors++;
}

cierror(format, narg)
char		*format;
int		narg;		/* Single numeric argument		*/
/*
 * Print a normal error message, numeric argument.
 */
{
	domsg("IError", format, (char *) narg);
	errors++;
}

cfatal(format, sarg)
char		*format;
char		*sarg;			/* Single string argument	*/
/*
 * A real disaster
 */
{
	domsg("SFatal error", format, sarg);
	exit(IO_ERROR);
}

cwarn(format, sarg)
char		*format;
char		*sarg;			/* Single string argument	*/
/*
 * A non-fatal error, string argument.
 */
{
	domsg("SWarning", format, sarg);
}

ciwarn(format, narg)
char		*format;
int		narg;			/* Single numeric argument	*/
/*
 * A non-fatal error, numeric argument.
 */
{
	domsg("IWarning", format, (char *) narg);
}



