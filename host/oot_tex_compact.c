/* (relink) The game's texture heap compaction, let through once in a while (nx2/oot_tex_compact.py).

   The texture data manager's per-frame step asks the heap's compaction thread for a round at every frame none runs. Each
   texture a round moves reaches the renderer as a new texture (upload, descriptors, sync: 3 to 5 ms in a busy place such
   as the market). nx2/oot_tex_compact.py has the step call hos_tex_compact_due just before it would ask; the answer is
   yes at most once every HOS_TEX_COMPACT_MS (default 1000). 0 is the game's own (every frame), "off" never asks.
   A round that has started still finishes: only the asking is spaced out. */
#include "hos.h"
#include <stdlib.h>
#include <string.h>

static int64_t tc_gap_ns = -2;       /* -2 not read yet, -1 never, 0 every frame */
static int64_t tc_last_ns;
static unsigned long tc_asked, tc_let;

static void tc_read(void) {
    const char *e = getenv("HOS_TEX_COMPACT_MS");
    int64_t ms = 1000;
    if (e && *e) {
        if (!strcmp(e, "off")) ms = -1;
        else ms = atoll(e);
    }
    tc_gap_ns = ms < 0 ? -1 : ms * 1000000;
    hos_log("tex: heap compaction round %s", ms < 0 ? "never asked for (HOS_TEX_COMPACT_MS=off)" : ms == 0 ? "asked for at every frame (the game's own)" : "asked for at most once per gap");
    if (ms > 0) hos_log("tex: the compaction gap is %lld ms (HOS_TEX_COMPACT_MS)", (long long)ms);
}

/* 1 when the step may ask for a round now. Only the game's step calls it, from its one thread. */
int hos_tex_compact_due(void) {
    if (tc_gap_ns == -2) tc_read();
    tc_asked++;
    int yes;
    if (tc_gap_ns == 0) {
        yes = 1;
    } else if (tc_gap_ns < 0) {
        yes = 0;
    } else {
        const int64_t now = hos_now_ns();
        yes = tc_last_ns == 0 || now - tc_last_ns >= tc_gap_ns;
        if (yes) tc_last_ns = now;
    }
    if (yes) tc_let++;
    if (tc_asked % 3600 == 0) hos_log("tex: heap compaction: %lu rounds let through in %lu asks", tc_let, tc_asked);
    return yes;
}
