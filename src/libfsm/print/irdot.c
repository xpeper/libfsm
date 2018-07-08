/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include <print/esc.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "libfsm/internal.h"

#include "ir.h"

static unsigned int
indexof(const struct ir *ir, const struct ir_state *cs)
{
	assert(ir != NULL);
	assert(cs != NULL);

	return cs - &ir->states[0];
}

static const char *
strategy_name(enum ir_strategy strategy)
{
	switch (strategy) {
	case IR_NONE: return "NONE";
	case IR_SAME: return "SAME";
	case IR_MODE: return "MODE";
	case IR_MANY: return "MANY";
	case IR_JUMP: return "JUMP";

	default:
		return "?";
	}
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, unsigned char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	dot_escputc_html(f, opt, c);
}

static void
print_state(FILE *f, const struct ir *ir,
	const struct ir_state *to, const struct ir_state *self)
{
	assert(f != NULL);
	assert(ir != NULL);
	assert(self != NULL);

	if (to == NULL) {
		fprintf(f, "(none)");
	} else if (to == self) {
		fprintf(f, "(self)");
	} else {
		fprintf(f, "S%u", indexof(ir, to));
	}
}

static void
print_grouprows(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, const struct ir_state *self,
	const struct ir_group *groups, size_t n)
{
	size_t j, k;

	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		assert(groups[j].ranges != NULL);

		for (k = 0; k < groups[j].n; k++) {
			fprintf(f, "\t\t  <TR>");

			if (groups[j].ranges[k].start == groups[j].ranges[k].end) {
				fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].start);
				fprintf(f, "</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].start);
				fprintf(f, "</TD>");
				fprintf(f, "<TD ALIGN='LEFT'>");
				print_endpoint(f, opt, groups[j].ranges[k].end);
				fprintf(f, "</TD>");
			}

			if (k + 1 < groups[j].n) {
				fprintf(f, "<TD ALIGN='LEFT'>&#x21B4;</TD>");
			} else {
				fprintf(f, "<TD ALIGN='LEFT' PORT='group%u'>",
					(unsigned) j);
				print_state(f, ir, groups[j].to, self);
				fprintf(f, "</TD>");
			}

			fprintf(f, "</TR>\n");
		}
	}
}

static void
print_grouplinks(FILE *f, const struct ir *ir, const struct ir_state *self,
	const struct ir_group *groups, size_t n)
{
	size_t j;

	assert(f != NULL);
	assert(ir != NULL);
	assert(groups != NULL);

	for (j = 0; j < n; j++) {
		if (groups[j].to == NULL) {
			fprintf(f, "\tcs%u:group%u -> cs%s;\n",
				indexof(ir, self), (unsigned) j,
				"(none)");
		} else if (groups[j].to == self) {
			/* no edge drawn */
		} else {
			fprintf(f, "\tcs%u:group%u -> cs%u;\n",
				indexof(ir, self), (unsigned) j,
				indexof(ir, groups[j].to));
		}
	}
}

static void
print_cs(FILE *f, const struct fsm_options *opt,
	const struct ir *ir, const struct ir_state *cs)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ir != NULL);
	assert(cs != NULL);

	if (cs->isend) {
		fprintf(f, "\tcs%u [ peripheries = 2 ];\n", indexof(ir, cs));
	}

	fprintf(f, "\tcs%u [ label =\n", indexof(ir, cs));
	fprintf(f, "\t\t<<TABLE BORDER='0' CELLPADDING='2' CELLSPACING='0'>\n");
	fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>S%u</TD><TD ALIGN='LEFT'>%s</TD></TR>\n",
		indexof(ir, cs), strategy_name(cs->strategy));

	if (cs->example != NULL) {
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>example</TD><TD ALIGN='LEFT'>");
		escputs(f, opt, dot_escputc_html, cs->example);
		fprintf(f, "</TD></TR>\n");
	}

	/* TODO: leaf callback for dot output */

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>to</TD><TD ALIGN='LEFT'>");
		print_state(f, ir, cs->u.same.to, cs);
		fprintf(f, "</TD></TR>\n");
		break;

	case IR_MANY:
		print_grouprows(f, opt, ir, cs, cs->u.many.groups, cs->u.many.n);
		break;

	case IR_MODE:
		fprintf(f, "\t\t  <TR><TD COLSPAN='2' ALIGN='LEFT'>mode</TD><TD ALIGN='LEFT' PORT='mode'>");
		print_state(f, ir, cs->u.mode.mode, cs);
		fprintf(f, "</TD></TR>\n");
		print_grouprows(f, opt, ir, cs, cs->u.mode.groups, cs->u.mode.n);
		break;

	case IR_JUMP:
		/* TODO */
		break;

	default:
		;
	}

	fprintf(f, "\t\t</TABLE>> ];\n");

	switch (cs->strategy) {
	case IR_NONE:
		break;

	case IR_SAME:
		if (cs->u.same.to == cs) {
			/* no edge drawn */
		} else if (cs->u.same.to != NULL) {
			fprintf(f, "\tcs%u -> cs%u;\n",
				indexof(ir, cs), indexof(ir, cs->u.same.to));
		}
		break;

	case IR_MANY:
		print_grouplinks(f, ir, cs, cs->u.many.groups, cs->u.many.n);
		break;

	case IR_MODE:
		if (cs->u.mode.mode == cs) {
			/* no edge drawn */
		} else if (cs->u.mode.mode != NULL) {
			fprintf(f, "\tcs%u:mode -> cs%u;\n",
				indexof(ir, cs), indexof(ir, cs->u.mode.mode));
		}
		print_grouplinks(f, ir, cs, cs->u.mode.groups, cs->u.mode.n);
		break;

	case IR_JUMP:
		break;

	default:
		;
	}
}

void
fsm_print_ir(FILE *f, const struct fsm *fsm)
{
	struct ir *ir;
	size_t i;

	assert(f != NULL);
	assert(fsm != NULL);

	ir = make_ir(fsm);
	if (ir == NULL) {
		return;
	}

	fprintf(f, "digraph G {\n");

	fprintf(f, "\tnode [ shape = box, style = rounded ];\n");
	fprintf(f, "\trankdir = LR;\n");
	fprintf(f, "\troot = start;\n");
	fprintf(f, "\n");
	fprintf(f, "\tstart [ shape = none, label = \"\" ];\n");

	for (i = 0; i < ir->n; i++) {
		if (&ir->states[i] == ir->start) {
			fprintf(f, "\tstart -> cs%u;\n", (unsigned) i);
			fprintf(f, "\n");
		}

		print_cs(f, fsm->opt, ir, &ir->states[i]);
	}

	fprintf(f, "}\n");

	free_ir(ir);
}
