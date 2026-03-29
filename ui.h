// ui.h
#pragma once
#include <3ds.h>
#include "renderer.h"
#include "game.h"
#include "fighter.h"

// ----------------------------------------
// Retornos de menus
// ----------------------------------------
#define CHARSELECT_NONE      0
#define CHARSELECT_CONFIRMED 1
#define CHARSELECT_BACK      2

#define PAUSE_NONE   0
#define PAUSE_RESUME 1
#define PAUSE_QUIT   2

typedef struct UI UI;

#ifdef __cplusplus
extern "C" {
#endif

UI*  ui_init(void);
void ui_destroy(UI* ui);

// Menu principal
void ui_update_menu(UI* ui, u32 kDown);
void ui_draw_menu(UI* ui, Renderer* r);

// Seleção de personagens
int  ui_update_char_select(UI* ui, u32 kDown);
void ui_draw_char_select(UI* ui, Renderer* r);
void ui_draw_char_select_bottom(UI* ui, Renderer* r);
int  ui_get_selected_char(UI* ui, int player);

// HUD durante luta
void ui_draw_hud_bottom(UI* ui, Renderer* r,
                         Fighter* p1, Fighter* p2,
                         const RoundInfo* ri);

// Pause
int  ui_update_pause(UI* ui, u32 kDown);
void ui_draw_pause(UI* ui, Renderer* r);

// Fim de round / jogo
void ui_draw_round_end(UI* ui, Renderer* r, int winner);
void ui_draw_game_over(UI* ui, Renderer* r, int match_winner);

// Hints de controles na tela inferior
void ui_draw_controls_hint(UI* ui, Renderer* r, int state);

#ifdef __cplusplus
}
#endif
