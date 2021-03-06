/* Generated by libfsm */

#include LF_HEADER

#include <stddef.h>

#include <fsm/fsm.h>

struct fsm *
utf8_Thai_fsm(const struct fsm_options *opt)
{
	struct fsm *fsm;
	size_t i;

	struct fsm_state *s[5] = { 0 };

	fsm = fsm_new(opt);
	if (fsm == NULL) {
		return NULL;
	}

	for (i = 0; i < 5; i++) {
		s[i] = fsm_addstate(fsm);
		if (s[i] == NULL) {
			goto error;
		}
	}

	if (!fsm_addedge_literal(fsm, s[0], s[1], 0xe0)) { goto error; }
	if (!fsm_addedge_literal(fsm, s[1], s[2], 0xb8)) { goto error; }
	if (!fsm_addedge_literal(fsm, s[1], s[3], 0xb9)) { goto error; }
	for (i = 0x81; i <= 0xba; i++) {
		if (!fsm_addedge_literal(fsm, s[2], s[4], i)) { goto error; }
	}
	for (i = 0x80; i <= 0x9b; i++) {
		if (!fsm_addedge_literal(fsm, s[3], s[4], i)) { goto error; }
	}

	fsm_setstart(fsm, s[0]);
	fsm_setend(fsm, s[4], 1);

	return fsm;

error:

	fsm_free(fsm);

	return NULL;
}

