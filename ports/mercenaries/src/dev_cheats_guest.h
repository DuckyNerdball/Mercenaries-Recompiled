/* Included by recomp_manual.c after dev_spawn.h / dev_factions_guest.h.
 * Runs on the retail main thread at the world-update checkpoint (via
 * recomp_dev_spawn_tick); every guest call preserves the interrupted context. */
#include "cheat_menu.h"
#include "dev_missions.h"

/* Player (index 0) toward every other faction and back, through the same retail
 * setter the Factions page uses (0x00095B50 on object 0x00323338). */
static void dev_cheat_relations(uint32_t stack, unsigned relation, char *message, size_t size)
{
    uint32_t value = recomp_float_bits(dev_relation_values[relation]);
    for (unsigned f = 1u; f < 7u; ++f) {
        uint32_t args[] = {1u, f + 1u, value};
        dev_call(stack, 0x00095B50u, 0x00323338u, 3, args);
        args[0] = f + 1u; args[1] = 1u;
        dev_call(stack, 0x00095B50u, 0x00323338u, 3, args);
    }
    snprintf(message, size, "Every faction is now %s toward you.", dev_relation_names[relation]);
    xbox_preview_log_event("dev-cheat", "relations all=%s", dev_relation_names[relation]);
}

/* Spawn a retail template_pic_* pickup ~2.5 m in front of the player using the
 * same RsMain spawn route (0x00174480) as dev vehicles/troops. */
static int dev_cheat_drop(uint32_t stack, const CheatDrop *drop, float side, char *message, size_t size)
{
    uint32_t pos = stack + 0x100, matrix = stack + 0x140, ray = stack + 0x200;
    uint32_t prop = stack + 0x400, list = stack + 0x600;
    const uint32_t calls[] = {0x8C7E0, 0x1ED340, 0x187A0, 0x12C650, 0x12BE40,
                              0x1EB010, 0x1EAFB0, 0x1EB080, 0x174480};
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); i++)
        if (!recomp_lookup(calls[i])) { snprintf(message, size, "Required retail spawn function is unavailable."); return 0; }

    uint32_t template_hash = dev_hash(drop->template_name);
    uint32_t model_args[] = {template_hash, dev_hash("geometryfile")};
    uint32_t model = dev_call(stack, 0x1ED340, 0, 2, model_args);
    if (!model || !dev_resident_model(model)) {
        dev_load_layer(stack, "template_weapons");
        model = dev_call(stack, 0x1ED340, 0, 2, model_args);
    }
    if (!model || !dev_resident_model(model)) {
        snprintf(message, size, "%s is not available in this map/chapter.", drop->name); return 0;
    }

    uint32_t player_arg[] = {0x660E4490u};
    uint32_t player = dev_call(stack, 0x8C7E0, 0, 1, player_arg);
    if (!dev_guest_address(player, 0x76C)) { snprintf(message, size, "Player is not ready."); return 0; }
    uint32_t vtable = guest_u32(player);
    if (!dev_guest_address(vtable, 0x40) || !recomp_lookup(guest_u32(vtable + 0x3C))) {
        snprintf(message, size, "Player position is unavailable."); return 0;
    }
    uint32_t position_arg[] = {pos};
    dev_call(stack, guest_u32(vtable + 0x3C), player, 1, position_arg);
    float origin[3]; memcpy(origin, guest_ptr(pos), 12);
    uint32_t camera = guest_u32(0x41410C);
    if (!dev_guest_address(camera, 0x40)) { snprintf(message, size, "Gameplay camera is unavailable."); return 0; }
    float dir[3] = {-*(float *)guest_ptr(camera + 0x30), 0.f, -*(float *)guest_ptr(camera + 0x38)};
    float length = sqrtf(dir[0] * dir[0] + dir[2] * dir[2]);
    if (!isfinite(origin[0]) || !isfinite(origin[1]) || !isfinite(origin[2]) || !isfinite(length) || length < 0.01f) {
        snprintf(message, size, "Look toward the horizon and try again."); return 0;
    }
    dir[0] /= length; dir[2] /= length;

    float hit[3];
    int placed = 0;
    /* Free Cam: drop on the ground just ahead of / below the free camera. */
    float cam_pos[3], cam_dir[3];
    if (recomp_freecam_enabled() && recomp_freecam_focus(cam_pos, cam_dir)) {
        float cl = sqrtf(cam_dir[0] * cam_dir[0] + cam_dir[2] * cam_dir[2]);
        if (isfinite(cl) && cl > 0.01f) { dir[0] = cam_dir[0] / cl; dir[2] = cam_dir[2] / cl; }
        float start[3] = {cam_pos[0] + dir[0] * 4.f - dir[2] * side, cam_pos[1] + 50.f,
                          cam_pos[2] + dir[2] * 4.f + dir[0] * side};
        float end[3] = {start[0], cam_pos[1] - 2000.f, start[2]};
        placed = dev_ray(stack, ray, start, end, hit) && isfinite(hit[1]);
    }
    for (unsigned attempt = 0; attempt < 3 && !placed; ++attempt) {
        float distance = 2.5f - attempt * 0.75f;
        /* side > 0 is to the player's right (perpendicular to the look direction). */
        float start[3] = {origin[0] + dir[0] * distance - dir[2] * side, origin[1] + 3.f,
                          origin[2] + dir[2] * distance + dir[0] * side};
        float end[3] = {start[0], origin[1] - 10.f, start[2]};
        if (!dev_ray(stack, ray, start, end, hit) || !isfinite(hit[1]) || fabsf(hit[1] - origin[1]) > 4.f) continue;
        float from[3] = {origin[0], origin[1] + 1.f, origin[2]}, to[3] = {hit[0], hit[1] + 1.f, hit[2]};
        if (dev_ray(stack, ray, from, to, NULL)) continue; /* wall between player and drop */
        placed = 1;
    }
    if (!placed) { snprintf(message, size, "No clear ground in front of you. Face open ground."); return 0; }

    float mat[16] = {dir[2], 0, -dir[0], 0, 0, 1, 0, 0, dir[0], 0, dir[2], 0,
                     hit[0], hit[1] + 0.3f, hit[2], 1};
    memcpy(guest_ptr(matrix), mat, sizeof(mat));
    static uint32_t serial;
    char name[48]; snprintf(name, sizeof(name), "recomp_cheat_%u", ++serial);
    dev_call(stack, 0x1EB010, list, 0, NULL);
    dev_property(stack, prop, list, "name", name);
    dev_property(stack, prop, list, "dieuponhibernation", "true");
    strcpy(guest_ptr(pos), name);
    uint32_t spawn_args[] = {template_hash, pos, matrix, list, 0};
    uint32_t actor = dev_call(stack, 0x174480, 0x4031B0, 5, spawn_args);
    int ok = dev_guest_address(actor, 16);
    if (ok) dev_transient_actor(actor);
    xbox_preview_log_event("dev-cheat", "drop template=%s actor=%08X ok=%d at=(%.3f,%.3f,%.3f)",
                           drop->template_name, actor, ok, hit[0], hit[1], hit[2]);
    snprintf(message, size, ok ? "%s dropped in front of you." : "The game refused to create %s.", drop->name);
    return ok;
}

