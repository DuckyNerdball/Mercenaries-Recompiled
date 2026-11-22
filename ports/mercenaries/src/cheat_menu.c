/* Cheat catalogue and the panel->game action queue. See cheat_menu.h.
 * Nothing here reads or writes guest memory. */
#include <windows.h>
#include "cheat_menu.h"

#define FX(n, h, f) {n, h, f, 0u, 0}
#define OUTFIT(n, hash) {"Outfit: " n, "Retail costume cheat: swaps the player model.", 0x0015CAD0u, hash, 1}
static const CheatEffect cheat_effects[] = {
    FX("God mode (toggle)", "Retail invulnerability toggle. The game clears it when a contract ends.", 0x0015C8E0u),
    FX("Infinite ammo (toggle)", "Retail infinite-ammunition toggle.", 0x0015C920u),
    FX("+$1,000,000", "Adds one million dollars through the retail SetMoney path.", 0x0015C940u),
    FX("Unlock all shop items", "Retail Merchant of Menace unlock (includes the Cheat Weapons crate).", 0x0015C980u),
    FX("Reset faction moods", "Retail reset: reloads every faction's standing from RS.INI.", 0x0015C9B0u),
    FX("Unlock all PDA cheat codes", "Removes the progress requirement for typing codes on the PDA Factions screen.", 0x0015CAA0u),
    OUTFIT("Prisoner", 0x87912831u),
    OUTFIT("NK Elite Soldier", 0x28416B1Fu),
    OUTFIT("SK Elite Soldier", 0x55C024CCu),
    OUTFIT("NK Ace of Diamonds", 0x579B5B99u),
    OUTFIT("UN Doctor", 0xF3356BF7u),
    OUTFIT("Mafia Heavy Soldier", 0x098E9176u),
    OUTFIT("NK Spade Card", 0x24A16E5Bu),
    OUTFIT("NK Heart Card", 0x1678F628u),
    OUTFIT("Allied Pilot", 0xE53D989Fu),
    OUTFIT("Allied Boss", 0x774D4842u),
    OUTFIT("Chinese Boss", 0xDD9DD0ABu),
    OUTFIT("Josef (Mafia)", 0x4A5D84BCu),
    OUTFIT("SK Boss", 0x2D0807CEu),
    OUTFIT("Han Solo", 0xBC8AC23Au),
    OUTFIT("Indiana Jones", 0x467B211Cu),
};
static const CheatDrop cheat_drops[] = {
    {"Health", "template_pic_health", 0},
    {"Armor", "template_pic_armor", 0},
    {"Ammo pack", "template_pic_ammopack", 0},
    {"Vehicle ammo", "template_pic_ammoveh", 0},
    {"Grenades", "template_pic_grenades", 0},
    {"Money", "template_pic_money", 0},
    {"Cheat: Portable Air Strike", "template_pic_cheatgun", 1},
    {"Cheat: Street Sweeper", "template_pic_cheatsmg", 1},
    {"Cheat: Pocket Artillery", "template_pic_cheatgl", 1},
    {"Assault Rifle (AK-47)", "template_pic_ak47", 1},
    {"Carbine (M4)", "template_pic_m4", 1},
    {"SMG (Type 85)", "template_pic_smg", 1},
    {"Covert SMG (MP5)", "template_pic_mp5", 1},
    {"Shotgun", "template_pic_shotgun", 1},
    {"Light MG (RPD)", "template_pic_lmg", 1},
    {"Prototype Rifle (MG36)", "template_pic_mg36", 1},
    {"Sniper Rifle", "template_pic_sniperrifle", 1},
    {"Covert Rifle", "template_pic_covertrifle", 1},
    {"Anti-Armor Rifle", "template_pic_amrifle", 1},
    {"RPG", "template_pic_rpg", 1},
    {"Anti-Tank Rocket (SMAW)", "template_pic_smaw", 1},
    {"Anti-Air Rocket (Stinger)", "template_pic_stinger", 1},
    {"Frag grenades", "template_pic_fraggrenade", 1},
    {"Stun grenades", "template_pic_stungrenade", 1},
    {"Smoke signal", "template_pic_smokegrenade", 1},
    {"C4", "template_pic_c4", 1},
    {"C4 detonator", "template_pic_c4detonator", 1},
    {"Bribe Mafia", "template_pic_mafiabribe", 0},
};
static const CheatCard cheat_cards[] = {
    /* sw_deck_clubs02 is empty in retail: the Two of Clubs is the tutorial capture. */
    {"Three of Clubs", "sw_deck_clubs03", "deck_clubs03", 0},
    {"Four of Clubs", "sw_deck_clubs04", "deck_clubs04", 0},
    {"Five of Clubs", "sw_deck_clubs05", "deck_clubs05", 0},
    {"Six of Clubs", "sw_deck_clubs06", "deck_clubs06", 0},
    {"Seven of Clubs", "sw_deck_clubs07", "deck_clubs07", 0},
    {"Eight of Clubs", "sw_deck_clubs08", "deck_clubs08", 0},
    {"Nine of Clubs", "sw_deck_clubs09", "deck_clubs09", 0},
    {"Ten of Clubs", "sw_deck_clubs10", "deck_clubs10", 0},
    {"Two of Diamonds", "sw_deck_diamonds02", "deck_diamonds02", 0},
    {"Three of Diamonds", "sw_deck_diamonds03", "deck_diamonds03", 0},
    {"Four of Diamonds", "sw_deck_diamonds04", "deck_diamonds04", 0},
    {"Five of Diamonds", "sw_deck_diamonds05", "deck_diamonds05", 0},
    {"Six of Diamonds", "sw_deck_diamonds06", "deck_diamonds06", 0},
    {"Seven of Diamonds", "sw_deck_diamonds07", "deck_diamonds07", 0},
    {"Eight of Diamonds", "sw_deck_diamonds08", "deck_diamonds08", 0},
    {"Nine of Diamonds", "sw_deck_diamonds09", "deck_diamonds09", 0},
    {"Ten of Diamonds", "sw_deck_diamonds10", "deck_diamonds10", 0},
    {"Two of Hearts", "nw_deck_hearts02", "deck_hearts02", 1},
    {"Three of Hearts", "nw_deck_hearts03", "deck_hearts03", 1},
    {"Four of Hearts", "nw_deck_hearts04", "deck_hearts04", 1},
    {"Five of Hearts", "nw_deck_hearts05", "deck_hearts05", 1},
    {"Six of Hearts", "nw_deck_hearts06", "deck_hearts06", 1},
    {"Seven of Hearts", "nw_deck_hearts07", "deck_hearts07", 1},
    {"Eight of Hearts", "nw_deck_hearts08", "deck_hearts08", 1},
    {"Nine of Hearts", "nw_deck_hearts09", "deck_hearts09", 1},
    {"Ten of Hearts", "nw_deck_hearts10", "deck_hearts10", 1},
    {"Two of Spades", "nw_deck_spades02", "deck_spades02", 1},
    {"Three of Spades", "nw_deck_spades03", "deck_spades03", 1},
    {"Four of Spades", "nw_deck_spades04", "deck_spades04", 1},
    {"Five of Spades", "nw_deck_spades05", "deck_spades05", 1},
    {"Six of Spades", "nw_deck_spades06", "deck_spades06", 1},
    {"Seven of Spades", "nw_deck_spades07", "deck_spades07", 1},
    {"Eight of Spades", "nw_deck_spades08", "deck_spades08", 1},
    {"Nine of Spades", "nw_deck_spades09", "deck_spades09", 1},
    {"Ten of Spades", "nw_deck_spades10", "deck_spades10", 1},
};
/* Aircraft per retail beacon template (template_sam_* airstrikename). The
 * Chinese "66" aircraft and tornado1/f2 are defined but unused by any beacon. */
