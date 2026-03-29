#pragma once
#include <3ds.h>

// ----------------------------------------
// Constantes de Fighter
// ----------------------------------------
#define MAX_HP            1000
#define MAX_ENERGY        200
#define GRAVITY           0.5f
#define JUMP_FORCE       -12.0f
#define MOVE_SPEED        4.5f
#define DASH_SPEED        9.0f
#define DASH_DURATION     12
#define KNOCKBACK_BASE    5.0f
#define STAGE_FLOOR       180.0f
#define STAGE_LEFT        30.0f
#define STAGE_RIGHT       370.0f

// ----------------------------------------
// Enums
// ----------------------------------------
typedef enum {
    STATE_IDLE = 0,
    STATE_WALKING_F,
    STATE_WALKING_B,
    STATE_JUMPING,
    STATE_CROUCHING,
    STATE_ATTACKING,
    STATE_BLOCKING,
    STATE_HIT,
    STATE_KNOCKDOWN,
    STATE_GETUP,
    STATE_DASH_F,
    STATE_DASH_B,
    STATE_SPECIAL,
    STATE_SUPER,
    STATE_DEAD
} FighterState;

typedef enum {
    ATTACK_NONE = 0,
    // Socos
    ATTACK_PUNCH_L,     // Soco leve (Y)
    ATTACK_PUNCH_H,     // Soco forte (X)
    // Chutes  
    ATTACK_KICK_L,      // Chute leve (B)
    ATTACK_KICK_H,      // Chute forte (A)
    // Aéreos
    ATTACK_AIR_PUNCH,
    ATTACK_AIR_KICK,
    // Especiais
    ATTACK_SPECIAL_1,   // L + X
    ATTACK_SPECIAL_2,   // L + A
    ATTACK_SUPER,       // L + R + X (gasta energia)
    // Agarrão
    ATTACK_GRAB,
    // Crouch
    ATTACK_CROUCH_KICK,
    ATTACK_CROUCH_PUNCH
} AttackType;

typedef enum {
    CHAR_RYOKU = 0,  // Lutador equilibrado
    CHAR_SEIRA,      // Lutadora ágil / combo
    CHAR_GORDO,      // Lento mas poderoso
    CHAR_NINJA,      // Furtivo / teleporte
    CHAR_COUNT
} CharacterID;

typedef enum {
    DIR_LEFT = -1,
    DIR_RIGHT = 1
} Direction;

// ----------------------------------------
// Hitbox / Hurtbox
// ----------------------------------------
typedef struct {
    float x, y, w, h;
    bool  active;
    int   damage;
    float knockback_x;
    float knockback_y;
    int   hitstun;      // frames de hitstun
    int   blockstun;    // frames de blockstun
    bool  launches;     // arremessa o oponente
    bool  low;          // só bloqueia agachado
    bool  overhead;     // só bloqueia em pé
} Hitbox;

typedef struct {
    float x, y, w, h;
    bool  active;
} Hurtbox;

// ----------------------------------------
// Frame de animação
// ----------------------------------------
typedef struct {
    int   sprite_x, sprite_y;   // posição no spritesheet
    int   sprite_w, sprite_h;
    int   duration;              // frames que dura
    Hitbox  hitbox;
    Hurtbox hurtbox;
    float   root_motion_x;       // deslocamento neste frame
    bool    cancel_window;       // permite cancelar ataque
} AnimFrame;

// ----------------------------------------
// Animação
// ----------------------------------------
#define MAX_ANIM_FRAMES 32

typedef struct {
    AnimFrame frames[MAX_ANIM_FRAMES];
    int       frame_count;
    bool      loops;
    int       chain_to;   // AnimID ao terminar (-1 = IDLE)
} Animation;

// ----------------------------------------
// Dados de personagem (constantes)
// ----------------------------------------
typedef struct {
    const char* name;
    int         max_hp;
    float       walk_speed;
    float       dash_speed;
    float       jump_force;
    float       weight;         // afeta knockback recebido
    int         attack_dmg[12]; // dano base de cada ataque
    int         energy_cost_super;
    const char* sprite_path;
    const char* portrait_path;
    u32         color_primary;
    u32         color_secondary;
} CharData;

// ----------------------------------------
// Estado dinâmico do fighter
// ----------------------------------------
typedef struct Fighter {
    // Identidade
    CharacterID  char_id;
    int          player_num;   // 0 ou 1
    Direction    facing;
    
    // Física
    float   x, y;
    float   vel_x, vel_y;
    bool    on_ground;
    
    // Vida e energia
    int     hp;
    int     max_hp;
    int     energy;
    int     max_energy;
    
    // Estado e animação
    FighterState state;
    AttackType   current_attack;
    int          anim_frame;
    int          frame_timer;
    int          state_timer;
    
    // Stun e invincibilidade
    int     hitstun_timer;
    int     blockstun_timer;
    int     invincible_timer;
    int     knockdown_timer;
    bool    is_blocking;
    
    // Combo
    int     combo_count;
    int     last_hit_timer;
    
    // Input buffer (para movimentos especiais)
    u32     input_buffer[16];
    int     input_buf_head;
    int     input_buf_timer;
    
    // Referência ao oponente
    struct Fighter* opponent;
    
    // Ponteiro para dados constantes do personagem
    const CharData* data;
} Fighter;

// ----------------------------------------
// Funções de Fighter
// ----------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

Fighter* fighter_create(CharacterID char_id, int player_num, float start_x);
void     fighter_destroy(Fighter* f);
void     fighter_reset(Fighter* f, float start_x);
void     fighter_update(Fighter* f);

// Movimentação
void     fighter_move(Fighter* f, float dx);
void     fighter_jump(Fighter* f);
void     fighter_dash(Fighter* f, Direction dir);
void     fighter_crouch(Fighter* f, bool crouching);

// Ataque
void     fighter_attack(Fighter* f, AttackType attack);
bool     fighter_try_special(Fighter* f, u32* input_buf, int buf_size);
void     fighter_cancel_attack(Fighter* f);

// Receber dano
bool     fighter_apply_hit(Fighter* f, const Hitbox* hb, Direction hit_dir);
void     fighter_block(Fighter* f, bool blocking);
void     fighter_set_blocking(Fighter* f, bool b);

// Queries
bool     fighter_is_attacking(const Fighter* f);
bool     fighter_is_invincible(const Fighter* f);
bool     fighter_is_dead(const Fighter* f);
bool     fighter_is_airborne(const Fighter* f);
Hitbox*  fighter_get_active_hitbox(Fighter* f);
Hurtbox* fighter_get_hurtbox(Fighter* f);
void     fighter_face_opponent(Fighter* f);
float    fighter_distance_to(const Fighter* f, const Fighter* other);

// Dados de personagens
const CharData* char_get_data(CharacterID id);

#ifdef __cplusplus
}
#endif