/* Direct call of a retail RsCheats effect routine. None of them use the
 * RsCheats instance (ecx); all reach the player through 0x8C7E0/0x8ECE0
 * themselves. 0x15CAD0 takes the costume model hash as its one argument. */
static int dev_cheat_effect(uint32_t stack, const CheatEffect *fx, char *message, size_t size)
{
    if (!fx) { snprintf(message, size, "Unknown cheat."); return 0; }
    if (!recomp_lookup(fx->function) || !recomp_lookup(0x8C7E0u)) {
        snprintf(message, size, "Retail cheat routine %08X is unavailable.", fx->function); return 0;
    }
    uint32_t player_arg[] = {0x660E4490u};
    uint32_t player = dev_call(stack, 0x8C7E0u, 0, 1, player_arg);
    if (!dev_guest_address(player, 0x76C)) { snprintf(message, size, "Player is not ready."); return 0; }
    if (fx->has_argument) { uint32_t arg = fx->argument; dev_call(stack, fx->function, 0, 1, &arg); }
    else dev_call(stack, fx->function, 0, 0, NULL);
    if (fx->function == 0x0015C8E0u) {
        uint32_t vtable = guest_u32(player);
        int on = dev_guest_address(vtable, 0x188) && recomp_lookup(guest_u32(vtable + 0x184)) &&
                 (dev_call(stack, guest_u32(vtable + 0x184), player, 0, NULL) & 0xFFu) != 0;
        snprintf(message, size, "God mode is now %s.", on ? "ON" : "OFF");
    } else if (fx->function == 0x0015C920u) {
        snprintf(message, size, "Infinite ammo is now %s.", guest_u8(0x3BF670u) ? "ON" : "OFF");
    } else if (fx->function == 0x0015C940u) {
        /* 0x15C940 itself reads the balance from 0x8ECE0's object (+0xAC4). */
        uint32_t holder = recomp_lookup(0x8ECE0u) ? dev_call(stack, 0x8ECE0u, 0, 1, player_arg) : 0;
        if (dev_guest_address(holder, 0xAC8)) snprintf(message, size, "+$1,000,000. Balance: $%.0f", (double)guest_f32(holder + 0xAC4u));
        else snprintf(message, size, "+$1,000,000.");
    } else snprintf(message, size, "%s: done.", fx->name);
    xbox_preview_log_event("dev-cheat", "effect=%08X arg=%08X", fx->function, fx->argument);
    return 1;
}

/* Load one Deck-of-52 card layer through the retail list loader (RsMain::LoadList
 * 0x180AE0, as dev_load_layer does for catalogue layers), finishing deferred
 * loads with 0x17F490. The layer places the card NPC and its authored guards and
 * vehicles at their retail location. A card whose NPC is already in the world
 * (found by name through 0x8C7E0) is skipped so its layer is never doubled.
 * Returns 1 loaded, 0 skipped/failed (message explains). */
static unsigned char dev_cheat_card_loaded[64];
static int dev_cheat_card(uint32_t stack, const CheatCard *card, char *message, size_t size)
{
    if (!card) { snprintf(message, size, "Unknown card."); return 0; }
    int province = recomp_dev_current_province();
    if (province != card->province) {
        snprintf(message, size, "%s lives in the %s province. Travel there first (F9 > Missions / Travel).",
                 card->name, card->province ? "Northern" : "Southern");
        return 0;
    }
    if (!recomp_lookup(0x180AE0) || !recomp_lookup(0x17F490) || !recomp_lookup(0x8C7E0)) {
        snprintf(message, size, "Retail layer loader is unavailable."); return 0;
    }
    uint32_t actor_hash = dev_hash(card->actor);
    uint32_t existing = dev_call(stack, 0x8C7E0, 0, 1, &actor_hash);
    if (dev_guest_address(existing, 16)) { snprintf(message, size, "%s is already out in the world.", card->name); return 0; }
    if (dev_cheat_card_loaded[card - recomp_cheat_card_at(0)]) {
        snprintf(message, size, "%s's camp was already loaded this session. Go to their location.", card->name); return 0;
    }
    uint32_t layer = dev_hash(card->layer);
    int importing = dev_region_import_active;
    dev_region_import_active = 1;
    dev_call(stack, 0x180AE0, 0, 1, &layer);
    if (*(uint8_t *)guest_ptr(0x413FC9)) dev_call(stack, 0x17F490, 0, 0, NULL);
    dev_region_import_active = importing;
    /* Mercenaries activates placed actors spatially: the card and guards only
     * become live objects once the player is near their camp, so an absent
     * actor right after LoadList is expected, not a failure. */
    uint32_t spawned = dev_call(stack, 0x8C7E0, 0, 1, &actor_hash);
    dev_cheat_card_loaded[card - recomp_cheat_card_at(0)] = 1;
    xbox_preview_log_event("dev-cheat", "card layer=%s actor=%08X", card->layer, spawned);
    snprintf(message, size, dev_guest_address(spawned, 16) ? "%s is now out in the world with their guards."
             : "%s's camp is loaded. The card appears when you get near their authored location.", card->name);
    return 1;
}

/* Run a chunk in the master mission Lua VM (same route as the F9 Missions tab:
 * lua_gettop 0x1DC970, luaL_loadbuffer 0x1DC480, lua_pcall 0x1DD5A0,
 * lua_settop 0x1DC980). Returns 0 on success. */
static uint32_t dev_cheat_lua(uint32_t stack, const char *script, const char *chunk_name)
{
    uint32_t vm = guest_u32(0x3717BCu);
    if (!dev_guest_address(vm, 0xA0u) || !recomp_lookup(0x1DC970u) || !recomp_lookup(0x1DC980u) ||
        !recomp_lookup(0x1DC480u) || !recomp_lookup(0x1DD5A0u)) return 0xFFFFFFFFu;
    uint32_t text = stack + 0x800u, name = stack + 0xC00u;
    size_t length = strlen(script);
    if (length >= 0x3F0u) return 0xFFFFFFFEu;
    memcpy(guest_ptr(text), script, length + 1);
    strcpy(guest_ptr(name), chunk_name);
    uint32_t top = dev_call(stack, 0x1DC970u, 0, 1, &vm);
    uint32_t load[] = {vm, text, (uint32_t)length, name};
    uint32_t error = dev_call(stack, 0x1DC480u, 0, 4, load);
    if (!error) { uint32_t call[] = {vm, 0, 0, 0}; error = dev_call(stack, 0x1DD5A0u, 0, 4, call); }
    if (error) recomp_lua_pcall_checkpoint(vm, error, name);
    uint32_t restore[] = {vm, top};
    dev_call(stack, 0x1DC980u, 0, 2, restore);
    return error;
}

