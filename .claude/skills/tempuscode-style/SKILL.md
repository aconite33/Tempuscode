---
name: tempuscode-style
description: Enforce Tempuscode (CircleMUD-derived C) coding conventions. Invoke before committing or opening a PR in this repo to audit new code against the project baseline. Triggers on phrases like "check style", "style review", "review conventions", or "audit the code".
---

# Tempuscode coding conventions

This skill audits changes in the Tempuscode repo against the conventions used by the baseline (pre-existing, human-written) code. When invoked, walk through the `Review procedure` below and report divergences with file:line citations. Do not auto-fix; present findings for the user to approve.

The rules here come from surveying the real baseline (branch `main`), not from generic C style guides. When two rules in general style conflict, follow what Tempuscode actually does.

## Review procedure

1. Identify the changed code. Prefer in order:
   - If the user named files, use those.
   - Otherwise `git diff --staged` for pre-commit.
   - Otherwise `git diff main...HEAD` for a branch review.
2. Run the greps under `Automated checks` against those files.
3. For each manual rule below, scan the added/modified lines and flag any violation.
4. Produce a single report organized by severity:
   - **Blockers**: em dashes, `--` in prose strings, ASCII art, Doxygen headers, section banners. These will make the code look AI-generated.
   - **Style**: brace/indent, variable declaration position, function header format, comment voice.
   - **Idiom**: `tmp_*` vs `malloc`, `damage()` vs direct HP writes, `act()` pronoun tokens.
5. For each finding, cite file:line. Quote the violating snippet and the baseline convention it breaks.
6. Stop. Do not edit unless the user asks you to fix.

## Automated checks

Run these against the changed files:

```
grep -nP "—"          <files>       # em dash hits
grep -nP " -- "       <files>       # double dash in prose
grep -nP "/\*\s*=+.*=+\s*\*/" <files> # banner comments
grep -nP "/\*\*"      <files>       # Doxygen openers
grep -nP "@param|@return" <files>   # Doxygen tags
grep -nE "^\s*//.*\s+$" <files>     # trailing whitespace on comments
grep -nP "<!--\s*=" sample_lib/etc/*.xml  # XML section dividers
```

Every hit is a finding.

## Rules

### File header (C source files)

Every `.c` file under `src/` opens with a five-line banner:

```
//
// File: <filename>            -- Part of TempusMUD
//
// Copyright 1998 by John Watson, all rights reserved.
//
```

Reference: `src/classes/act.monk.c:1-5`, `src/classes/act.cleric.c:1-5`, `src/magic/magic.c:1-10`.

Some files preserve an earlier CircleMUD block before the TempusMUD block. Keep both when editing existing files; new files use only the TempusMUD block.

### Include ordering

Order, with a blank line between groups:

1. System headers: `<stdio.h>`, `<string.h>`, `<stdint.h>`, `<stdbool.h>`, `<ctype.h>`, `<time.h>`, `<errno.h>`, `<unistd.h>`.
2. Third-party: `<glib.h>`, `<libpq-fe.h>`, `<libxml/parser.h>`.
3. Project headers, quoted: `"interpreter.h"`, `"structs.h"`, `"utils.h"`, `"constants.h"`, `"comm.h"`, etc.

Reference: `src/classes/act.monk.c:11-44`.

### Braces and indent

- 4 spaces, never tabs.
- K&R: opening brace on the same line as the function or control keyword.
- `else` on its own line, after the closing brace of the preceding `if` block.
- Multi-line function calls indent the continuation to align with the first argument.

Reference: `src/classes/act.monk.c:253-272` (whole `do_whirlwind` opener).

### Variable declarations

C89 layout: all local declarations sit at the top of the function or block, before any executable statement. No mid-block declarations.

```c
ACMD(do_whirlwind)
{
    struct creature *vict = NULL;
    struct obj_data *ovict = NULL;
    int percent = 0, prob = 0, i;
    char *arg;

    arg = tmp_getword(&argument);
```

Reference: `src/classes/act.monk.c:253-260`.

- Pointers initialize to `NULL`, never `0`.
- `bool` comes from `<stdbool.h>`.
- Multiple declarations per line are fine when the type is the same.

### Comments

- Single-line comments use `//`. Multi-line blocks use `/* */` but only for license headers and rare long explanations.
- Function header above each `ACMD` / `ASPELL` / significant static function is a three-line `//` block:

```c
//
// whirlwind attack
//
```

Reference: `src/classes/act.monk.c:248-251`, and every other function header in that file.

