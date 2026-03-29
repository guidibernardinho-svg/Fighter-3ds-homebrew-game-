#pragma once
#include <3ds.h>
#include "game.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Mapeamento de botões do 3DS:
 *
 * Jogador 1 (teclado esquerdo + botões):
 *   DPAD_LEFT / DPAD_RIGHT -> mover
 *   DPAD_DOWN              -> agachar
 *   DPAD_UP                -> pular
 *   Y -> Soco leve
 *   X -> Soco forte
 *   B -> Chute leve
 *   A -> Chute forte
 *   L -> Segurar = bloqueio
 *   L + X -> Especial 1
 *   L + R + X -> Super
 *
 * Jogador 2 (botões direitos em modo dual - requer segundo 3DS ou gamepad):
 *   Igual ao P1 mas com controle remoto via IR / local 2P
 *   Para testes: simples IA ou segundo conjunto de teclas
 */

InputState input_get_p1(u32 kDown, u32 kHeld);
InputState input_get_p2(u32 kDown, u32 kHeld);

// IA simples para o jogador 2 (CPU)
InputState input_ai_update(const struct Fighter* ai_fighter, const struct Fighter* opponent, int difficulty);

#ifdef __cplusplus
}
#endif

// ============================================================
// Implementação inline (pode ser separada em input.cpp)
// ============================================================
#ifdef INPUT_IMPLEMENTATION
#include "input.h"
#include "fighter.h"
#include <stdlib.h>
#include <math.h>

InputState input_get_p1(u32 kDown, u32 kHeld) {
    InputState in = {0};
    
    in.move_left  = (kHeld & KEY_DLEFT)  != 0;
    in.move_right = (kHeld & KEY_DRIGHT) != 0;
    in.crouch     = (kHeld & KEY_DDOWN)  != 0;
    in.jump       = (kDown & KEY_DUP)    != 0;
    
    in.punch_l    = (kDown & KEY_Y) != 0;
    in.punch_h    = (kDown & KEY_X) != 0;
    in.kick_l     = (kDown & KEY_B) != 0;
    in.kick_h     = (kDown & KEY_A) != 0;
    
    in.block      = (kHeld & KEY_L) != 0 && !in.punch_l && !in.punch_h &&
                    !in.kick_l && !in.kick_h;
    
    // Especial: L + X
    in.special    = (kHeld & KEY_L) && (kDown & KEY_X) != 0;
    
    // Super: L + R + X
    in.super_move = (kHeld & KEY_L) && (kHeld & KEY_R) && (kDown & KEY_X);
    
    return in;
}

InputState input_get_p2(u32 kDown, u32 kHeld) {
    // Em modo single-player retorna estado vazio (CPU usa input_ai_update)
    InputState in = {0};
    return in;
}

// ----------------------------------------
// IA básica
// ----------------------------------------
static int ai_timer = 0;
static int ai_action = 0;

InputState input_ai_update(const Fighter* ai, const Fighter* opponent, int difficulty) {
    InputState in = {0};
    if (!ai || !opponent) return in;
    if (fighter_is_dead(ai) || fighter_is_dead(opponent)) return in;
    
    float dist = fighter_distance_to(ai, opponent);
    bool  close = dist < 80.0f;
    bool  mid   = dist < 150.0f;
    
    ai_timer--;
    if (ai_timer <= 0) {
        // Escolhe uma ação com base na dificuldade
        int r = rand() % 100;
        ai_timer = 20 + rand() % (60 - difficulty * 10);
        
        if (fighter_is_dead(ai) || fighter_is_dead(opponent)) ai_action = 0;
        else if (close) {
            if (r < 10 * difficulty)      ai_action = 1; // soco leve
            else if (r < 20 * difficulty) ai_action = 2; // soco forte
            else if (r < 30 * difficulty) ai_action = 3; // chute leve
            else if (r < 40 * difficulty) ai_action = 4; // chute forte
            else if (r < 45)              ai_action = 5; // pulo
            else if (r < 55)              ai_action = 6; // bloqueio
            else                          ai_action = 0;
        } else if (mid) {
            if (r < 30) ai_action = 7; // avança
            else if (r < 40 * difficulty) ai_action = 8; // especial
            else ai_action = 7;
        } else {
            ai_action = 7; // avança
        }
    }
    
    // Sempre se move na direção certa (se não em ação)
    bool should_advance = !close;
    float dir_to_opp = (opponent->x > ai->x) ? 1.0f : -1.0f;
    float ai_facing  = (float)ai->facing;
    
    switch (ai_action) {
        case 0: break; // idle
        case 1: in.punch_l = true; break;
        case 2: in.punch_h = true; break;
        case 3: in.kick_l  = true; break;
        case 4: in.kick_h  = true; break;
        case 5: in.jump    = true; break;
        case 6: in.block   = true; break;
        case 7: // avançar
            in.move_left  = (dir_to_opp < 0);
            in.move_right = (dir_to_opp > 0);
            break;
        case 8: in.special = true; break;
    }
    
    // Evita paredes
    if (ai->x <= STAGE_LEFT + 10)  in.move_right = true;
    if (ai->x >= STAGE_RIGHT - 10) in.move_left  = true;
    
    // Reação a ataques: bloqueia com chance
    if (fighter_is_attacking(opponent) && (rand() % 10) < difficulty) {
        in.block = true;
        in.punch_l = in.punch_h = in.kick_l = in.kick_h = false;
    }
    
    return in;
}
#endif // INPUT_IMPLEMENTATION
