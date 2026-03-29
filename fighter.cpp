#include "fighter.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ----------------------------------------
// Dados dos personagens
// ----------------------------------------
static const CharData CHAR_DATA[CHAR_COUNT] = {
    // RYOKU - Equilibrado (como Ryu)
    {
        .name             = "Ryoku",
        .max_hp           = 1000,
        .walk_speed       = 4.5f,
        .dash_speed       = 9.0f,
        .jump_force       = -12.0f,
        .weight           = 1.0f,
        .attack_dmg       = {60, 90, 50, 80, 70, 60, 120, 150, 300, 70, 45, 55},
        .energy_cost_super = 200,
        .sprite_path      = "romfs:/sprites/ryoku.t3x",
        .portrait_path    = "romfs:/portraits/ryoku.t3x",
        .color_primary    = 0xFF4444FF,
        .color_secondary  = 0xFFFFFFFF,
    },
    // SEIRA - Ágil, boa em combos
    {
        .name             = "Seira",
        .max_hp           = 850,
        .walk_speed       = 5.5f,
        .dash_speed       = 11.0f,
        .jump_force       = -13.5f,
        .weight           = 0.85f,
        .attack_dmg       = {45, 70, 55, 65, 60, 55, 100, 130, 270, 60, 40, 45},
        .energy_cost_super = 180,
        .sprite_path      = "romfs:/sprites/seira.t3x",
        .portrait_path    = "romfs:/portraits/seira.t3x",
        .color_primary    = 0xFFFF44FF,
        .color_secondary  = 0xFF44FFFF,
    },
    // GORDO - Lento, fortíssimo
    {
        .name             = "Gordo",
        .max_hp           = 1300,
        .walk_speed       = 2.8f,
        .dash_speed       = 6.0f,
        .jump_force       = -10.0f,
        .weight           = 1.4f,
        .attack_dmg       = {80, 130, 60, 110, 90, 80, 180, 200, 400, 90, 65, 75},
        .energy_cost_super = 220,
        .sprite_path      = "romfs:/sprites/gordo.t3x",
        .portrait_path    = "romfs:/portraits/gordo.t3x",
        .color_primary    = 0xFF22BB22,
        .color_secondary  = 0xFF884400,
    },
    // NINJA - Mobilidade e ilusões
    {
        .name             = "Ninja",
        .max_hp           = 800,
        .walk_speed       = 6.0f,
        .dash_speed       = 13.0f,
        .jump_force       = -14.0f,
        .weight           = 0.75f,
        .attack_dmg       = {50, 75, 45, 65, 55, 50, 110, 140, 260, 55, 35, 45},
        .energy_cost_super = 160,
        .sprite_path      = "romfs:/sprites/ninja.t3x",
        .portrait_path    = "romfs:/portraits/ninja.t3x",
        .color_primary    = 0xFF222222,
        .color_secondary  = 0xFF8844FF,
    },
};

const CharData* char_get_data(CharacterID id) {
    if (id >= CHAR_COUNT) return &CHAR_DATA[0];
    return &CHAR_DATA[id];
}

// ----------------------------------------
// Criação e destruição
// ----------------------------------------
Fighter* fighter_create(CharacterID char_id, int player_num, float start_x) {
    Fighter* f = (Fighter*)calloc(1, sizeof(Fighter));
    if (!f) return NULL;
    
    f->char_id    = char_id;
    f->player_num = player_num;
    f->data       = char_get_data(char_id);
    
    fighter_reset(f, start_x);
    return f;
}

void fighter_destroy(Fighter* f) {
    if (f) free(f);
}