- Inline comments inside functions use `//`, with a space after the slashes. Keep them short and declarative.
- No `/* ================ SECTION ================ */` banners. The baseline never uses them, inside functions or between case labels.
- No `} /* switch */` or `} /* end if */` closing-brace labels.
- No TODO / FIXME / XXX comments in committed code unless the item is tracked elsewhere.

### Names

- Functions and locals: `snake_case`.
- Macros, enum values, constants: `ALL_CAPS_WITH_UNDERSCORES`.
- Structs: `struct foo_bar` used directly in types. No typedef aliases in application code.
- Class predicate macros: `IS_<CLASS>(ch)`, e.g. `IS_MAGE`, `IS_CLERIC`, `IS_WARLOCK`.
- Bool-returning helpers: `is_<x>()`, `can_<x>()`, `has_<x>()`, `affected_by_<x>()`.

### User-facing output strings

- Always end `\r\n`.
- Capitalize the first letter of each sentence.
- One sentence per error message. Do not chain two sentences with a period.
- Prefer `cannot` over `can't`.
- "who?" not "whom?" in target-missing prompts. Reference: `src/combat/act.offensive.c` has `"Backstab who?"`, `src/classes/act.monk.c:271` has `"Whirlwind who?"`.
- Patron error phrasing:

```c
send_to_char(ch, "You fail.\r\n");
send_to_char(ch, "You are too exhausted!\r\n");
send_to_char(ch, "You cannot summon the energy to do so.\r\n");
```

- Use `send_to_char` for direct messages, `act` for messages involving multiple creatures with pronoun substitution, `d_printf` for pre-game / nanny output.

### act() phrasing

- Active voice, present tense, one sentence.
- Standard CircleMUD pronoun tokens only: `$n` (actor name), `$N` (target name), `$m`/`$M` (actor him/her), `$s`/`$S` (actor his/her), `$e`/`$E` (actor he/she), `$p` (object).
- Three-recipient tuples use `to_char`, `to_vict`, `to_room` or `to_notvict`. Messages to each should describe the same action from that viewer's perspective.

Reference: `src/magic/spell_parser.c:138-164`, `src/magic/spells.c:1967-1980`.

### Command and spell idioms

**ACMD opener** (reference `src/classes/act.monk.c:253-260`):

```c
ACMD(do_x)
{
    struct creature *vict = NULL;
    /* other locals */
    char *arg;

    arg = tmp_getword(&argument);
    if (!*arg) {
        vict = random_opponent(ch);
    } else {
        vict = get_char_room_vis(ch, arg);
    }
    if (!vict) {
        send_to_char(ch, "X who?\r\n");
        return;
    }
```

**ASPELL opener** (reference `src/magic/spells.c:1957-1990`):

```c
ASPELL(spell_x)
{
    struct affected_type af;
    int dam = 0;

    init_affect(&af);
    /* ... */
}
```

**Rules of thumb:**
- Use `damage(ch, vict, NULL, dam, SPELL_X, -1)` or `WEAR_RANDOM` to deal spell damage. Never mutate `GET_HIT()` directly to deal damage (only to heal or cap).
- Call `init_affect(&af)` on every new `struct affected_type` before populating it.
- Call `gain_skill_prof(ch, SPELL_X)` on successful cast/skill use.
- End the function with `WAIT_STATE(ch, PULSE_VIOLENCE * N)` for offensive abilities. `N` is 1-2 for light, 3 for heavy.
- Transient strings come from `tmp_getword`, `tmp_strdup`, `tmp_sprintf`. Never `strdup`/`malloc` for per-command strings.

### Switch dispatch style

- No blank lines between adjacent `case:` blocks.
- `break;` at the end of each case.
- Fallthrough: stack the case labels with no `/* fallthrough */` comment between them.
- No section banner comments grouping cases together.
- No `default:` clause unless the switch truly benefits from one. When present, use `errlog` for unexpected values.

Reference: `src/magic/spell_parser.c:528-610` (MAG_MANUAL dispatch — dozens of cases, no dividers).

### Memory management

- `tmp_*` family auto-frees at pulse end. Use it for anything command-scoped.
- `CREATE` macro wraps malloc for long-lived allocations.
- `free` paired explicitly with `malloc`. Never `free` a `tmp_*` result.

### Logging

- `errlog(fmt, ...)` for error conditions.
- `slog(fmt, ...)` for boot-time and load-time progress.
- `mudlog(level, brief, fmt, ...)` for events admins should see in-game.
- `do_stat` / `ch->desc` output uses `send_to_char` or `d_printf`, not `slog`.