/* Aim point for strikes. Direction = gameplay camera back axis ([0x41410C]
 * +0x30..+0x38, negated: the same axis the vehicle spawner uses, so pitch is
 * included). Eye = render camera position ([0x4140E8+0x1C] +0x40..+0x48) when it
 * is near the player and its back axis agrees, else the player's head.
 * A single long dev_ray passes through terrain (v8 hit the world bounds at
 * x=2048 / z=2056), so march along the view in 4 m steps and use the proven
 * short vertical ray to find the ground under each sample; the first sample
 * at or below the ground (or a roof) is the target. */
/* Free Cam: aim along the free camera's own view from its position (screen
 * centre), marching up to 1.5 km so you can strike from high up. */
static int dev_cheat_freecam_aim(uint32_t stack, float *target, float *heading_deg, float *eye_out)
{
    float eye[3], f[3];
    if (!recomp_freecam_enabled() || !recomp_freecam_focus(eye, f)) return 0;
    float l = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (!isfinite(l) || l < 0.01f) return 0;
    for (int i = 0; i < 3; ++i) f[i] /= l;
    uint32_t ray = stack + 0x200;
    float hit[3], prev_t = 1.f;
    int found = 0, samples = 0;
    for (float t = 1.f; t <= 1500.f && samples < 400; prev_t = t, t += (t < 200.f ? 3.f : 6.f), ++samples) {
        float p[3] = {eye[0] + f[0] * t, eye[1] + f[1] * t, eye[2] + f[2] * t};
        float top[3] = {p[0], p[1] + 250.f, p[2]}, bottom[3] = {p[0], p[1] - 250.f, p[2]};
        if (!dev_ray(stack, ray, top, bottom, hit) || !isfinite(hit[1]) || p[1] > hit[1]) continue;
        float m = (prev_t + t) * 0.5f;
        float q[3] = {eye[0] + f[0] * m, eye[1] + f[1] * m, eye[2] + f[2] * m};
        float qt[3] = {q[0], q[1] + 250.f, q[2]}, qb[3] = {q[0], q[1] - 250.f, q[2]};
        if (dev_ray(stack, ray, qt, qb, hit) && isfinite(hit[1])) { target[0] = q[0]; target[1] = hit[1]; target[2] = q[2]; }
        else memcpy(target, p, 12);
        found = 1;
        break;
    }
    float h = sqrtf(f[0] * f[0] + f[2] * f[2]);
    if (!isfinite(h) || h < 0.01f) { f[0] = 0; f[2] = 1; h = 1; }
    if (!found) {
        target[0] = eye[0] + f[0] / h * 150.f; target[2] = eye[2] + f[2] / h * 150.f; target[1] = eye[1];
        float top[3] = {target[0], eye[1] + 400.f, target[2]}, bottom[3] = {target[0], eye[1] - 1500.f, target[2]};
        if (dev_ray(stack, ray, top, bottom, hit) && isfinite(hit[1])) target[1] = hit[1];
    }
    *heading_deg = atan2f(f[0], f[2]) * 57.29578f;
    if (eye_out) memcpy(eye_out, eye, 12);
    xbox_preview_log_event("dev-cheat", "freecam aim eye=(%.1f,%.1f,%.1f) dir=(%.2f,%.2f,%.2f) found=%d samples=%d target=(%.1f,%.1f,%.1f)",
                           eye[0], eye[1], eye[2], f[0], f[1], f[2], found, samples, target[0], target[1], target[2]);
    return 1;
}

static int dev_cheat_aim_point(uint32_t stack, const float *player, float *target, float *heading_deg)
{
    if (dev_cheat_freecam_aim(stack, target, heading_deg, NULL)) return 1;
    uint32_t ray = stack + 0x200;
    uint32_t camera = guest_u32(0x41410C);
    if (!dev_guest_address(camera, 0x40)) return 0;
    float f[3];
    for (int i = 0; i < 3; ++i) f[i] = -*(float *)guest_ptr(camera + 0x30 + 4 * i);
    float l = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (!isfinite(l) || l < 0.01f) return 0;
    for (int i = 0; i < 3; ++i) f[i] /= l;
    /* The reticle sits ~10 deg above screen centre (third-person over-the-
     * shoulder framing), so tilt the view axis up along the camera's up axis. */
    {
        float u[3];
        for (int i = 0; i < 3; ++i) u[i] = *(float *)guest_ptr(camera + 0x20 + 4 * i);
        float ul = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
        if (!isfinite(ul) || ul < 0.5f || ul > 1.5f) { u[0] = 0; u[1] = 1; u[2] = 0; ul = 1; }
        const float tilt = 0.176f; /* tan(10 deg) */
        for (int i = 0; i < 3; ++i) f[i] += u[i] / ul * tilt;
        l = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
        for (int i = 0; i < 3; ++i) f[i] /= l;
    }
    /* Fallback eye: approximate third-person camera (~5 m behind, ~3 m up), so
     * the crosshair's downward pitch doesn't land the strike at our feet. */
    float fh = sqrtf(f[0] * f[0] + f[2] * f[2]);
    if (!isfinite(fh) || fh < 0.01f) fh = 1.f;
    float eye[3] = {player[0] - f[0] / fh * 5.f, player[1] + 3.2f, player[2] - f[2] / fh * 5.f};
    int red_used = 0;
    uint32_t red = guest_u32(0x4140E8u + 0x1Cu);
    float rp[3] = {0, 0, 0}, rb[3] = {0, 0, 0};
    if (dev_guest_address(red, 0x4C)) {
        memcpy(rb, guest_ptr(red + 0x30), 12); memcpy(rp, guest_ptr(red + 0x40), 12);
        float bl = sqrtf(rb[0] * rb[0] + rb[1] * rb[1] + rb[2] * rb[2]);
        float dx = rp[0] - player[0], dy = rp[1] - player[1], dz = rp[2] - player[2];
        float agree = isfinite(bl) && bl > 0.01f ? -(rb[0] * f[0] + rb[1] * f[1] + rb[2] * f[2]) / bl : 0.f;
        if (isfinite(rp[0]) && isfinite(rp[1]) && isfinite(rp[2]) && agree > 0.95f &&
            dx * dx + dy * dy + dz * dz < 40.f * 40.f) {
            memcpy(eye, rp, 12);
            red_used = 1;
        }
    }
    float h = sqrtf(f[0] * f[0] + f[2] * f[2]);
    if (!isfinite(h) || h < 0.01f) { f[0] = 0; f[2] = 1; h = 1; }
    float along = (player[0] - eye[0]) * f[0] + (player[1] - eye[1]) * f[1] + (player[2] - eye[2]) * f[2];
    float t = (along > 0 ? along : 0) + 6.f, hit[3];
    int found = 0, samples = 0;
    float prev_t = t;
    for (; t <= 800.f && samples < 220; prev_t = t, t += 4.f, ++samples) {
        float p[3] = {eye[0] + f[0] * t, eye[1] + f[1] * t, eye[2] + f[2] * t};
        float top[3] = {p[0], p[1] + 250.f, p[2]}, bottom[3] = {p[0], p[1] - 250.f, p[2]};
        if (!dev_ray(stack, ray, top, bottom, hit) || !isfinite(hit[1])) continue;
        if (p[1] <= hit[1]) {
            /* Crossed the surface between prev_t and t: take the midpoint's ground. */
            float m = (prev_t + t) * 0.5f;
            float q[3] = {eye[0] + f[0] * m, eye[1] + f[1] * m, eye[2] + f[2] * m};
            float qt[3] = {q[0], q[1] + 250.f, q[2]}, qb[3] = {q[0], q[1] - 250.f, q[2]};
            if (dev_ray(stack, ray, qt, qb, hit) && isfinite(hit[1])) { target[0] = q[0]; target[1] = hit[1]; target[2] = q[2]; }
            else { target[0] = p[0]; target[1] = p[1]; target[2] = p[2]; }
            found = 1;
            break;
        }
    }
    if (!found) {
        /* Sky: 150 m ahead horizontally, snapped to the ground. */
        target[0] = player[0] + f[0] / h * 150.f; target[2] = player[2] + f[2] / h * 150.f; target[1] = player[1];
        float top[3] = {target[0], player[1] + 400.f, target[2]}, bottom[3] = {target[0], player[1] - 600.f, target[2]};
        if (dev_ray(stack, ray, top, bottom, hit) && isfinite(hit[1])) target[1] = hit[1];
    }
    xbox_preview_log_event("dev-cheat", "aim player=(%.1f,%.1f,%.1f) eye=(%.1f,%.1f,%.1f) red=%d redpos=(%.1f,%.1f,%.1f) dir=(%.2f,%.2f,%.2f) found=%d samples=%d target=(%.1f,%.1f,%.1f)",
                           player[0], player[1], player[2], eye[0], eye[1], eye[2], red_used, rp[0], rp[1], rp[2],
                           f[0], f[1], f[2], found, samples, target[0], target[1], target[2]);
    float hx = target[0] - player[0], hz = target[2] - player[2];
    if (hx * hx + hz * hz < 1.f) { hx = f[0]; hz = f[2]; }
    *heading_deg = atan2f(hx, hz) * 57.29578f;
    return 1;
}

