#include "game.h"
#include <stdlib.h>
#include <string.h>

// ----------------------------------------
// Criação / destruição
// ----------------------------------------
Game* game_init(void) {
    Game* g = (Game*)calloc(1, sizeof(Game));
    if (!g) return NULL;
    g->winner       = -1;
    g->match_winner = -1;
    return g;
}

void game_destroy(Game* g) {
    if (!g) return;
    if (g->fighters[0]) fighter_destroy(g->fighters[0]);
    if (g->fighters[1]) fighter_destroy(g->fighters[1]);
    free(g);
}

void game_reset(Game* g) {
    if (g->fighters[0]) { fighter_destroy(g->fighters[0]); g->fighters[0] = NULL; }
    if (g->fighters[1]) { fighter_destroy(g->fighters[1]); g->fighters[1] = NULL; }
    g->round_info.round_num    = 1;
    g->round_info.p1_rounds_won = 0;
    g->round_info.p2_rounds_won = 0;
    g->winner       = -1;
    g->match_winner = -1;
    g->round_over   = false;
    g->game_over    = false;
    g->effect_count = 0;
    g->frame_count  = 0;
}

// ----------------------------------------
// Iniciar luta
// ----------------------------------------
void game_start_fight(Game* g, int p1_char, int p2_char) {
    // Limpa fighters anteriores
    if (g->fighters[0]) fighter_destroy(g->fighters[0]);
    if (g->fighters[1]) fighter_destroy(g->fighters[1]);
    
    g->fighters[0] = fighter_create((CharacterID)p1_char, 0, 110.0f);
    g->fighters[1] = fighter_create((CharacterID)p2_char, 1, 290.0f);
    
    // Liga referências mútuas
    g->fighters[0]->opponent = g->fighters[1];
    g->fighters[1]->opponent = g->fighters[0];
    
    g->round_info.round_num  = 1;
    g->round_info.max_rounds = MAX_ROUNDS;
    g->round_info.time_left  = ROUND_TIME;
    g->time_ticks  = ROUND_TIME * TICKS_PER_SEC;
    g->winner      = -1;
    g->round_over  = false;
    g->game_over   = false;
    g->effect_count = 0;
    g->frame_count  = 0;
    g->stage_id    = 0; // TODO: seleção de stage
}

// ----------------------------------------
// Colisão de hitbox vs hurtbox
// ----------------------------------------
static bool rects_overlap(float ax, float ay, float aw, float ah,
                           float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx &&
           ay < by + bh && ay + ah > by;
}

static void check_hit(Game* g, Fighter* attacker, Fighter* defender) {
    if (!fighter_is_attacking(attacker)) return;
    if (fighter_is_dead(defender)) return;
    
    Hitbox*  hb = fighter_get_active_hitbox(attacker);
    Hurtbox* hurt = fighter_get_hurtbox(defender);
    
    if (!hb->active || !hurt->active) return;
    
    if (rects_overlap(hb->x, hb->y, hb->w, hb->h,
                      hurt->x, hurt->y, hurt->w, hurt->h)) {
        Direction hit_dir = (attacker->x < defender->x) ? DIR_RIGHT : DIR_LEFT;
        bool hit = fighter_apply_hit(defender, hb, hit_dir);
        if (hit) {
            // Efeito visual
            float fx = (attacker->x + defender->x) * 0.5f;
            float fy = hurt->y + hurt->h * 0.3f;
            game_spawn_effect(g, defender->is_blocking ? FX_BLOCK_SPARK : FX_HIT_SPARK, fx, fy);
            
            // Combo counter
            attacker->combo_count++;
            attacker->last_hit_timer = 90;
            
            // Ganha energia ao acertar
            attacker->energy = (attacker->energy + 10 < attacker->max_energy) ?
                               attacker->energy + 10 : attacker->max_energy;
        }
    }
}

// ----------------------------------------
// Colisão de empurrão entre fighters
// ----------------------------------------
static void resolve_push(Fighter* a, Fighter* b) {
    float dist = fabsf(a->x - b->x);
    float min_dist = 45.0f;
    if (dist < min_dist && dist > 1.0f) {
        float overlap = (min_dist - dist) * 0.5f;
        if (a->x < b->x) {
            a->x -= overlap;
            b->x += overlap;
        } else {
            a->x += overlap;
            b->x -= overlap;
        }
    }
}