void fighter_reset(Fighter* f, float start_x) {
    f->x         = start_x;
    f->y         = STAGE_FLOOR;
    f->vel_x     = 0.0f;
    f->vel_y     = 0.0f;
    f->on_ground = true;
    f->facing    = (f->player_num == 0) ? DIR_RIGHT : DIR_LEFT;
    
    f->hp         = f->data->max_hp;
    f->max_hp     = f->data->max_hp;
    f->energy     = 0;
    f->max_energy = MAX_ENERGY;
    
    f->state          = STATE_IDLE;
    f->current_attack = ATTACK_NONE;
    f->anim_frame     = 0;
    f->frame_timer    = 0;
    f->state_timer    = 0;
    
    f->hitstun_timer    = 0;
    f->blockstun_timer  = 0;
    f->invincible_timer = 0;
    f->knockdown_timer  = 0;
    f->is_blocking      = false;
    
    f->combo_count   = 0;
    f->last_hit_timer = 0;
    
    memset(f->input_buffer, 0, sizeof(f->input_buffer));
    f->input_buf_head  = 0;
    f->input_buf_timer = 0;
}

// ----------------------------------------
// Física e movimento
// ----------------------------------------
static void apply_physics(Fighter* f) {
    // Gravidade
    if (!f->on_ground) {
        f->vel_y += GRAVITY;
    }
    
    // Atrito horizontal no chão
    if (f->on_ground && f->state != STATE_WALKING_F && f->state != STATE_WALKING_B) {
        f->vel_x *= 0.7f;
        if (fabsf(f->vel_x) < 0.1f) f->vel_x = 0.0f;
    }
    
    // Aplica velocidade
    f->x += f->vel_x;
    f->y += f->vel_y;
    
    // Chão
    if (f->y >= STAGE_FLOOR) {
        f->y         = STAGE_FLOOR;
        f->vel_y     = 0.0f;
        f->on_ground = true;
        
        if (f->state == STATE_JUMPING) {
            f->state     = STATE_IDLE;
            f->anim_frame = 0;
            f->frame_timer = 0;
        }
        if (f->state == STATE_KNOCKDOWN && f->vel_y <= 0) {
            f->knockdown_timer = 60; // 1 segundo no chão
        }
    } else {
        f->on_ground = false;
    }
    
    // Paredes do stage
    if (f->x < STAGE_LEFT)  { f->x = STAGE_LEFT;  f->vel_x = 0; }
    if (f->x > STAGE_RIGHT) { f->x = STAGE_RIGHT; f->vel_x = 0; }
}

void fighter_move(Fighter* f, float dx) {
    if (f->state == STATE_ATTACKING || f->state == STATE_HIT ||
        f->state == STATE_KNOCKDOWN  || f->state == STATE_DEAD ||
        f->hitstun_timer > 0 || f->blockstun_timer > 0) return;
    
    f->vel_x = dx * f->data->walk_speed;
    if (!f->on_ground) f->vel_x = dx * f->data->walk_speed * 0.8f;
    
    if (f->on_ground) {
        f->state = (dx != 0) ?
            ((dx * f->facing > 0) ? STATE_WALKING_F : STATE_WALKING_B) :
            STATE_IDLE;
    }
}

void fighter_jump(Fighter* f) {
    if (!f->on_ground) return;
    if (f->state == STATE_ATTACKING || f->hitstun_timer > 0 || f->blockstun_timer > 0) return;
    
    f->vel_y     = f->data->jump_force;
    f->on_ground = false;
    f->state     = STATE_JUMPING;
    f->anim_frame = 0;
    f->frame_timer = 0;
}

void fighter_dash(Fighter* f, Direction dir) {
    if (f->state == STATE_ATTACKING || f->state == STATE_HIT ||
        f->hitstun_timer > 0) return;
    
    f->vel_x     = dir * f->data->dash_speed;
    f->state     = (dir == f->facing) ? STATE_DASH_F : STATE_DASH_B;
    f->state_timer = DASH_DURATION;
    f->invincible_timer = (dir != f->facing) ? 8 : 0; // Back dash: invencível
}

void fighter_crouch(Fighter* f, bool crouching) {
    if (!f->on_ground) return;
    if (f->state == STATE_ATTACKING || f->hitstun_timer > 0) return;
    f->state = crouching ? STATE_CROUCHING : STATE_IDLE;
}