static int dev_cheat_strike_templates_ready(uint32_t stack, const char *plane)
{
    uint32_t check[] = {dev_hash(plane), dev_hash("geometryfile")};
    return dev_call(stack, 0x1ED340, 0, 2, check) != 0;
}

/* Strike at the crosshair (aim point), on foot, driving or flying. */
static int dev_cheat_strike(uint32_t stack, const CheatStrike *strike, char *message, size_t size)
{
    if (!strike) { snprintf(message, size, "Unknown strike."); return 0; }
    uint32_t player_arg[] = {0x660E4490u};
    uint32_t player = dev_call(stack, 0x8C7E0, 0, 1, player_arg);
    if (!dev_guest_address(player, 0x76C)) { snprintf(message, size, "Player is not ready."); return 0; }
    uint32_t vtable = guest_u32(player), pos = stack + 0x100;
    if (!dev_guest_address(vtable, 0x40) || !recomp_lookup(guest_u32(vtable + 0x3C))) { snprintf(message, size, "Player position is unavailable."); return 0; }
    uint32_t position_arg[] = {pos};
    dev_call(stack, guest_u32(vtable + 0x3C), player, 1, position_arg);
    float origin[3]; memcpy(origin, guest_ptr(pos), 12);
    if (!isfinite(origin[0]) || !isfinite(origin[1]) || !isfinite(origin[2])) { snprintf(message, size, "Player position is invalid."); return 0; }
    float target[3], heading = 0;
    if (!dev_cheat_aim_point(stack, origin, target, &heading)) { snprintf(message, size, "Camera is unavailable; try again."); return 0; }
    /* Strike aircraft templates live in the template_weapons layer (preloaded at gameplay start). */
    if (!dev_cheat_strike_templates_ready(stack, strike->plane)) dev_load_layer(stack, "template_weapons");
    if (!dev_cheat_strike_templates_ready(stack, strike->plane)) dev_load_layer(stack, "template_vehicles"); /* killbox planes */
    if (!dev_cheat_strike_templates_ready(stack, strike->plane)) { snprintf(message, size, "%s is not available in this map.", strike->name); return 0; }
    float dx = target[0] - origin[0], dz = target[2] - origin[2];
    float distance = sqrtf(dx * dx + dz * dz);
    /* Do what a thrown support smoke grenade does (0x1C98F..0x1CA4E): spawn
     * template_ammo_AirstrikeBeaconCreatedBySmokeGrenade at the target with
     * property {airstrikename 0x5CD62A3E = hash(plane template)}, set its pose
     * (vtbl+0x30) and arm it (vtbl+0x1B0). The beacon then calls in the plane
     * and flies the real attack run. Mission_SpawnAirstrike only spawns the
     * aircraft (it circles without attacking), so it's the fallback. */
    uint32_t matrix = stack + 0x140, prop = stack + 0x400, list = stack + 0x600;
    float hr = heading * 0.0174533f, sn = sinf(hr), cs = cosf(hr);
    /* Retail support ammo (strike->beacon) carries its own settings: gunship
     * SpookyFlyTime, or an artillery AmmoProjectile (detonateUponStop) that
     * calls the Type 66 barrage when it lands, so drop that one from 1.5 m. */
    float lift = strike->beacon ? 1.5f : 0.2f;
    float mat[16] = {cs, 0, -sn, 0, 0, 1, 0, 0, sn, 0, cs, 0, target[0], target[1] + lift, target[2], 1};
    memcpy(guest_ptr(matrix), mat, sizeof(mat));
    const uint32_t beacon_template = strike->beacon ? dev_hash(strike->beacon)
                                                    : 0x075DCAB1u; /* template_ammo_AirstrikeBeaconCreatedBySmokeGrenade */
    uint32_t beacon = 0;
    for (int attempt = 0; attempt < 2 && !dev_guest_address(beacon, 16); ++attempt) {
        if (attempt) dev_load_layer(stack, "template_weapons");
        dev_call(stack, 0x1EB010, list, 0, NULL);
        if (!strike->beacon) {
            uint32_t pair[] = {0x5CD62A3Eu, dev_hash(strike->plane)};
            dev_call(stack, 0x1EA730, prop, 2, pair);
            uint32_t add[] = {prop};
            dev_call(stack, 0x1EB080, list, 1, add);
        }
        uint32_t spawn_args[] = {beacon_template, 0, matrix, list, 0};
        beacon = dev_call(stack, 0x174480, 0x4031B0, 5, spawn_args);
    }
    int ok = 0;
    uint32_t error = 0;
    if (dev_guest_address(beacon, 0x200) && dev_guest_address(guest_u32(beacon), 0x1B4)) {
        uint32_t vt = guest_u32(beacon);
        uint32_t cls = dev_call(stack, guest_u32(vt + 4), beacon, 0, NULL);
        if ((cls & guest_u32(0x30C144)) == guest_u32(0x30C140)) {
            uint32_t entity = dev_call(stack, 0x113700, beacon, 0, NULL);
            if (entity) dev_call(stack, 0x1EAA20, entity, 0, NULL);
            uint32_t pose[] = {matrix};
            dev_call(stack, guest_u32(vt + 0x30), beacon, 1, pose);
            dev_call(stack, guest_u32(vt + 0x1B0), beacon, 0, NULL);
            ok = 1;
        } else if (strike->beacon) {
            ok = 1; /* artillery shell: spawned at the target, detonates when it stops */
        }
    }
    if (!ok && !strike->beacon) {
        char script[256];
        snprintf(script, sizeof(script), "Mission_SpawnAirstrike('%s',%.3f,%.3f,%.3f,%.3f)",
                 strike->plane, target[0], target[1], target[2], heading);
        error = dev_cheat_lua(stack, script, "cheat airstrike");
    }
    xbox_preview_log_event("dev-cheat", "strike plane=%s via=%s beacon=%08X ok=%d fallback_error=%u at=(%.1f,%.1f,%.1f) dist=%.0f",
                           strike->plane, strike->beacon ? strike->beacon : "generic", beacon, ok, error, target[0], target[1], target[2], distance);
    if (ok) snprintf(message, size, "%s inbound at your crosshair (%.0f m).", strike->name, distance);
    else snprintf(message, size, error ? "The game rejected the strike (see preview log)." : "Beacon failed; spawned %s directly.", strike->name);
    return ok || error == 0;
}