static const CheatStrike cheat_strikes[] = {
    {"Cluster Bomb (A-10)", "template_allies_support_a10", NULL},
    {"Tank Buster (A-10)", "template_allies_support_a10b", NULL},
    {"Carpet Bomb (B-52)", "template_allies_support_b52", NULL},
    {"Bunker Buster (F-15)", "template_allies_support_f15", NULL},
    {"Surgical Strike (Tornado)", "template_allies_support_tornado", NULL},
    {"Laser-Guided Missile (EF2000)", "template_allies_support_ef2000", NULL},
    {"Stealth Fighter Attack (F-117)", "template_sk_support_f117", NULL},
    {"Stealth Bomber Attack (B-2)", "template_sk_support_b2", NULL},
    {"Air Superiority (F-22)", "template_sk_support_f22", NULL},
    {"Gunship (AC-130, 10 s)", "template_sk_support_ac130", "template_sam_gunship"},
    {"Gunship 2 (AC-130, 20 s)", "template_sk_support_ac130", "template_sam_gunship2"},
    {"Gunship 3 (AC-130, 45 s)", "template_sk_support_ac130", "template_sam_gunship3"},
    {"Cruise Missile", "template_sk_support_cruise", NULL},
    {"Fuel-Air Bomb (Su-25)", "template_china_support_frogfoot", NULL},
    {"Strategic Missile (FROG)", "template_mafia_support_frog", NULL},
    {"Artillery Barrage (China, 30 m)", "template_china_support_66", "template_sam_artillery"},
    {"Artillery Barrage 2 (China, 45 m)", "template_china_support_66_2", "template_sam_artillery2"},
    {"Artillery Barrage 3 (China, 60 m)", "template_china_support_66_3", "template_sam_artillery3"},
    {"DEV: Killbox (F-22)", "template_f22_for_killbox", NULL},
    {"DEV: Killbox (NK Su-25)", "template_nk_killbox", NULL},
    {"CUT: Airburst (China 66air)", "template_china_support_66air", NULL},
    {"CUT: Air Superiority (F-15 'F2')", "template_allies_support_f2", NULL},
    {"CUT: Surgical Strike (Tornado alt)", "template_allies_support_tornado1", NULL},
};
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
unsigned recomp_cheat_strike_count(void) { return (unsigned)COUNT(cheat_strikes); }
const CheatStrike *recomp_cheat_strike_at(unsigned i) { return i < COUNT(cheat_strikes) ? &cheat_strikes[i] : NULL; }
static volatile LONG g_infinite_money, g_density = 1;
void recomp_cheat_set_infinite_money(int on) { InterlockedExchange(&g_infinite_money, on ? 1 : 0); }
int recomp_cheat_infinite_money(void) { return (int)InterlockedCompareExchange(&g_infinite_money, 0, 0); }
void recomp_cheat_set_density(int m) { InterlockedExchange(&g_density, m < 1 ? 1 : m > 3 ? 3 : m); }
int recomp_cheat_density(void) { return (int)InterlockedCompareExchange(&g_density, 0, 0); }
static const char *const cheat_warzones[] = {
    "Tank field: 8 NK tanks and AA",
    "Armored assault: NK tanks, APCs, SAM, rockets + infantry",
    "Infantry company: 30 NK soldiers",
    "Chinese armor column (hostile only if China is)",
    "Fuel depot: 12 explosive fuel tanks",
    "All-out war: tank field + infantry + fuel depot",
};
unsigned recomp_cheat_warzone_count(void) { return (unsigned)COUNT(cheat_warzones); }
const char *recomp_cheat_warzone_name(unsigned i) { return i < COUNT(cheat_warzones) ? cheat_warzones[i] : NULL; }
static volatile LONG g_activation_range;
static const float activation_scale_sq[] = {1.f, 2.25f, 4.f, 9.f, 16.f};
void recomp_cheat_set_activation_range(int i) { InterlockedExchange(&g_activation_range, i < 0 ? 0 : i > 4 ? 4 : i); }
int recomp_cheat_activation_range(void) { return (int)g_activation_range; }
float recomp_activation_scale_sq(void) { return activation_scale_sq[(unsigned)g_activation_range % 5u]; }
static volatile LONG g_ambush;
void recomp_cheat_set_ambush(int on) { InterlockedExchange(&g_ambush, on ? 1 : 0); }
int recomp_cheat_ambush(void) { return (int)InterlockedCompareExchange(&g_ambush, 0, 0); }
/* Quick strike: hotkey/controller chord fires the selected strike at the aim point. */
static volatile LONG g_quick_strike = 1, g_quick_pending, g_quick_announce;
void recomp_cheat_set_quick_strike(int i)
{
    int n = (int)COUNT(cheat_strikes);
    InterlockedExchange(&g_quick_strike, ((i % n) + n) % n);
}
int recomp_cheat_quick_strike(void) { return (int)InterlockedCompareExchange(&g_quick_strike, 0, 0); }
void recomp_cheat_fire_quick_strike(void) { InterlockedExchange(&g_quick_pending, 1); }
int recomp_cheat_take_quick_strike(void) { return (int)InterlockedExchange(&g_quick_pending, 0); }
void recomp_cheat_cycle_quick_strike(int direction)
{
    recomp_cheat_set_quick_strike(recomp_cheat_quick_strike() + direction);
    InterlockedIncrement(&g_quick_announce);
}
long recomp_cheat_quick_strike_announce(void) { return (long)InterlockedCompareExchange(&g_quick_announce, 0, 0); }
unsigned recomp_cheat_card_count(void) { return (unsigned)COUNT(cheat_cards); }
const CheatCard *recomp_cheat_card_at(unsigned i) { return i < COUNT(cheat_cards) ? &cheat_cards[i] : NULL; }
unsigned recomp_cheat_effect_count(void) { return (unsigned)COUNT(cheat_effects); }
const CheatEffect *recomp_cheat_effect_at(unsigned i) { return i < COUNT(cheat_effects) ? &cheat_effects[i] : NULL; }
unsigned recomp_cheat_drop_count(void) { return (unsigned)COUNT(cheat_drops); }
const CheatDrop *recomp_cheat_drop_at(unsigned i) { return i < COUNT(cheat_drops) ? &cheat_drops[i] : NULL; }

static volatile LONG g_action;
int recomp_cheat_request_action(unsigned action)
{
    if (!action) return 0;
    return InterlockedCompareExchange(&g_action, (LONG)action, 0) == 0;
}
unsigned recomp_cheat_take_action(void) { return (unsigned)InterlockedExchange(&g_action, 0); }