// ----------------------------------------
// Sistema de ataques
// ----------------------------------------

// Define hitboxes por tipo de ataque e personagem
static Hitbox get_attack_hitbox(const Fighter* f, AttackType atk) {
    Hitbox hb = {0};
    hb.active = true;
    
    float dir = (float)f->facing;
    float base_x = f->x + dir * 20.0f;
    
    int dmg_idx = (int)atk - 1;
    if (dmg_idx < 0 || dmg_idx >= 12) dmg_idx = 0;
    hb.damage = f->data->attack_dmg[dmg_idx];
    
    switch (atk) {
        case ATTACK_PUNCH_L:
            hb.x = base_x; hb.y = f->y - 50; hb.w = 40; hb.h = 20;
            hb.knockback_x = 2.5f * dir; hb.knockback_y = -1.0f;
            hb.hitstun = 12; hb.blockstun = 6;
            break;
        case ATTACK_PUNCH_H:
            hb.x = base_x; hb.y = f->y - 55; hb.w = 50; hb.h = 25;
            hb.knockback_x = 5.0f * dir; hb.knockback_y = -2.0f;
            hb.hitstun = 20; hb.blockstun = 10;
            break;
        case ATTACK_KICK_L:
            hb.x = base_x; hb.y = f->y - 20; hb.w = 55; hb.h = 20;
            hb.knockback_x = 3.0f * dir; hb.knockback_y = -0.5f;
            hb.hitstun = 14; hb.blockstun = 8; hb.low = true;
            break;
        case ATTACK_KICK_H:
            hb.x = base_x; hb.y = f->y - 60; hb.w = 60; hb.h = 30;
            hb.knockback_x = 7.0f * dir; hb.knockback_y = -3.0f;
            hb.hitstun = 22; hb.blockstun = 12; hb.launches = true;
            break;
        case ATTACK_AIR_KICK:
            hb.x = base_x; hb.y = f->y - 30; hb.w = 50; hb.h = 35;
            hb.knockback_x = 4.0f * dir; hb.knockback_y = 3.0f;
            hb.hitstun = 18; hb.blockstun = 8; hb.overhead = true;
            break;
        case ATTACK_SPECIAL_1:
            hb.x = base_x; hb.y = f->y - 55; hb.w = 80; hb.h = 30;
            hb.knockback_x = 8.0f * dir; hb.knockback_y = -2.0f;
            hb.hitstun = 25; hb.blockstun = 14;
            break;
        case ATTACK_SUPER:
            hb.x = f->x + dir * 10; hb.y = f->y - 70; hb.w = 120; hb.h = 80;
            hb.knockback_x = 15.0f * dir; hb.knockback_y = -6.0f;
            hb.hitstun = 40; hb.blockstun = 20; hb.launches = true;
            break;
        case ATTACK_CROUCH_KICK:
            hb.x = base_x; hb.y = f->y - 10; hb.w = 60; hb.h = 15;
            hb.knockback_x = 4.0f * dir; hb.knockback_y = 0.5f;
            hb.hitstun = 16; hb.blockstun = 8; hb.low = true;
            break;
        default:
            hb.x = base_x; hb.y = f->y - 40; hb.w = 40; hb.h = 20;
            hb.knockback_x = 3.0f * dir; hb.knockback_y = -1.0f;
            hb.hitstun = 12; hb.blockstun = 6;
            break;
    }
    return hb;
}

// Duração de cada ataque em frames
static int get_attack_duration(AttackType atk) {
    switch (atk) {
        case ATTACK_PUNCH_L:    return 18;
        case ATTACK_PUNCH_H:    return 28;
        case ATTACK_KICK_L:     return 22;
        case ATTACK_KICK_H:     return 35;
        case ATTACK_AIR_PUNCH:  return 20;
        case ATTACK_AIR_KICK:   return 25;
        case ATTACK_SPECIAL_1:  return 40;
        case ATTACK_SPECIAL_2:  return 45;
        case ATTACK_SUPER:      return 60;
        case ATTACK_GRAB:       return 30;
        case ATTACK_CROUCH_KICK: return 24;
        case ATTACK_CROUCH_PUNCH: return 16;
        default: return 20;
    }
}