/* HUD toast via the game's own Ui_PrintHudMessage (plain ASCII only). */
static void dev_cheat_hud(uint32_t stack, const char *text)
{
    char clean[120], script[160];
    size_t n = 0;
    for (const char *c = text; *c && n + 1 < sizeof(clean); ++c)
        if (*c >= 32 && *c < 127 && *c != '\'' && *c != '\\') clean[n++] = *c;
    clean[n] = 0;
    snprintf(script, sizeof(script), "Ui_PrintHudMessage('%s')", clean);
    dev_cheat_lua(stack, script, "cheat hud");
}

/* ---------------- Warzone ----------------
 * Hostile units go through the normal dev spawner (dev_wz_push in dev_spawn.h)
 * with full NK crews, so they fight the player. Placement is a grid around the
 * crosshair, facing the player. Fuel tanks are template_misc props. */
typedef struct { const char *template_name; int troop; unsigned count; } DevWzUnit;
static const DevWzUnit dev_wz_tanks[] = {
    {"template_nk_type62tank", 0, 4}, {"template_nk_type54", 0, 2}, {"template_nk_zsu-57aa", 0, 2}};
static const DevWzUnit dev_wz_assault[] = {
    {"template_nk_type54", 0, 2}, {"template_nk_bmp-1apc", 0, 2}, {"template_nk_btr60", 0, 2},
    {"template_nk_sa8sam", 0, 1}, {"template_nk_type89", 0, 1},
    {"template_nk_soldier", 1, 8}, {"template_nk_heavy", 1, 4}};
static const DevWzUnit dev_wz_infantry[] = {
    {"template_nk_soldier", 1, 10}, {"template_nk_heavy", 1, 8}, {"template_nk_elite", 1, 8},
    {"template_nk_officer", 1, 4}};
static const DevWzUnit dev_wz_china[] = {
    {"template_china_type96", 0, 2}, {"template_china_type80", 0, 2}, {"template_china_tunguska", 0, 1},
    {"template_china_apc", 0, 2}, {"template_china_soldier", 1, 8}, {"template_china_heavy", 1, 4}};

static int dev_cheat_player_pos(uint32_t stack, float *out)
{
    uint32_t player_arg[] = {0x660E4490u};
    uint32_t player = dev_call(stack, 0x8C7E0, 0, 1, player_arg);
    if (!dev_guest_address(player, 0x76C)) return 0;
    uint32_t vtable = guest_u32(player), pos = stack + 0x100;
    if (!dev_guest_address(vtable, 0x40) || !recomp_lookup(guest_u32(vtable + 0x3C))) return 0;
    uint32_t position_arg[] = {pos};
    dev_call(stack, guest_u32(vtable + 0x3C), player, 1, position_arg);
    memcpy(out, guest_ptr(pos), 12);
    return isfinite(out[0]) && isfinite(out[1]) && isfinite(out[2]);
}

static float dev_cheat_ground(uint32_t stack, float x, float y, float z)
{
    float top[3] = {x, y + 120.f, z}, bottom[3] = {x, y - 300.f, z}, hit[3];
    if (dev_ray(stack, stack + 0x200, top, bottom, hit) && isfinite(hit[1])) return hit[1];
    return y;
}

/* Queue a group: vehicles on a 4-wide grid (14 m x 16 m), infantry squads of up
 * to 8 behind them. fx/fz = unit direction from the player toward the centre. */
static unsigned dev_wz_queue_group(uint32_t stack, const DevWzUnit *units, unsigned n, const float *centre,
                                   float fx, float fz, unsigned faction, unsigned *skipped)
{
    float rx = fz, rz = -fx;
    unsigned cell = 0, squad = 0, queued = 0;
    for (unsigned u = 0; u < n; ++u) {
        int index = units[u].troop ? dev_find_troop(units[u].template_name) : dev_find_vehicle(units[u].template_name);
        if (index < 0) { ++*skipped; xbox_preview_log_event("dev-warzone", "missing template=%s", units[u].template_name); continue; }
        if (units[u].troop) {
            for (unsigned left = units[u].count; left > 0;) {
                unsigned take = left > 8 ? 8 : left;
                float lat = ((float)(squad % 3) - 1.f) * 12.f, depth = 36.f + (float)(squad / 3) * 10.f;
                float at[3] = {centre[0] + rx * lat + fx * depth, 0, centre[2] + rz * lat + fz * depth};
                at[1] = dev_cheat_ground(stack, at[0], centre[1], at[2]) + 2.f;
                unsigned request = (unsigned)index | DEV_SPAWN_TROOP | ((take - 1u) << DEV_SPAWN_COUNT_SHIFT);
                queued += (unsigned)dev_wz_push(request, at, -fx, -fz);
                left -= take; ++squad;
            }
        } else {
            for (unsigned k = 0; k < units[u].count; ++k, ++cell) {
                float lat = ((float)(cell % 4) - 1.5f) * 14.f, depth = (float)(cell / 4) * 16.f;
                float at[3] = {centre[0] + rx * lat + fx * depth, 0, centre[2] + rz * lat + fz * depth};
                at[1] = dev_cheat_ground(stack, at[0], centre[1], at[2]) + 2.f;
                unsigned request = (unsigned)index | (2u << DEV_SPAWN_CREW_SHIFT) | (faction << DEV_SPAWN_FACTION_SHIFT);
                queued += (unsigned)dev_wz_push(request, at, -fx, -fz);
            }
        }
    }
    return queued;
}

