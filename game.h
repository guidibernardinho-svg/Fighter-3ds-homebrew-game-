#pragma once
#include "fighter.h"

// ----------------------------------------
// Configurações da partida
// ----------------------------------------
#define MAX_ROUNDS      3
#define ROUND_TIME      99   // segundos
#define TICKS_PER_SEC   60
#define MAX_EFFECTS     64

// ----------------------------------------
// Resultados de update
// ----------------------------------------
typedef enum {
    RESULT_ONGOING = 0,
    RESULT_ROUND_END,
    RESULT_GAME_OVER
} GameResult;

// ----------------------------------------
// Informações de round
// ----------------------------------------
typedef struct {
    int  round_num;
    int  max_rounds;
    int  time_left;      // segundos
    int  p1_rounds_won;
    int  p2_rounds_won;
} RoundInfo;

// ----------------------------------------
// Efeito visual
// ----------------------------------------
typedef enum {
    FX_NONE = 0,
    FX_HIT_SPARK,
    FX_BLOCK_SPARK,
    FX_KO_FLASH,
    FX_SUPER_FLASH,
    FX_DUST,
    FX_BLOOD
} EffectType;

typedef struct {
    EffectType type;
    float      x, y;
    int        timer;
    int        duration;
    u32        color;
    float      scale;
} Effect;

// ----------------------------------------
// Estrutura principal do jogo
// ----------------------------------------
typedef struct {
    Fighter*  fighters[2];
    int       stage_id;
    
    RoundInfo round_info;
    int       time_ticks;      // ticks restantes no round
    
    int       winner;          // -1 nenhum, 0 ou 1
    int       match_winner;
    bool      round_over;
    bool      game_over;
    
    Effect    effects[MAX_EFFECTS];
    int       effect_count;
    
    int       frame_count;     // total de frames desde início
    bool      super_flash;     // pausa durante super
    int       super_flash_timer;
} Game;

// ----------------------------------------
// Input de um jogador neste frame
// ----------------------------------------
typedef struct {
    bool move_left;
    bool move_right;
    bool crouch;
    bool jump;
    bool punch_l;
    bool punch_h;
    bool kick_l;
    bool kick_h;
    bool special;
    bool super_move;
    bool block;
} InputState;

// ----------------------------------------
// API
// ----------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

Game*      game_init(void);
void       game_destroy(Game* g);
void       game_reset(Game* g);

void       game_start_fight(Game* g, int p1_char, int p2_char);
GameResult game_update(Game* g, const InputState* p1_in, const InputState* p2_in);
void       game_next_round(Game* g);

// Getters para o renderer e UI
Fighter*        game_get_fighter(Game* g, int idx);
int             game_get_stage(const Game* g);
Effect*         game_get_effects(Game* g);
int             game_get_effect_count(const Game* g);
const RoundInfo* game_get_round_info(const Game* g);
int             game_get_winner(const Game* g);
int             game_get_match_winner(const Game* g);

// Efeitos
void game_spawn_effect(Game* g, EffectType type, float x, float y);

#ifdef __cplusplus
}
#endif