void fighter_attack(Fighter* f, AttackType atk) {
    if (f->state == STATE_ATTACKING || f->state == STATE_HIT ||
        f->state == STATE_DEAD || f->hitstun_timer > 0 ||
        f->blockstun_timer > 0) return;
    
    // Super requer energia suficiente
    if (atk == ATTACK_SUPER && f->energy < f->data->energy_cost_super) return;
    if (atk == ATTACK_SUPER) f->energy -= f->data->energy_cost_super;
    
    // Não pode atacar agachado com ataques aéreos
    if (f->state == STATE_CROUCHING && 
        (atk == ATTACK_AIR_PUNCH || atk == ATTACK_AIR_KICK)) return;
    
    f->current_attack = atk;
    f->state          = STATE_ATTACKING;
    f->state_timer    = get_attack_duration(atk);
    f->anim_frame     = 0;
    f->frame_timer    = 0;
}

void fighter_cancel_attack(Fighter* f) {
    f->current_attack = ATTACK_NONE;
    f->state          = f->on_ground ? STATE_IDLE : STATE_JUMPING;
    f->state_timer    = 0;
}

// ----------------------------------------
// Receber dano
// ----------------------------------------
bool fighter_apply_hit(Fighter* f, const Hitbox* hb, Direction hit_dir) {
    if (fighter_is_invincible(f)) return false;
    if (f->state == STATE_DEAD) return false;
    
    // Verifica bloqueio
    bool blocked = false;
    if (f->is_blocking) {
        // Verificar direção do bloqueio
        bool facing_hit = (f->facing != hit_dir);
        bool can_block  = facing_hit;
        
        if (hb->low && f->state != STATE_CROUCHING) can_block = false;
        if (hb->overhead && f->state == STATE_CROUCHING) can_block = false;
        
        if (can_block) {
            blocked = true;
            f->blockstun_timer = hb->blockstun;
            f->hp -= hb->damage / 5; // Chip damage
            // Ganha um pouco de energia ao bloquear
            f->energy = (f->energy + 5 < f->max_energy) ? f->energy + 5 : f->max_energy;
        }
    }
    
    if (!blocked) {
        // Aplica dano
        int real_damage = (int)(hb->damage / f->data->weight);
        f->hp -= real_damage;
        if (f->hp < 0) f->hp = 0;
        
        // Knockback
        f->vel_x = hb->knockback_x;
        f->vel_y = hb->knockback_y;
        
        // Hitstun
        f->hitstun_timer    = hb->hitstun;
        f->state            = hb->launches ? STATE_KNOCKDOWN : STATE_HIT;
        f->current_attack   = ATTACK_NONE;
        f->is_blocking      = false;
        
        // Ganha energia ao receber dano
        f->energy = (f->energy + 15 < f->max_energy) ? f->energy + 15 : f->max_energy;
        
        if (f->hp <= 0) f->state = STATE_DEAD;
    }
    
    return true; // Golpe conectou
}

void fighter_set_blocking(Fighter* f, bool b) {
    if (f->state == STATE_ATTACKING || f->state == STATE_DEAD) {
        f->is_blocking = false;
        return;
    }
    f->is_blocking = b;
    if (b && f->state != STATE_CROUCHING) f->state = STATE_BLOCKING;
    else if (!b && f->state == STATE_BLOCKING) f->state = STATE_IDLE;
}