// ----------------------------------------
// Processar input
// ----------------------------------------
static void process_input(Fighter* f, const InputState* in) {
    if (fighter_is_dead(f)) return;
    
    // Super flash: nenhum input (exceto o super que causou)
    // (tratado no game_update)
    
    // Bloqueio
    bool wants_block = in->block ||
        (f->facing == DIR_RIGHT && in->move_left) ||
        (f->facing == DIR_LEFT  && in->move_right);
    fighter_set_blocking(f, wants_block && !in->punch_l && !in->punch_h &&
                                         !in->kick_l && !in->kick_h);
    
    // Movimento
    if (!in->block) {
        float dx = 0;
        if (in->move_left)  dx -= 1.0f;
        if (in->move_right) dx += 1.0f;
        if (dx != 0.0f) fighter_move(f, dx);
        else if (f->state == STATE_WALKING_F || f->state == STATE_WALKING_B) {
            fighter_move(f, 0);
        }
    }
    
    // Agachar
    if (in->crouch && f->on_ground) fighter_crouch(f, true);
    else if (!in->crouch && f->state == STATE_CROUCHING) fighter_crouch(f, false);
    
    // Pulo
    if (in->jump) fighter_jump(f);
    
    // Ataques - prioridade: super > especial > forte > leve
    if (in->super_move) {
        fighter_attack(f, ATTACK_SUPER);
    } else if (in->special) {
        fighter_attack(f, ATTACK_SPECIAL_1);
    } else if (in->punch_h) {
        fighter_attack(f, f->on_ground ? ATTACK_PUNCH_H : ATTACK_AIR_PUNCH);
    } else if (in->kick_h) {
        fighter_attack(f, f->on_ground ?
            (f->state == STATE_CROUCHING ? ATTACK_CROUCH_KICK : ATTACK_KICK_H) :
            ATTACK_AIR_KICK);
    } else if (in->punch_l) {
        fighter_attack(f, f->state == STATE_CROUCHING ? ATTACK_CROUCH_PUNCH : ATTACK_PUNCH_L);
    } else if (in->kick_l) {
        fighter_attack(f, ATTACK_KICK_L);
    }
}

// ----------------------------------------
// Update de efeitos
// ----------------------------------------
static void update_effects(Game* g) {
    for (int i = 0; i < g->effect_count; i++) {
        g->effects[i].timer--;
        if (g->effects[i].timer <= 0) {
            // Remove efeito: troca com o último
            g->effects[i] = g->effects[--g->effect_count];
            i--;
        }
    }
}

// ----------------------------------------
// Verificar fim de round
// ----------------------------------------
static void check_round_end(Game* g) {
    bool p1_dead = fighter_is_dead(g->fighters[0]);
    bool p2_dead = fighter_is_dead(g->fighters[1]);
    bool timeout = g->time_ticks <= 0;
    
    if (!p1_dead && !p2_dead && !timeout) return;
    
    // Determina vencedor
    if (p1_dead && p2_dead) {
        // Empate: quem tem mais HP ganha
        if (g->fighters[0]->hp > g->fighters[1]->hp)      g->winner = 0;
        else if (g->fighters[1]->hp > g->fighters[0]->hp) g->winner = 1;
        else g->winner = -1; // empate real
    } else if (p1_dead) {
        g->winner = 1;
    } else if (p2_dead) {
        g->winner = 0;
    } else {
        // Timeout: quem tem mais HP
        if (g->fighters[0]->hp >= g->fighters[1]->hp) g->winner = 0;
        else g->winner = 1;
    }
    
    // Contabiliza round
    if (g->winner == 0) g->round_info.p1_rounds_won++;
    else if (g->winner == 1) g->round_info.p2_rounds_won++;
    
    g->round_over = true;
    
    // Verifica fim de jogo
    int max_wins = (MAX_ROUNDS / 2) + 1; // melhor de 3 = 2 vitórias
    if (g->round_info.p1_rounds_won >= max_wins) {
        g->match_winner = 0;
        g->game_over = true;
    } else if (g->round_info.p2_rounds_won >= max_wins) {
        g->match_winner = 1;
        g->game_over = true;
    }
    
    game_spawn_effect(g, FX_KO_FLASH, 200.0f, 120.0f);
}

