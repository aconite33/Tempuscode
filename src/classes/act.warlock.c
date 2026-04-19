//
// File: act.warlock.c                  -- Part of TempusMUD
//
// Signature at-will ability and helpers for the Warlock class.
//

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <glib.h>

#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include "constants.h"
#include "comm.h"
#include "security.h"
#include "handler.h"
#include "defs.h"
#include "desc_data.h"
#include "macros.h"
#include "room_data.h"
#include "zone_data.h"
#include "race.h"
#include "creature.h"
#include "libpq-fe.h"
#include "db.h"
#include "account.h"
#include "screen.h"
#include "char_class.h"
#include "tmpstr.h"
#include "spells.h"
#include "materials.h"
#include "fight.h"
#include <libxml/parser.h>
#include "obj_data.h"
#include "strutil.h"

ACMD(do_eldritch_blast)
{
    struct creature *vict = NULL;
    char *arg;
    int beams, dam, i;
    int lvl;

    arg = tmp_getword(&argument);

    if (!IS_WARLOCK(ch) && GET_LEVEL(ch) < LVL_IMMORT) {
        send_to_char(ch, "You haven't forged the pact required for that.\r\n");
        return;
    }

    if (!*arg) {
        vict = random_opponent(ch);
    } else {
        vict = get_char_room_vis(ch, arg);
    }

    if (!vict) {
        send_to_char(ch, "Blast whom?\r\n");
        return;
    }
    if (vict == ch) {
        send_to_char(ch, "You cannot blast yourself.\r\n");
        return;
    }
    if (!ok_to_attack(ch, vict, true)) {
        return;
    }
    if (AFF3_FLAGGED(ch, AFF3_MUTED)) {
        send_to_char(ch, "You cannot speak the eldritch words.\r\n");
        return;
    }
    if (GET_MANA(ch) < 3 || GET_MOVE(ch) < 5) {
        send_to_char(ch, "You lack the eldritch energy to invoke.\r\n");
        return;
    }

    /* Effective level scales with remort generation. */
    lvl = GET_LEVEL(ch) + GET_REMORT_GEN(ch) * 2;
    if (lvl >= 45) {
        beams = 4;
    } else if (lvl >= 30) {
        beams = 3;
    } else if (lvl >= 15) {
        beams = 2;
    } else {
        beams = 1;
    }

    GET_MANA(ch) -= 3;
    GET_MOVE(ch) -= 5;

    act("You unleash a crackling beam of eldritch energy at $N!",
        false, ch, NULL, vict, TO_CHAR);
    act("$n unleashes a crackling beam of eldritch energy at YOU!",
        false, ch, NULL, vict, TO_VICT);
    act("$n unleashes a crackling beam of eldritch energy at $N!",
        false, ch, NULL, vict, TO_NOTVICT);

    for (i = 0; i < beams; i++) {
        if (is_dead(vict) || vict->in_room != ch->in_room) {
            break;
        }
        if (CHECK_SKILL(ch, SKILL_ELDRITCH_BLAST) + GET_CHA(ch)
            > number(1, 101) + GET_DEX(vict)) {
            dam = dice(1 + (lvl > 4), 8) + lvl / 3;
        } else {
            dam = 0;  /* beam fizzles */
        }
        dam = warlock_align_scale(ch, dam);
        damage(ch, vict, NULL, dam, SKILL_ELDRITCH_BLAST, -1);
        if (is_dead(ch)) {
            return;
        }
    }

    gain_skill_prof(ch, SKILL_ELDRITCH_BLAST);
    WAIT_STATE(ch, PULSE_VIOLENCE);
}