### XML data files (`sample_lib/etc/*.xml`)

- 2-space indent.
- On `<spell>` / `<skill>` elements: `id` attribute first, then `name`.
- Child element order inside a `<spell>`: `<granted>` (multiple allowed), `<cost>`, `<position>`, `<target>` (multiple), `<flag>` (multiple), `<affectmessage>`, `<potion>`.
- Self-closing for elements with only attributes, e.g. `<granted class="mage" level="5"/>`.
- No `<!-- section name -->` dividers between spells. Spells flow in file order, not grouped by class.

### Damage-message records (`sample_lib/misc/messages`)

Every record is exactly 12 lines after the `M` and ID:

```
M
 <id>
<death char>
<death vict>
<death room>
<miss char>
<miss vict>
<miss room>
<hit char>
<hit vict>
<hit room>
<god char>
<god vict>
<god room>
```

Use `#` on a line to suppress that message. Never skip a line — each record is exactly 12 after the ID. Reference: existing entries throughout `sample_lib/misc/messages`.

## AI-tell blacklist

These patterns are not present anywhere in the baseline. Using them in new code is an immediate tell that the code was generated.

- **Em dash (`—`)**. Not a single em dash in the whole baseline. Use a comma, period, semicolon, or parentheses.
- **`--` inside string literals**. The baseline uses `--` only as the C decrement operator and in comment-separator dashes that have spaces around them. A string like `"Your pact falters -- the Abyss answers."` is wrong.
- **Doxygen-style headers**. No `/**`, no `@param`, no `@return`, no `@brief`. Function headers are three-line `//` comments.
- **Section banner comments**. No `/* ============ SECTION ============ */` or `// ---- section ----`. Baseline groups code by file and position, not in-file banners.
- **ASCII art**. No box-drawing characters, no `+---+---+` tables, no decorative separators.
- **XML dividers**. No `<!-- ===== Class X ===== -->` dividers between groups of XML elements.
- **Defensive nulls on guaranteed pointers**. If the baseline trusts a pointer (e.g. `ch` in an ACMD is always non-null), new code should too.
- **Hedging prose in comments**. No "might be", "could potentially", "in general", "this should probably". Comments state facts.
- **Stdlib explanations**. Do not annotate `dice(1, 6)` or `number(0, 100)` with what they do. Readers know.
- **Multi-sentence error strings**. `"Your stigmata fails.\r\n"` not `"Your stigmata fails. Another symbol already burns.\r\n"`. If two reasons are possible, pick the primary one.
- **Trailing whitespace** on any line, especially comments.
- **"Simply"**, **"just"**, **"merely"**, **"of course"**, **"obviously"** in comments. Baseline never uses these.

## Reference files to cite

When the user asks "where's the pattern for X", point at these:

- Class file layout: `src/classes/act.monk.c` (lines 1-44 for header/includes, 248-420 for a full ACMD).
- ACMD with auto-targeting: `src/classes/act.monk.c:253-419` (`do_whirlwind`).
- ASPELL body (affect-based): `src/magic/spells.c:1957-2030` (`spell_death_knell`).
- ASPELL body (summon with scaling): `src/magic/spells.c:2572-2695` (`spell_summon_legion`).
- `mag_affects` case: `src/magic/magic.c:1212-1225` (`SPELL_CURSE`).
- `mag_damage` case: `src/magic/magic.c:616-620` (`SPELL_MAGIC_MISSILE`).
- DoT tick loop: `src/mobiles/mobact.c:470-481` (`SPELL_STIGMATA` tick).
- MAG_MANUAL dispatch entry: `src/magic/spell_parser.c:608-610` (`SPELL_SUMMON_LEGION`).
- XML spell entry: `sample_lib/etc/spells.xml:831-843` (`<spell id="17" name="curse">`).

## Output format

When reporting, structure the report as:

```
## Style audit: <branch or file list>

### Blockers (N)

- path/to/file.c:123 — em dash in string literal
    "Your pact falters — the Abyss answers."
  Fix: replace with comma or period.

### Style (N)

- path/to/file.c:45 — mid-block variable declaration
    int x = 3;  (inside for-loop body)
  Fix: move to top of function per C89 layout used elsewhere in the file.

### Idiom (N)

- path/to/file.c:67 — direct GET_HIT manipulation to deal damage
  Fix: use damage(ch, vict, NULL, amount, SPELL_X, -1).
```

Blockers must be cleared before committing. Style/idiom items are discussed with the user.
