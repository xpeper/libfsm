/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 22 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include "libre/re_ast.h"
	#include "libre/re_char_class.h"

	#include <re/re.h>

	#ifndef DIALECT
	#error DIALECT required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, DIALECT)

	#define LX_TOKEN   CAT(LX_PREFIX, _token)
	#define LX_STATE   CAT(LX_PREFIX, _lx)
	#define LX_NEXT    CAT(LX_PREFIX, _next)
	#define LX_INIT    CAT(LX_PREFIX, _init)

	#define DIALECT_PARSE  CAT(parse_re_, DIALECT)
	#define DIALECT_CHAR_CLASS  CAT(re_char_class_, DIALECT)
	#define DIALECT_CHAR_TYPE   CAT(re_char_type_, DIALECT)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define DIALECT_ENTRY CAT(p_re__, DIALECT)

	#define TOK_CLASS__alnum  TOK_CLASS_ALNUM
	#define TOK_CLASS__alpha  TOK_CLASS_ALPHA
	#define TOK_CLASS__any    TOK_CLASS_ANY
	#define TOK_CLASS__ascii  TOK_CLASS_ASCII
	#define TOK_CLASS__blank  TOK_CLASS_BLANK
	#define TOK_CLASS__cntrl  TOK_CLASS_CNTRL
	#define TOK_CLASS__digit  TOK_CLASS_DIGIT
	#define TOK_CLASS__graph  TOK_CLASS_GRAPH
	#define TOK_CLASS__lower  TOK_CLASS_LOWER
	#define TOK_CLASS__print  TOK_CLASS_PRINT
	#define TOK_CLASS__punct  TOK_CLASS_PUNCT
	#define TOK_CLASS__space  TOK_CLASS_SPACE
	#define TOK_CLASS__spchr  TOK_CLASS_SPCHR
	#define TOK_CLASS__upper  TOK_CLASS_UPPER
	#define TOK_CLASS__word   TOK_CLASS_WORD
	#define TOK_CLASS__xdigit TOK_CLASS_XDIGIT

	#define TOK_CLASS__nspace  TOK_CLASS_NSPACE
	#define TOK_CLASS__ndigit  TOK_CLASS_NDIGIT

	/* This is a hack to work around the AST files not being able to include lexer.h. */
	#define AST_POS_OF_LX_POS(AST_POS, LX_POS)			\
		do {							\
			AST_POS.line = LX_POS.line;			\
			AST_POS.col = LX_POS.col;			\
			AST_POS.byte = LX_POS.byte;			\
		} while (0)


	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"
	#include "../../re_char_class.h"

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef char_class_constructor_fun * t_ast__char__class__id;
	typedef struct ast_count t_ast__count;

	typedef struct re_char_class_ast * t_char__class__ast;
	typedef struct ast_char_class * t_char__class;
	typedef enum re_char_class_flags t_char__class__flags;

	typedef struct ast_range_endpoint t_range__endpoint;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		int overlap; /* permit overlap in groups */

		/*
		 * Lexical position stored for syntax errors.
		 */
		struct re_pos synstart;
		struct re_pos synend;

		/*
		 * Lexical positions stored for errors which describe multiple tokens.
		 * We're able to store these without needing a stack, because these are
		 * non-recursive productions.
		 */
		struct re_pos groupstart; struct re_pos groupend;
		struct re_pos rangestart; struct re_pos rangeend;
		struct re_pos countstart; struct re_pos countend;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		re_getchar_fun *f;
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { mark(&act_state->synstart, &lex_state->lx.start); \
	                              mark(&act_state->synend,   &lex_state->lx.end);   \
	                              act_state->lex_tok = LX_NEXT(&lex_state->lx); \
		} while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	mark(struct re_pos *r, const struct lx_pos *pos)
	{
		assert(r != NULL);
		assert(pos != NULL);

		r->byte = pos->byte;
	}

	/* TODO: centralise perhaps */
	static void
	snprintdots(char *s, size_t sz, const char *msg)
	{
		size_t n;

		assert(s != NULL);
		assert(sz > 3);
		assert(msg != NULL);

		n = sprintf(s, "%.*s", (int) sz - 3 - 1, msg);
		if (n == sz - 3 - 1) {
			strcpy(s + sz, "...");
		}
	}

	/* TODO: centralise */
	/* XXX: escaping really depends on dialect */
	static const char *
	escchar(char *s, size_t sz, int c)
	{
		size_t i;

		const struct {
			int c;
			const char *s;
		} a[] = {
			{ '\\', "\\\\" },

			{ '^',  "\\^"  },
			{ '-',  "\\-"  },
			{ ']',  "\\]"  },
			{ '[',  "\\["  },

			{ '\f', "\\f"  },
			{ '\n', "\\n"  },
			{ '\r', "\\r"  },
			{ '\t', "\\t"  },
			{ '\v', "\\v"  }
		};

		assert(s != NULL);
		assert(sz >= 5);

		(void) sz;

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].c == c) {
				return a[i].s;
			}
		}

		if (!isprint((unsigned char) c)) {
			sprintf(s, "\\x%02X", (unsigned char) c);
			return s;
		}

		sprintf(s, "%c", c);
		return s;
	}