// ----------------------------------------
// Update principal
// ----------------------------------------
GameResult game_update(Game* g, const InputState* p1_in, const InputState* p2_in) {
    if (!g->fighters[0] || !g->fighters[1]) return RESULT_ONGOING;
    if (g->round_over) {
        return g->game_over ? RESULT_GAME_OVER : RESULT_ROUND_END;
    }
    
    // Super flash: congela o jogo por alguns frames
    if (g->super_flash) {
        g->super_flash_timer--;
        if (g->super_flash_timer <= 0) g->super_flash = false;
        update_effects(g);
        return RESULT_ONGOING;
    }
    
    // Processar input
    process_input(g->fighters[0], p1_in);
    process_input(g->fighters[1], p2_in);
    
    // Atualizar fighters
    fighter_update(g->fighters[0]);
    fighter_update(g->fighters[1]);
    
    // Resolução de empurrão
    resolve_push(g->fighters[0], g->fighters[1]);
    
    // Detecção de hits (bidirecional)
    check_hit(g, g->fighters[0], g->fighters[1]);
    check_hit(g, g->fighters[1], g->fighters[0]);
    
    // Timer do round
    g->time_ticks--;
    g->round_info.time_left = g->time_ticks / TICKS_PER_SEC;
    
    // Efeitos
    update_effects(g);
    
    g->frame_count++;
    
    // Verificar fim de round
    check_round_end(g);
    
    if (g->round_over) {
        return g->game_over ? RESULT_GAME_OVER : RESULT_ROUND_END;
    }
    
    return RESULT_ONGOING;
}

// ----------------------------------------
// Próximo round
// ----------------------------------------
void game_next_round(Game* g) {
    g->round_info.round_num++;
    g->winner     = -1;
    g->round_over = false;
    g->time_ticks = ROUND_TIME * TICKS_PER_SEC;
    g->round_info.time_left = ROUND_TIME;
    g->effect_count = 0;
    
    // Reset posições e vida dos fighters
    fighter_reset(g->fighters[0], 110.0f);
    fighter_reset(g->fighters[1], 290.0f);
}

// ----------------------------------------
// Efeitos
// ----------------------------------------
void game_spawn_effect(Game* g, EffectType type, float x, float y) {
    if (g->effect_count >= MAX_EFFECTS) return;
    
    Effect* e = &g->effects[g->effect_count++];
    e->type  = type;
    e->x     = x;
    e->y     = y;
    e->scale = 1.0f;
    
    switch (type) {
        case FX_HIT_SPARK:
            e->duration = 12; e->color = 0xFFFFFF00; break;
        case FX_BLOCK_SPARK:
            e->duration = 10; e->color = 0xFF4488FF; break;
        case FX_KO_FLASH:
            e->duration = 45; e->color = 0xFFFFFFFF; e->scale = 5.0f; break;
        case FX_SUPER_FLASH:
            e->duration = 30; e->color = 0xFFFFDD00; e->scale = 3.0f; break;
        case FX_DUST:
            e->duration = 20; e->color = 0xFFBBBBBB; break;
        default:
            e->duration = 15; e->color = 0xFFFFFFFF; break;
    }
    e->timer = e->duration;
}

// ----------------------------------------
// Getters
// ----------------------------------------
Fighter* game_get_fighter(Game* g, int idx) {
    if (idx < 0 || idx > 1) return NULL;
    return g->fighters[idx];
}

int game_get_stage(const Game* g) { return g->stage_id; }

Effect* game_get_effects(Game* g) { return g->effects; }

int game_get_effect_count(const Game* g) { return g->effect_count; }

const RoundInfo* game_get_round_info(const Game* g) { return &g->round_info; }

int game_get_winner(const Game* g) { return g->winner; }

int game_get_match_winner(const Game* g) { return g->match_winner; }