/* A template_misc prop (fuel tank) snapped to the ground. */
static int dev_cheat_prop(uint32_t stack, const char *template_name, const char *layer, float x, float y, float z, float yaw)
{
    uint32_t matrix = stack + 0x140, prop = stack + 0x400, list = stack + 0x600, pos = stack + 0x100;
    uint32_t args[] = {dev_hash(template_name), dev_hash("geometryfile")};
    uint32_t model = dev_call(stack, 0x1ED340, 0, 2, args);
    if (!model || !dev_resident_model(model)) { dev_load_layer(stack, layer); model = dev_call(stack, 0x1ED340, 0, 2, args); }
    if (!model || !dev_resident_model(model)) return 0;
    float ground = dev_cheat_ground(stack, x, y, z), s = sinf(yaw), c = cosf(yaw);
    float mat[16] = {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, x, ground + 0.05f, z, 1};
    memcpy(guest_ptr(matrix), mat, sizeof(mat));
    static uint32_t serial;
    char name[48]; snprintf(name, sizeof(name), "recomp_prop_%u", ++serial);
    dev_call(stack, 0x1EB010, list, 0, NULL);
    dev_property(stack, prop, list, "name", name);
    dev_property(stack, prop, list, "dieuponhibernation", "true");
    strcpy(guest_ptr(pos), name);
    uint32_t spawn_args[] = {dev_hash(template_name), pos, matrix, list, 0};
    uint32_t actor = dev_call(stack, 0x174480, 0x4031B0, 5, spawn_args);
    int ok = dev_guest_address(actor, 16);
    if (ok) dev_transient_actor(actor);
    return ok;
}

static unsigned dev_cheat_fuel_depot(uint32_t stack, const float *centre, float fx, float fz)
{
    float rx = fz, rz = -fx;
    unsigned made = 0;
    for (unsigned i = 0; i < 12; ++i) {
        float lat = ((float)(i % 4) - 1.5f) * 6.f, depth = 8.f + (float)(i / 4) * 6.f;
        made += (unsigned)dev_cheat_prop(stack, "template_fueltank", "template_misc",
                                         centre[0] + rx * lat + fx * depth, centre[1], centre[2] + rz * lat + fz * depth,
                                         (float)i * 0.7f);
    }
    return made;
}

static int dev_cheat_warzone(uint32_t stack, unsigned preset, char *message, size_t size)
{
    float origin[3], target[3], heading = 0;
    if (!dev_cheat_player_pos(stack, origin)) { snprintf(message, size, "Player is not ready."); return 0; }
    if (dev_cheat_freecam_aim(stack, target, &heading, origin)) {
        origin[1] = target[1]; /* Free Cam: measure the 60 m stand-off from the camera, not the mercenary */
    } else if (!dev_cheat_aim_point(stack, origin, target, &heading)) { snprintf(message, size, "Camera is unavailable."); return 0; }
    float fx = target[0] - origin[0], fz = target[2] - origin[2], l = sqrtf(fx * fx + fz * fz);
    if (l < 1.f) { fx = sinf(heading * 0.0174533f); fz = cosf(heading * 0.0174533f); l = 1.f; }
    fx /= l; fz /= l;
    if (l < 60.f) { /* keep hostile armor at least ~60 m out */
        float push = 60.f - l;
        target[0] += fx * push; target[2] += fz * push;
        target[1] = dev_cheat_ground(stack, target[0], target[1], target[2]);
    }
    unsigned queued = 0, skipped = 0, fuel = 0;
    if (preset == 0 || preset == 5) queued += dev_wz_queue_group(stack, dev_wz_tanks, 3, target, fx, fz, 3u, &skipped);
    if (preset == 1) queued += dev_wz_queue_group(stack, dev_wz_assault, 7, target, fx, fz, 3u, &skipped);
    if (preset == 2 || preset == 5) queued += dev_wz_queue_group(stack, dev_wz_infantry, 4, target, fx, fz, 3u, &skipped);
    if (preset == 3) queued += dev_wz_queue_group(stack, dev_wz_china, 6, target, fx, fz, 1u, &skipped);
    if (preset == 4 || preset == 5) {
        float depot[3] = {target[0] - fz * 30.f, target[1], target[2] + fx * 30.f}; /* off to the side of the armor */
        if (preset == 4) memcpy(depot, target, 12);
        fuel = dev_cheat_fuel_depot(stack, depot, fx, fz);
    }
    dev_wz_made = dev_wz_tried = 0;
    xbox_preview_log_event("dev-warzone", "preset=%u queued=%u skipped=%u fuel=%u at=(%.1f,%.1f,%.1f)",
                           preset, queued, skipped, fuel, target[0], target[1], target[2]);
    snprintf(message, size, "Warzone: %u groups/vehicles queued, %u fuel tanks placed at your crosshair%s.",
             queued, fuel, skipped ? " (some templates missing in this map)" : "");
    return queued > 0 || fuel > 0;
}

/* Ambush waves: every ~45 s a random NK group spawns 120-160 m away and attacks. */
static void dev_cheat_ambush_tick(uint32_t stack, ULONGLONG now)
{
    static ULONGLONG next;
    if (!recomp_cheat_ambush()) { next = 0; return; }
    if (!next) { next = now + 8000; return; }
    if (now < next || dev_wz_pending()) return;
    next = now + 45000;
    float player[3];
    if (!dev_cheat_player_pos(stack, player)) return;
    float bearing = (float)(rand() % 360) * 0.0174533f, dist = 120.f + (float)(rand() % 40);
    float centre[3] = {player[0] + sinf(bearing) * dist, player[1], player[2] + cosf(bearing) * dist};
    centre[1] = dev_cheat_ground(stack, centre[0], player[1], centre[2]);
    float fx = (centre[0] - player[0]) / dist, fz = (centre[2] - player[2]) / dist;
    static const DevWzUnit wave_a[] = {{"template_nk_type54", 0, 1}, {"template_nk_soldier", 1, 6}};
    static const DevWzUnit wave_b[] = {{"template_nk_btr60", 0, 2}, {"template_nk_heavy", 1, 6}};
    static const DevWzUnit wave_c[] = {{"template_nk_zsu-57aa", 0, 1}, {"template_nk_jeepmgun", 0, 2}, {"template_nk_soldier", 1, 8}};
    static const DevWzUnit wave_d[] = {{"template_nk_type62tank", 0, 2}, {"template_nk_elite", 1, 6}};
    unsigned skipped = 0, pick = (unsigned)rand() % 4u, queued = 0;
    if (pick == 0) queued = dev_wz_queue_group(stack, wave_a, 2, centre, fx, fz, 3u, &skipped);
    else if (pick == 1) queued = dev_wz_queue_group(stack, wave_b, 2, centre, fx, fz, 3u, &skipped);
    else if (pick == 2) queued = dev_wz_queue_group(stack, wave_c, 3, centre, fx, fz, 3u, &skipped);
    else queued = dev_wz_queue_group(stack, wave_d, 2, centre, fx, fz, 3u, &skipped);
    static const char *dirs[] = {"north", "north-east", "east", "south-east", "south", "south-west", "west", "north-west"};
    float deg = atan2f(fx, fz) * 57.29578f; if (deg < 0) deg += 360.f;
    char text[96]; snprintf(text, sizeof(text), "Ambush incoming from the %s!", dirs[((int)((deg + 22.5f) / 45.f)) % 8]);
    dev_wz_made = dev_wz_tried = 0;
    xbox_preview_log_event("dev-warzone", "ambush pick=%u queued=%u skipped=%u at=(%.1f,%.1f,%.1f)", pick, queued, skipped, centre[0], centre[1], centre[2]);
    dev_cheat_hud(stack, text);
}