#line 219 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Cflags_C_Cflag__set(flags, lex_state, act_state, err, t_re__flags, t_re__flags *);
static void p_138(flags, lex_state, act_state, err);
static void p_142(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *, t_pos *);
static void p_expr_C_Cchar_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *, t_pos *);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_191(flags, lex_state, act_state, err);
static void p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cflags(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Cchar_Htype(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Clist_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr *);
static void p_223(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_226(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
extern void p_re__pcre(flags, lex_state, act_state, err, t_ast__expr *);
static void p_230(flags, lex_state, act_state, err, t_ast__char__class__id *, t_pos *, t_char__class__ast *);
static void p_expr_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_246(flags, lex_state, act_state, err, t_char *, t_pos *, t_char__class__ast *);
static void p_249(flags, lex_state, act_state, err, t_ast__expr *, t_pos *, t_unsigned *, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_expr_C_Cflags_C_Cflag__set(flags flags, lex_state lex_state, act_state act_state, err err, t_re__flags ZIi, t_re__flags *ZOo)
{
	t_re__flags ZIo;

	switch (CURRENT_TERMINAL) {
	case (TOK_FLAG__INSENSITIVE):
		{
			t_re__flags ZIc;

			/* BEGINNING OF EXTRACT: FLAG_INSENSITIVE */
			{
#line 476 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 274 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: FLAG_INSENSITIVE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: re-flag-union */
			{
#line 739 "src/libre/parser.act"

		(ZIo) = (ZIi) | (ZIc);
	
#line 284 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: re-flag-union */
		}
		break;
	case (TOK_FLAG__UNKNOWN):
		{
			ADVANCE_LEXER;
			ZIo = ZIi;
			/* BEGINNING OF ACTION: err-unknown-flag */
			{
#line 538 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 302 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unknown-flag */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOo = ZIo;
}

static void
p_138(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI139;
		t_pos ZI140;
		t_pos ZI141;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZI139 = '-';
		ZI140 = lex_state->lx.start;
		ZI141   = lex_state->lx.end;
	
#line 341 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-range */
		{
#line 517 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 362 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-range */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_142(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOupper, t_pos *ZOu__end)
{
	t_range__endpoint ZIupper;
	t_pos ZIu__end;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZIu__start;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZIc = '-';
		ZIu__start = lex_state->lx.start;
		ZIu__end   = lex_state->lx.end;
	
#line 393 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 721 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (ZIc);
		(ZIupper) = range;
	
#line 406 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
		}
		break;
	case (TOK_NAMED__CHAR__CLASS):
		{
			t_pos ZIu__start;

			p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass (flags, lex_state, act_state, err, &ZIupper, &ZIu__start, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			t_pos ZIu__start;

			p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral (flags, lex_state, act_state, err, &ZIupper, &ZIu__start, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOupper = ZIupper;
	*ZOu__end = ZIu__end;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hterm(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI239;
			t_pos ZI240;
			t_pos ZI241;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI240 = lex_state->lx.start;
		ZI241   = lex_state->lx.end;

		ZI239 = lex_state->buf.a[0];
	
#line 471 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_246 (flags, lex_state, act_state, err, &ZI239, &ZI240, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI243;
			t_pos ZI244;
			t_pos ZI245;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 305 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI243 = lex_state->buf.a[2];
		if ((unsigned char) ZI243 > 127) {
			goto ZL1;
		}
		ZI243 = (((toupper((unsigned char)ZI243)) - 64) % 128 + 128) % 128;

		ZI244 = lex_state->lx.start;
		ZI245   = lex_state->lx.end;
	
#line 506 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 559 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 519 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			p_246 (flags, lex_state, act_state, err, &ZI243, &ZI244, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZIc;
			t_pos ZI110;
			t_pos ZI111;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 273 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZI110 = lex_state->lx.start;
		ZI111   = lex_state->lx.end;
	
#line 558 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: char-class-ast-literal */
			{
#line 649 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_literal((ZIc));
	
#line 568 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-literal */
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI235;
			t_pos ZI236;
			t_pos ZI237;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 361 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI236 = lex_state->lx.start;
		ZI237   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZI235 = (char) (unsigned char) u;
	
#line 621 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_246 (flags, lex_state, act_state, err, &ZI235, &ZI236, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI231;
			t_pos ZI232;
			t_pos ZI233;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 321 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI232 = lex_state->lx.start;
		ZI233   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZI231 = (char) (unsigned char) u;
	
#line 680 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_246 (flags, lex_state, act_state, err, &ZI231, &ZI232, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CHAR__CLASS):
		{
			t_ast__char__class__id ZI227;
			t_pos ZI228;
			t_pos ZI229;

			/* BEGINNING OF INLINE: expr::char-class::class-named */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_NAMED__CHAR__CLASS):
						/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
						{
#line 455 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZI227);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZI228 = lex_state->lx.start;
		ZI229   = lex_state->lx.end;
	
#line 725 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: NAMED_CHAR_CLASS */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
			}
			/* END OF INLINE: expr::char-class::class-named */
			p_230 (flags, lex_state, act_state, err, &ZI227, &ZI228, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-term */
		{
#line 489 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL3;
	
#line 760 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-term */
		/* BEGINNING OF ACTION: char-class-ast-flag-none */
		{
#line 705 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
	
#line 769 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-flag-none */
	}
	goto ZL0;
ZL3:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_range__endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__char__class__id ZIid;

		/* BEGINNING OF INLINE: expr::char-class::class-named */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_NAMED__CHAR__CLASS):
					/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
					{
#line 455 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZIid);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 822 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: NAMED_CHAR_CLASS */
					break;
				default:
					goto ZL1;
				}
				ADVANCE_LEXER;
			}
		}
		/* END OF INLINE: expr::char-class::class-named */
		/* BEGINNING OF ACTION: ast-range-endpoint-char-class */
		{
#line 728 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_CHAR_CLASS;
		range.u.char_class.ctor = (ZIid);
		(ZIr) = range;
	
#line 842 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-range-endpoint-char-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOstart = ZIstart;
	*ZOend = ZIend;
}

static void
p_expr_C_Cchar_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_pos ZI159;
		t_char__class__ast ZIhead;
		t_char__class__ast ZIbody;
		t_pos ZIend;
		t_char__class__ast ZIhb;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 252 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI159   = lex_state->lx.end;
	
#line 881 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead (flags, lex_state, act_state, err, &ZIhead);
		p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZIbody);
		/* BEGINNING OF INLINE: 162 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				t_char ZI163;
				t_pos ZI164;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 257 "src/libre/parser.act"

		ZI163 = ']';
		ZI164 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 911 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 566 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 926 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 524 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 942 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				ZIend = ZIstart;
			}
		ZL2:;
		}
		/* END OF INLINE: 162 */
		/* BEGINNING OF ACTION: char-class-ast-concat */
		{
#line 683 "src/libre/parser.act"

		(ZIhb) = re_char_class_ast_concat((ZIhead), (ZIbody));
	
#line 956 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-concat */
		/* BEGINNING OF ACTION: ast-expr-char-class */
		{
#line 695 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZIhb), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 971 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-char-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_range__endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 130 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1013 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 305 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIc = lex_state->buf.a[2];
		if ((unsigned char) ZIc > 127) {
			goto ZL1;
		}
		ZIc = (((toupper((unsigned char)ZIc)) - 64) % 128 + 128) % 128;

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1039 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 559 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1052 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 361 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 1101 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 321 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 1151 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 130 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 721 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (ZIc);
		(ZIr) = range;
	
#line 1171 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-range-endpoint-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOstart = ZIstart;
	*ZOend = ZIend;
}

static void
p_expr_C_Cliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 91 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI99;
					t_pos ZI100;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI99 = lex_state->lx.start;
		ZI100   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1216 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI93;
					t_pos ZI94;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 273 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZI93 = lex_state->lx.start;
		ZI94   = lex_state->lx.end;
	
#line 1250 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI97;
					t_pos ZI98;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 361 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI97 = lex_state->lx.start;
		ZI98   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 1303 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI95;
					t_pos ZI96;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 321 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI95 = lex_state->lx.start;
		ZI96   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 1356 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 91 */
		/* BEGINNING OF ACTION: ast-expr-literal */
		{
#line 595 "src/libre/parser.act"

		(ZInode) = re_ast_expr_literal((ZIc));
	
#line 1373 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_191(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSE):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 510 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 1412 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOf)
{
	t_char__class__ast ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI106;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 242 "src/libre/parser.act"

		ZI106 = '^';
	
#line 1439 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: char-class-ast-flag-invert */
			{
#line 709 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_INVERTED);
	
#line 1449 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-invert */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-flag-none */
			{
#line 705 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
	
#line 1462 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-none */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	*ZOf = ZIf;
}

static void
p_expr(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, &ZInode);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cflags(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_re__flags ZIempty__pos;
		t_re__flags ZIempty__neg;
		t_re__flags ZIpos;
		t_re__flags ZIneg;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENFLAGS):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 735 "src/libre/parser.act"

	        (ZIempty__pos) = RE_FLAGS_NONE;
	
#line 1523 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 735 "src/libre/parser.act"

	        (ZIempty__neg) = RE_FLAGS_NONE;
	
#line 1532 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF INLINE: 179 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_FLAG__UNKNOWN): case (TOK_FLAG__INSENSITIVE):
				{
					p_expr_C_Cflags_C_Cflag__set (flags, lex_state, act_state, err, ZIempty__pos, &ZIpos);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					ZIpos = ZIempty__pos;
				}
				break;
			}
		}
		/* END OF INLINE: 179 */
		/* BEGINNING OF INLINE: 181 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NEGATE):
				{
					ADVANCE_LEXER;
					p_expr_C_Cflags_C_Cflag__set (flags, lex_state, act_state, err, ZIempty__neg, &ZIneg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					ZIneg = ZIempty__neg;
				}
				break;
			}
		}
		/* END OF INLINE: 181 */
		/* BEGINNING OF INLINE: 184 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEFLAGS):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: ast-expr-re-flags */
				{
#line 743 "src/libre/parser.act"

		(ZInode) = re_ast_expr_re_flags((ZIpos), (ZIneg));
	
#line 1592 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-expr-re-flags */
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-closeflags */
				{
#line 545 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL1;
	
#line 1608 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closeflags */
				/* BEGINNING OF ACTION: ast-expr-empty */
				{
#line 583 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
	
#line 1617 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-expr-empty */
			}
		ZL4:;
		}
		/* END OF INLINE: 184 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char__class__ast ZIl;

		p_expr_C_Cchar_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZIl);
		/* BEGINNING OF INLINE: 157 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CHAR__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					t_char__class__ast ZIr;

					p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZIr);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: char-class-ast-concat */
					{
#line 683 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_concat((ZIl), (ZIr));
	
#line 1664 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: char-class-ast-concat */
				}
				break;
			default:
				{
					ZInode = ZIl;
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 157 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Htype(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__char__class__id ZIid;
		t_pos ZIstart;
		t_pos ZIend;
		t_char__class__ast ZInamed;

		switch (CURRENT_TERMINAL) {
		case (TOK_NAMED__CHAR__CLASS):
			/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
			{
#line 455 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZIid);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1728 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CHAR_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: char-class-ast-named-class */
		{
#line 691 "src/libre/parser.act"

		(ZInamed) = re_char_class_ast_named_class((ZIid));
	
#line 1742 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-named-class */
		/* BEGINNING OF ACTION: ast-expr-char-class */
		{
#line 695 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZInamed), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1757 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-char-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Clist_Hof_Hatoms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZI224;

		p_expr_C_Catom (flags, lex_state, act_state, err, &ZI224);
		p_226 (flags, lex_state, act_state, err, &ZI224, &ZInode);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Clist_Hof_Halts(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENCAPTURE): case (TOK_OPENGROUP):
	case (TOK_NAMED__CHAR__CLASS): case (TOK_OPENFLAGS): case (TOK_ESC): case (TOK_CONTROL):
	case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			t_ast__expr ZI221;

			p_expr_C_Calt (flags, lex_state, act_state, err, &ZI221);
			p_223 (flags, lex_state, act_state, err, &ZI221, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: ast-expr-empty */
			{
#line 583 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
	
#line 1823 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_223(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZI221, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			t_ast__expr ZIr;

			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 591 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((*ZI221), (ZIr));
	
#line 1861 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
		}
		break;
	default:
		{
			ZInode = *ZI221;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_226(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZI224, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_OPENSUB): case (TOK_OPENCAPTURE): case (TOK_OPENGROUP):
	case (TOK_NAMED__CHAR__CLASS): case (TOK_OPENFLAGS): case (TOK_ESC): case (TOK_CONTROL):
	case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			t_ast__expr ZIr;

			p_expr_C_Clist_Hof_Hatoms (flags, lex_state, act_state, err, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-concat */
			{
#line 587 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((*ZI224), (ZIr));
	
#line 1905 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-concat */
		}
		break;
	default:
		{
			ZInode = *ZI224;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

void
p_re__pcre(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 211 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 211 */
		/* BEGINNING OF INLINE: 212 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 552 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 1969 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 212 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_230(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__char__class__id *ZI227, t_pos *ZI228, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_range__endpoint ZIlower;
			t_range__endpoint ZIupper;
			t_pos ZIu__end;
			t_pos ZI150;
			t_pos ZI151;

			/* BEGINNING OF ACTION: ast-range-endpoint-char-class */
			{
#line 728 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_CHAR_CLASS;
		range.u.char_class.ctor = (*ZI227);
		(ZIlower) = range;
	
#line 2008 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-char-class */
			p_138 (flags, lex_state, act_state, err);
			p_142 (flags, lex_state, act_state, err, &ZIupper, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 571 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI228));
		mark(&act_state->rangeend,   &(ZIu__end));
	
#line 2024 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			ZI150 = *ZI228;
			ZI151 = ZIu__end;
			/* BEGINNING OF ACTION: char-class-ast-range */
			{
#line 654 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;
		AST_POS_OF_LX_POS(ast_start, (*ZI228));
		AST_POS_OF_LX_POS(ast_end, (ZIu__end));

		if ((ZIlower).t != AST_RANGE_ENDPOINT_LITERAL ||
		    (ZIupper).t != AST_RANGE_ENDPOINT_LITERAL) {
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
			    escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode) = re_char_class_ast_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
	
#line 2060 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-named-class */
			{
#line 691 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_named_class((*ZI227));
	
#line 2073 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-named-class */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIe;

		/* BEGINNING OF INLINE: 187 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-expr-any */
					{
#line 599 "src/libre/parser.act"

		(ZIe) = re_ast_expr_any();
	
#line 2112 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-any */
				}
				break;
			case (TOK_CONTROL):
				{
					t_char ZI192;
					t_pos ZI193;
					t_pos ZI194;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 305 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI192 = lex_state->buf.a[2];
		if ((unsigned char) ZI192 > 127) {
			goto ZL1;
		}
		ZI192 = (((toupper((unsigned char)ZI192)) - 64) % 128 + 128) % 128;

		ZI193 = lex_state->lx.start;
		ZI194   = lex_state->lx.end;
	
#line 2141 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 559 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 2154 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
					/* BEGINNING OF ACTION: ast-expr-empty */
					{
#line 583 "src/libre/parser.act"

		(ZIe) = re_ast_expr_empty();
	
#line 2163 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-empty */
				}
				break;
			case (TOK_OPENCAPTURE):
				{
					t_ast__expr ZIg;

					ADVANCE_LEXER;
					p_expr (flags, lex_state, act_state, err, &ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-expr-group */
					{
#line 609 "src/libre/parser.act"

		(ZIe) = re_ast_expr_group((ZIg));
	
#line 2184 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-group */
					p_191 (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_expr (flags, lex_state, act_state, err, &ZIe);
					p_191 (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUP):
				{
					p_expr_C_Cchar_Hclass (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_NAMED__CHAR__CLASS):
				{
					p_expr_C_Cchar_Htype (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENFLAGS):
				{
					p_expr_C_Cflags (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_expr_C_Cliteral (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 187 */
		/* BEGINNING OF INLINE: 195 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_pos ZI247;
					t_pos ZI248;
					t_unsigned ZIm;

					/* BEGINNING OF EXTRACT: OPENCOUNT */
					{
#line 263 "src/libre/parser.act"

		ZI247 = lex_state->lx.start;
		ZI248   = lex_state->lx.end;
	
#line 2262 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENCOUNT */
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 435 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL4;
		}

		ZIm = (unsigned int) u;
	
#line 2290 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_249 (flags, lex_state, act_state, err, &ZIe, &ZI247, &ZIm, &ZInode);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_OPT):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-opt */
					{
#line 629 "src/libre/parser.act"

		(ZIc) = ast_count(0, NULL, 1, NULL);
	
#line 2316 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-opt */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
	
#line 2325 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			case (TOK_PLUS):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-plus */
					{
#line 621 "src/libre/parser.act"

		(ZIc) = ast_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2341 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-plus */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
	
#line 2350 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			case (TOK_STAR):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-kleene */
					{
#line 617 "src/libre/parser.act"

		(ZIc) = ast_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2366 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-kleene */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
	
#line 2375 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			default:
				{
					t_ast__count ZIc;

					/* BEGINNING OF ACTION: atom-one */
					{
#line 625 "src/libre/parser.act"

		(ZIc) = ast_count(1, NULL, 1, NULL);
	
#line 2390 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-one */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
	
#line 2399 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 496 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL1;
	
#line 2417 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-count */
				ZInode = ZIe;
			}
		ZL3:;
		}
		/* END OF INLINE: 195 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_246(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI243, t_pos *ZI244, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_range__endpoint ZIlower;
			t_range__endpoint ZIupper;
			t_pos ZIu__end;
			t_pos ZI150;
			t_pos ZI151;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 721 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (*ZI243);
		(ZIlower) = range;
	
#line 2457 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_138 (flags, lex_state, act_state, err);
			p_142 (flags, lex_state, act_state, err, &ZIupper, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 571 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI244));
		mark(&act_state->rangeend,   &(ZIu__end));
	
#line 2473 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			ZI150 = *ZI244;
			ZI151 = ZIu__end;
			/* BEGINNING OF ACTION: char-class-ast-range */
			{
#line 654 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;
		AST_POS_OF_LX_POS(ast_start, (*ZI244));
		AST_POS_OF_LX_POS(ast_end, (ZIu__end));

		if ((ZIlower).t != AST_RANGE_ENDPOINT_LITERAL ||
		    (ZIupper).t != AST_RANGE_ENDPOINT_LITERAL) {
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
			    escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode) = re_char_class_ast_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
	
#line 2509 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-literal */
			{
#line 649 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_literal((*ZI243));
	
#line 2522 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-literal */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_249(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIe, t_pos *ZI247, t_unsigned *ZIm, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI198;
			t_pos ZIend;
			t_ast__count ZIc;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 268 "src/libre/parser.act"

		ZI198 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2557 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 576 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI247));
		mark(&act_state->countend,   &(ZIend));
	
#line 2568 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: expr-count */
			{
#line 633 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);
			mark(&act_state->countstart, &(*ZI247));
			mark(&act_state->countend,   &(ZIend));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZI247));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 2589 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: expr-count */
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((*ZIe), (ZIc));
	
#line 2598 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIend;
			t_pos ZI201;
			t_ast__count ZIc;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 435 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIn = (unsigned int) u;
	
#line 2635 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 268 "src/libre/parser.act"

		ZIend = lex_state->lx.start;
		ZI201   = lex_state->lx.end;
	
#line 2652 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 576 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI247));
		mark(&act_state->countend,   &(ZIend));
	
#line 2667 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: expr-count */
			{
#line 633 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);
			mark(&act_state->countstart, &(*ZI247));
			mark(&act_state->countend,   &(ZIend));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZI247));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 2688 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: expr-count */
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 613 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((*ZIe), (ZIc));
	
#line 2697 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Calt(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Clist_Hof_Hatoms (flags, lex_state, act_state, err, &ZInode);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

/* BEGINNING OF TRAILER */

#line 747 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->getc_opaque != NULL);

		lex_state = lx->getc_opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(re_getchar_fun *f, void *opaque,
		void (*entry)(flags, lex_state, act_state, err,
			struct ast_expr **),
		struct flags *flags, int overlap,
		struct ast_re *new, struct re_err *err)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(entry != NULL);

		if (err == NULL) {
			err = &dummy;
		}

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc       = lgetc;
		lx->getc_opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf_opaque = &lex_state->buf;
		lx->push       = CAT(LX_PREFIX, _dynpush);
		lx->clear      = CAT(LX_PREFIX, _dynclear);
		lx->free       = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(flags, lex_state, act_state, err, &new->expr);

		lx->free(lx->buf_opaque);

		if (err->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		return 0;

	error:

		/*
		 * Some errors describe multiple tokens; for these, the start and end
		 * positions belong to potentially different tokens, and therefore need
		 * to be stored statefully (in act_state). These are all from
		 * non-recursive productions by design, and so a stack isn't needed.
		 *
		 * Lexical errors describe a problem with a single token; for these,
		 * the start and end positions belong to that token.
		 *
		 * Syntax errors occur at the first point the order of tokens is known
		 * to be incorrect, rather than describing a span of bytes. For these,
		 * the start of the next token is most relevant.
		 */

		switch (err->e) {
		case RE_EOVERLAP:  err->start = act_state->groupstart; err->end = act_state->groupend; break;
		case RE_ENEGRANGE: err->start = act_state->rangestart; err->end = act_state->rangeend; break;
		case RE_ENEGCOUNT: err->start = act_state->countstart; err->end = act_state->countend; break;

		case RE_EHEXRANGE:
		case RE_EOCTRANGE:
		case RE_ECOUNTRANGE:
			/*
			 * Lexical errors: These are always generated for the current token,
			 * so lx->start/end here is correct because ADVANCE_LEXER has
			 * not been called.
			 */
			mark(&err->start, &lx->start);
			mark(&err->end,   &lx->end);
			break;

		default:
			/*
			 * Due to LL(1) lookahead, lx->start/end is the next token.
			 * This is approximately correct as the position of an error,
			 * but to be exactly correct, we store the pos for the previous token.
			 * This is more visible when whitespace exists.
			 */
			err->start = act_state->synstart;
			err->end   = act_state->synstart; /* single point */
			break;
		}

		return -1;
	}

	struct ast_re *
	DIALECT_PARSE(re_getchar_fun *f, void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct ast_re *ast;
		struct flags top, *fl = &top;

		top.flags = flags;

		assert(f != NULL);

		ast = re_ast_new();

		if (-1 == parse(f, opaque, DIALECT_ENTRY, fl, overlap, ast, err)) {
			goto error;
		}

		return ast;

	error:
		re_ast_free(ast);

		return NULL;
	}

#line 2897 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