// ----------------------------------------
// Update principal
// ----------------------------------------
void fighter_update(Fighter* f) {
    // Temporizadores
    if (f->hitstun_timer > 0)    f->hitstun_timer--;
    if (f->blockstun_timer > 0)  f->blockstun_timer--;
    if (f->invincible_timer > 0) f->invincible_timer--;
    if (f->last_hit_timer > 0)   f->last_hit_timer--;
    if (f->last_hit_timer == 0)  f->combo_count = 0;
    
    // Reset estado de hit após hitstun
    if (f->hitstun_timer == 0 && f->state == STATE_HIT && f->hp > 0) {
        f->state = f->on_ground ? STATE_IDLE : STATE_JUMPING;
    }
    
    // Ataque: decrementa timer
    if (f->state == STATE_ATTACKING) {
        f->state_timer--;
        if (f->state_timer <= 0) {
            f->current_attack = ATTACK_NONE;
            f->state = f->on_ground ? STATE_IDLE : STATE_JUMPING;
        }
    }
    
    // Dash: decrementa timer
    if ((f->state == STATE_DASH_F || f->state == STATE_DASH_B) && f->state_timer > 0) {
        f->state_timer--;
        if (f->state_timer <= 0) {
            f->state = STATE_IDLE;
            f->vel_x *= 0.3f;
        }
    }
    
    // Knockdown
    if (f->state == STATE_KNOCKDOWN) {
        if (f->on_ground) {
            f->knockdown_timer--;
            if (f->knockdown_timer <= 0 && f->hp > 0) {
                f->state = STATE_GETUP;
                f->invincible_timer = 30;
                f->state_timer = 30;
            }
        }
    }
    if (f->state == STATE_GETUP) {
        f->state_timer--;
        if (f->state_timer <= 0) f->state = STATE_IDLE;
    }
    
    // Vira para o oponente automaticamente (fora de ataques)
    if (f->opponent && f->state == STATE_IDLE) {
        fighter_face_opponent(f);
    }
    
    // Regeneração lenta de energia em idle
    if (f->state == STATE_IDLE && f->energy < f->max_energy) {
        // 1 ponto a cada 2 frames
        if ((f->frame_timer % 2) == 0) f->energy++;
    }
    
    // Frame timer para animação
    f->frame_timer++;
    
    // Física
    apply_physics(f);
}

// ----------------------------------------
// Queries
// ----------------------------------------
bool fighter_is_attacking(const Fighter* f) {
    return f->state == STATE_ATTACKING && f->current_attack != ATTACK_NONE;
}

bool fighter_is_invincible(const Fighter* f) {
    return f->invincible_timer > 0;
}

bool fighter_is_dead(const Fighter* f) {
    return f->state == STATE_DEAD || f->hp <= 0;
}

bool fighter_is_airborne(const Fighter* f) {
    return !f->on_ground;
}

Hitbox* fighter_get_active_hitbox(Fighter* f) {
    static Hitbox hb;
    if (!fighter_is_attacking(f)) { hb.active = false; return &hb; }
    
    // Hitbox ativa nos frames do meio do ataque
    int total = get_attack_duration(f->current_attack);
    int elapsed = total - f->state_timer;
    int active_start = total / 4;
    int active_end   = total / 2;
    
    if (elapsed >= active_start && elapsed <= active_end) {
        hb = get_attack_hitbox(f, f->current_attack);
        hb.active = true;
    } else {
        hb.active = false;
    }
    return &hb;
}

Hurtbox* fighter_get_hurtbox(Fighter* f) {
    static Hurtbox hb;
    hb.active = (f->state != STATE_DEAD);
    hb.x = f->x - 20;
    hb.w = 40;
    
    if (f->state == STATE_CROUCHING) {
        hb.y = f->y - 40;
        hb.h = 40;
    } else if (f->state == STATE_JUMPING || !f->on_ground) {
        hb.y = f->y - 65;
        hb.h = 55;
    } else {
        hb.y = f->y - 70;
        hb.h = 70;
    }
    return &hb;
}

void fighter_face_opponent(Fighter* f) {
    if (!f->opponent) return;
    f->facing = (f->opponent->x > f->x) ? DIR_RIGHT : DIR_LEFT;
}

float fighter_distance_to(const Fighter* f, const Fighter* other) {
    return fabsf(f->x - other->x);
}