/* Periodic toggles. Money: retail SetMoney 0x57F50 on 0x8ECE0's object (+0xAC4).
 * Density: traffic manager 0x3DC970 holds [+0x28] zones of 0x294 bytes at +0x2C;
 * zone +4 = traffic density, +0x27C = civilian density (what the Traffic_SetZone*
 * Lua natives write). Scripts keep re-authoring these, so the multiplier tracks
 * the latest authored base value and re-applies on change. */
static void dev_cheat_maintain(uint32_t stack)
{
    static ULONGLONG next;
    static float base_traffic[64], base_civ[64], written_traffic[64], written_civ[64];
    static int applied_density = 1;
    ULONGLONG now = GetTickCount64();
    if (now < next) return;
    next = now + 500;
    dev_cheat_ambush_tick(stack, now);
    if (dev_wz_tried && !dev_wz_pending()) {
        char text[96];
        snprintf(text, sizeof(text), "Spawned %u of %u hostile groups/vehicles", dev_wz_made, dev_wz_tried);
        xbox_preview_log_event("dev-warzone", "done made=%u tried=%u", dev_wz_made, dev_wz_tried);
        dev_cheat_hud(stack, text);
        dev_wz_made = dev_wz_tried = 0;
    }
    /* Preload strike aircraft once per gameplay session so quick strikes fire
     * without a load hitch. Re-check every ~10 s (layers can unload on travel). */
    static unsigned preload_tick;
    if ((preload_tick++ % 20u) == 0u && !dev_cheat_strike_templates_ready(stack, recomp_cheat_strike_at(1)->plane)) {
        dev_load_layer(stack, "template_weapons");
        xbox_preview_log_event("dev-cheat", "preloaded template_weapons ready=%d",
                               dev_cheat_strike_templates_ready(stack, recomp_cheat_strike_at(1)->plane));
    }
    if (recomp_cheat_infinite_money() && recomp_lookup(0x8ECE0u) && recomp_lookup(0x57F50u)) {
        uint32_t name = 0x660E4490u;
        uint32_t holder = dev_call(stack, 0x8ECE0u, 0, 1, &name);
        if (dev_guest_address(holder, 0xAC8) && guest_f32(holder + 0xAC4u) < 9000000.f) {
            uint32_t amount = recomp_float_bits(9999999.f);
            dev_call(stack, 0x57F50u, holder, 1, &amount);
        }
    }
    int density = recomp_cheat_density();
    uint32_t count = guest_u32(0x3DC970u + 0x28u);
    if (count > 64u) count = 64u;
    if (density == 1 && applied_density == 1) return;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t zone = 0x3DC970u + 0x2Cu + i * 0x294u;
        float *traffic = (float *)guest_ptr(zone + 4u), *civ = (float *)guest_ptr(zone + 0x27Cu);
        if (!isfinite(*traffic) || !isfinite(*civ)) continue;
        if (*traffic != written_traffic[i]) base_traffic[i] = *traffic;
        if (*civ != written_civ[i]) base_civ[i] = *civ;
        *traffic = written_traffic[i] = base_traffic[i] * (float)density;
        *civ = written_civ[i] = base_civ[i] * (float)density;
    }
    if (density != applied_density)
        xbox_preview_log_event("dev-cheat", "density x%d zones=%u", density, count);
    applied_density = density;
}

/* Quick-strike input, run on every guest pad poll for port 0 (game thread).
 *   Hold LB + click R3      fire the selected strike at the crosshair
 *   Hold LB + D-pad L / R   choose the strike (shown on the HUD)
 *   F7                      fire (keyboard)
 * While LB is held it is withheld from the game, so the chord never switches
 * grenades / ejects passengers; a plain LB tap is replayed for 100 ms on release.
 * Buttons used by a chord stay swallowed until they are released. */
static void dev_cheat_filter_pad(XBOX_INPUT_STATE *st, ULONGLONG now)
{
    static int lb_down, lb_used, f7_prev;
    static WORD prev, swallow;
    static ULONGLONG replay_until;
    const WORD chord = XBOX_GAMEPAD_RIGHT_THUMB | XBOX_GAMEPAD_DPAD_LEFT | XBOX_GAMEPAD_DPAD_RIGHT;
    int gameplay = recomp_dev_menu_allowed() && g_xbox_mem_offset &&
                   guest_u32(0x413F6C) == 0x4249D707u && guest_u32(0x413F68) == 0xC2CBD863u;
    WORD buttons = st->Gamepad.wButtons, pressed = (WORD)(buttons & ~prev);
    prev = buttons;
    int f7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    if (f7 && !f7_prev && gameplay) {
        DWORD pid = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &pid);
        if (pid == GetCurrentProcessId()) recomp_cheat_fire_quick_strike();
    }
    f7_prev = f7;
    if (!gameplay) { lb_down = lb_used = 0; swallow = 0; replay_until = 0; return; }
    if (st->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] > XBOX_ANALOG_BUTTON_THRESHOLD) {
        if (!lb_down) { lb_down = 1; lb_used = 0; }
        st->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] = 0;
        if (pressed & XBOX_GAMEPAD_RIGHT_THUMB) { recomp_cheat_fire_quick_strike(); lb_used = 1; }
        if (pressed & XBOX_GAMEPAD_DPAD_LEFT) { recomp_cheat_cycle_quick_strike(-1); lb_used = 1; }
        if (pressed & XBOX_GAMEPAD_DPAD_RIGHT) { recomp_cheat_cycle_quick_strike(1); lb_used = 1; }
        swallow |= (WORD)(buttons & chord);
    } else if (lb_down) {
        lb_down = 0;
        if (!lb_used) replay_until = now + 100;
    }
    swallow &= buttons;
    st->Gamepad.wButtons &= (WORD)~swallow;
    if (now < replay_until) st->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] = 255;
}

