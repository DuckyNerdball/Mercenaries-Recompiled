/* F9 "Cheats" page.
 *
 * Retail cheats: the original RsCheats effect routines are called directly on
 * the retail main thread (no PDA screen, no code entry, no unlock gating).
 * Mapped from the retail XBE (sha256 AA08EA21...99D7):
 *   0x15E230 polls D-pad presses into a 25-entry buffer, 0x15C800 matches it
 *   against the code table and gate 0x3C0480::0x15ECD0, then jump table
 *   0x15E44C dispatches one of the effects below. Entry 5 is an empty (cut) case.
 *
 * Drops and faction presets: queued from the panel thread, executed at the
 * same world-update checkpoint as dev spawning. */
#ifndef MERCENARIES_CHEAT_MENU_H
#define MERCENARIES_CHEAT_MENU_H
#include <stdint.h>

typedef struct CheatEffect {
    const char *name;
    const char *help;
    uint32_t function; /* retail effect routine */
    uint32_t argument; /* one stack argument when has_argument */
    int has_argument;
} CheatEffect;

typedef struct CheatDrop {
    const char *name;
    const char *template_name; /* template_pic_* pickup */
    int weapon;                /* included in "Drop all weapons" */
} CheatDrop;

/* Deck of 52 numbered cards: one retail world layer per card ("sw_deck_clubs03")
 * holding the card NPC ("deck_clubs03") with its authored guards/vehicles.
 * Clubs/Diamonds live in the Southern province, Hearts/Spades in the Northern.
 * Face cards and Aces come from contracts (F9 Missions tab). */
typedef struct CheatCard {
    const char *name;   /* "Three of Clubs" */
    const char *layer;  /* "sw_deck_clubs03" */
    const char *actor;  /* "deck_clubs03" */
    int province;       /* 0 Southern, 1 Northern (recomp_dev_current_province) */
} CheatCard;
unsigned recomp_cheat_card_count(void);
const CheatCard *recomp_cheat_card_at(unsigned index);
#define CHEAT_CARD_ALL 0xFFFFu

/* Airstrikes through the retail Lua native Mission_SpawnAirstrike(plane, x, y, z,
 * heading_degrees) (0x11D650): it spawns the strike aircraft template and starts
 * its attack run on the point - no beacon, so it works on foot, driving or flying. */
/* plane = aircraft template (readiness check / generic beacon AirstrikeName).
 * beacon = optional retail support ammo template spawned instead of the generic
 * smoke-grenade beacon (gunship fly-time variants, artillery shells). */
typedef struct CheatStrike { const char *name; const char *plane; const char *beacon; } CheatStrike;
unsigned recomp_cheat_strike_count(void);
const CheatStrike *recomp_cheat_strike_at(unsigned index);

/* Persistent toggles, read by the game thread every ~0.5 s. */
void recomp_cheat_set_infinite_money(int on);
int recomp_cheat_infinite_money(void);
void recomp_cheat_set_density(int multiplier); /* 1..3 */
int recomp_cheat_density(void);
/* Warzone presets (hostile armor/infantry/fuel depots at the crosshair) and ambush waves. */
unsigned recomp_cheat_warzone_count(void);
const char *recomp_cheat_warzone_name(unsigned index);
void recomp_cheat_set_ambush(int on);
/* World activation range: scales every world object's authored wake-up /
 * hibernation distance (spore +0x2C/+0x2E). index 0..4 = x1, x1.5, x2, x3, x4. */
void recomp_cheat_set_activation_range(int index);
int recomp_cheat_activation_range(void);
float recomp_activation_scale_sq(void);
int recomp_cheat_ambush(void);
/* Quick strike (LB + R3, or F7): selected index, shared with the F9 strike list. */
void recomp_cheat_set_quick_strike(int index);
int recomp_cheat_quick_strike(void);
void recomp_cheat_fire_quick_strike(void);
int recomp_cheat_take_quick_strike(void);
void recomp_cheat_cycle_quick_strike(int direction); /* game input thread; announces on HUD */
long recomp_cheat_quick_strike_announce(void);
/* Controller/keyboard chord filter, called on every guest pad poll (port 0).
 * Holding LB is withheld from the game; LB + R3 fires, LB + D-pad L/R cycles.
 * A plain LB tap is delivered to the game on release. */

unsigned recomp_cheat_effect_count(void);
const CheatEffect *recomp_cheat_effect_at(unsigned index);
unsigned recomp_cheat_drop_count(void);
const CheatDrop *recomp_cheat_drop_at(unsigned index);

/* Panel thread -> game thread. */
#define CHEAT_ACTION_DROP      0x10000u /* | drop index */
#define CHEAT_ACTION_FRIENDLY  0x20000u
#define CHEAT_ACTION_HOSTILE   0x30000u
#define CHEAT_ACTION_DROP_ALL  0x40000u /* every weapon pickup, in a row */
#define CHEAT_ACTION_EFFECT    0x50000u /* | effect index */
#define CHEAT_ACTION_CARD      0x60000u /* | card index, or CHEAT_CARD_ALL */
#define CHEAT_ACTION_STRIKE    0x70000u /* | strike index */
#define CHEAT_ACTION_WARZONE   0x80000u /* | warzone preset index */
int recomp_cheat_request_action(unsigned action);
unsigned recomp_cheat_take_action(void);
void recomp_cheat_action_result(int success, const char *message);

/* Game thread, called from recomp_dev_spawn_tick. */
void recomp_dev_cheat_tick(void);
#endif