void recomp_dev_cheat_tick(void)
{
    static int executing;
    if (executing) return;
    if (recomp_dev_menu_allowed() &&
        g_xbox_mem_offset && guest_u32(0x413F6C) == 0x4249D707u && guest_u32(0x413F68) == 0xC2CBD863u &&
        g_esp >= 0x20000u && g_esp < 0x4000000u) {
        recomp_saved_guest_cpu_context saved_maintain;
        recomp_save_guest_cpu_context(&saved_maintain);
        executing = 1;
        dev_cheat_maintain((g_esp - 0x4000u) & ~15u);
        executing = 0;
        recomp_restore_guest_cpu_context(&saved_maintain);
    }
    if (recomp_cheat_take_quick_strike()) {
        int index = recomp_cheat_quick_strike();
        if (recomp_dev_menu_allowed() && g_xbox_mem_offset && guest_u32(0x413F6C) == 0x4249D707u &&
            guest_u32(0x413F68) == 0xC2CBD863u && g_esp >= 0x20000u && g_esp < 0x4000000u) {
            char quick[192];
            recomp_saved_guest_cpu_context saved_quick;
            recomp_save_guest_cpu_context(&saved_quick);
            executing = 1;
            uint32_t qs = (g_esp - 0x4000u) & ~15u;
            const CheatStrike *strike = recomp_cheat_strike_at((unsigned)index);
            int fired = dev_cheat_strike(qs, strike, quick, sizeof(quick));
            dev_cheat_hud(qs, fired && strike ? strike->name : quick);
            executing = 0;
            recomp_restore_guest_cpu_context(&saved_quick);
        }
    }
    {
        /* Announce the newly selected strike after LB + D-pad left/right. */
        static LONG shown;
        LONG announce = recomp_cheat_quick_strike_announce();
        if (announce != shown) {
            shown = announce;
            int pending = recomp_cheat_quick_strike();
            if (recomp_dev_menu_allowed() && g_xbox_mem_offset && guest_u32(0x413F6C) == 0x4249D707u &&
                guest_u32(0x413F68) == 0xC2CBD863u && g_esp >= 0x20000u && g_esp < 0x4000000u) {
                const CheatStrike *strike = recomp_cheat_strike_at((unsigned)pending);
                char text[120];
                snprintf(text, sizeof(text), "Quick strike: %s", strike ? strike->name : "?");
                recomp_saved_guest_cpu_context saved_hud;
                recomp_save_guest_cpu_context(&saved_hud);
                executing = 1;
                dev_cheat_hud((g_esp - 0x4000u) & ~15u, text);
                executing = 0;
                recomp_restore_guest_cpu_context(&saved_hud);
            }
        }
    }
    unsigned action = recomp_cheat_take_action();
    if (!action) return;
    char message[192];
    if (!recomp_dev_menu_allowed() || !g_xbox_mem_offset ||
        guest_u32(0x413F6C) != 0x4249D707u || guest_u32(0x413F68) != 0xC2CBD863u ||
        g_esp < 0x20000u || g_esp >= 0x4000000u) {
        recomp_cheat_action_result(0, "Enter normal gameplay (not a menu or cutscene) first.");
        return;
    }
    executing = 1;
    recomp_saved_guest_cpu_context saved;
    recomp_save_guest_cpu_context(&saved);
    uint32_t stack = (g_esp - 0x4000u) & ~15u;
    int ok = 0;
    unsigned kind = action & 0xFFFF0000u, index = action & 0xFFFFu;
    if (kind == CHEAT_ACTION_DROP) {
        const CheatDrop *drop = recomp_cheat_drop_at(index);
        if (drop) ok = dev_cheat_drop(stack, drop, 0.f, message, sizeof(message));
        else snprintf(message, sizeof(message), "Unknown drop.");
    } else if (kind == CHEAT_ACTION_DROP_ALL) {
        /* Weapon pickups only, fanned ~1.1 m apart so they don't stack. */
        unsigned total = 0, made = 0, slot = 0;
        char last[192] = "";
        for (unsigned i = 0; i < recomp_cheat_drop_count(); ++i) total += (unsigned)recomp_cheat_drop_at(i)->weapon;
        for (unsigned i = 0; i < recomp_cheat_drop_count(); ++i) {
            const CheatDrop *drop = recomp_cheat_drop_at(i);
            if (!drop->weapon) continue;
            float side = ((float)slot++ - (float)(total - 1u) * 0.5f) * 1.1f;
            made += (unsigned)dev_cheat_drop(stack, drop, side, last, sizeof(last));
        }
        ok = made > 0;
        if (made == total) snprintf(message, sizeof(message), "Dropped all %u weapons in a row in front of you.", total);
        else snprintf(message, sizeof(message), "Dropped %u of %u weapons. Last problem: %s", made, total, last);
    } else if (kind == CHEAT_ACTION_CARD && index == CHEAT_CARD_ALL) {
        int province = recomp_dev_current_province();
        unsigned loaded = 0, already = 0, failed = 0;
        char last[192] = "";
        for (unsigned i = 0; i < recomp_cheat_card_count(); ++i) {
            const CheatCard *card = recomp_cheat_card_at(i);
            if (card->province != province) continue;
            uint32_t actor_hash = dev_hash(card->actor);
            if (dev_guest_address(dev_call(stack, 0x8C7E0, 0, 1, &actor_hash), 16) ||
                dev_cheat_card_loaded[i]) { ++already; continue; }
            if (dev_cheat_card(stack, card, last, sizeof(last))) ++loaded; else ++failed;
        }
        ok = loaded > 0;
        if (province < 0) snprintf(message, sizeof(message), "Enter the Southern or Northern province first.");
        else snprintf(message, sizeof(message), "%s province: %u card camps loaded, %u already loaded/out, %u failed. Cards appear as you approach their locations.%s%s",
                      province ? "Northern" : "Southern", loaded, already, failed, failed ? " Last: " : "", failed ? last : "");
    } else if (kind == CHEAT_ACTION_CARD) {
        ok = dev_cheat_card(stack, recomp_cheat_card_at(index), message, sizeof(message));
    } else if (kind == CHEAT_ACTION_STRIKE) {
        ok = dev_cheat_strike(stack, recomp_cheat_strike_at(index), message, sizeof(message));
    } else if (kind == CHEAT_ACTION_WARZONE) {
        ok = dev_cheat_warzone(stack, index, message, sizeof(message));
    } else if (kind == CHEAT_ACTION_EFFECT) {
        ok = dev_cheat_effect(stack, recomp_cheat_effect_at(index), message, sizeof(message));
    } else if (kind == CHEAT_ACTION_FRIENDLY || kind == CHEAT_ACTION_HOSTILE) {
        if (recomp_lookup(0x00095B50u)) {
            dev_cheat_relations(stack, kind == CHEAT_ACTION_FRIENDLY ? 3u : 0u, message, sizeof(message));
            ok = 1;
        } else snprintf(message, sizeof(message), "Retail faction setter is unavailable.");
    } else snprintf(message, sizeof(message), "Unknown cheat request.");
    recomp_restore_guest_cpu_context(&saved);
    executing = 0;
    recomp_cheat_action_result(ok, message);
}
