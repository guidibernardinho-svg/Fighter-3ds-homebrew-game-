/**
 * 3DS Fighter - Jogo de Luta para Nintendo 3DS
 * Desenvolvido com libctru e citro3d
 * Compatível com Homebrew Launcher e CIA
 */

#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fighter.h"
#include "game.h"
#include "input.h"
#include "renderer.h"
#include "audio.h"
#include "ui.h"

// Dimensões da tela
#define SCREEN_WIDTH_TOP  400
#define SCREEN_HEIGHT_TOP 240
#define SCREEN_WIDTH_BOT  320
#define SCREEN_HEIGHT_BOT 240

// Estados do jogo
typedef enum {
    STATE_MENU = 0,
    STATE_CHARACTER_SELECT,
    STATE_FIGHT,
    STATE_ROUND_END,
    STATE_GAME_OVER,
    STATE_VICTORY,
    STATE_PAUSE
} GameState;

// Variáveis globais
static GameState currentState = STATE_MENU;
static Game* game = NULL;
static Renderer* renderer = NULL;
static AudioManager* audio = NULL;
static UI* ui = NULL;

// Configurações
#define MAX_FPS 60
#define TICKS_PER_FRAME (SYSCLOCK_ARM11 / MAX_FPS)

// ----------------------------------------
// Inicialização
// ----------------------------------------
static bool initSystems() {
    // Inicializa serviços do 3DS
    gfxInitDefault();
    gfxSet3D(true); // Habilita efeito 3D
    
    // Citro2D para renderização 2D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    // ROM FS para assets
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        // Fallback: continua sem romfs
    }
    
    // Inicializa subsistemas do jogo
    audio = audio_init();
    renderer = renderer_init(SCREEN_WIDTH_TOP, SCREEN_HEIGHT_TOP);
    ui = ui_init();
    game = game_init();
    
    return (game && renderer && ui);
}

static void cleanupSystems() {
    if (game)     game_destroy(game);
    if (renderer) renderer_destroy(renderer);
    if (audio)    audio_destroy(audio);
    if (ui)       ui_destroy(ui);
    
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    romfsExit();
}

// ----------------------------------------
// Loop de atualização por estado
// ----------------------------------------
static void updateMenu(u32 kDown) {
    ui_update_menu(ui, kDown);
    if (kDown & KEY_A) {
        currentState = STATE_CHARACTER_SELECT;
        audio_play_sfx(audio, SFX_CONFIRM);
    }
}

static void updateCharSelect(u32 kDown) {
    int result = ui_update_char_select(ui, kDown);
    if (result == CHARSELECT_CONFIRMED) {
        // Pega personagens selecionados e inicia luta
        int p1char = ui_get_selected_char(ui, 0);
        int p2char = ui_get_selected_char(ui, 1);
        game_start_fight(game, p1char, p2char);
        currentState = STATE_FIGHT;
        audio_play_bgm(audio, BGM_FIGHT);
    } else if (result == CHARSELECT_BACK) {
        currentState = STATE_MENU;
    }
}

static void updateFight(u32 kDown, u32 kHeld) {
    if (kDown & KEY_START) {
        currentState = STATE_PAUSE;
        return;
    }
    
    // Processa input dos jogadores
    InputState p1_input = input_get_p1(kDown, kHeld);
    InputState p2_input = input_get_p2(kDown, kHeld);
    
    // Atualiza lógica da luta
    GameResult result = game_update(game, &p1_input, &p2_input);
    
    if (result == RESULT_ROUND_END) {
        currentState = STATE_ROUND_END;
        audio_play_sfx(audio, SFX_KO);
    } else if (result == RESULT_GAME_OVER) {
        currentState = STATE_GAME_OVER;
        audio_play_bgm(audio, BGM_MENU);
    }
}

static void updatePause(u32 kDown) {
    int result = ui_update_pause(ui, kDown);
    if (result == PAUSE_RESUME) {
        currentState = STATE_FIGHT;
    } else if (result == PAUSE_QUIT) {
        game_reset(game);
        currentState = STATE_MENU;
        audio_play_bgm(audio, BGM_MENU);
    }
}

static void updateRoundEnd(u32 kDown) {
    if (kDown & KEY_A) {
        game_next_round(game);
        currentState = STATE_FIGHT;
    }
}

static void updateGameOver(u32 kDown) {
    if (kDown & KEY_A) {
        game_reset(game);
        currentState = STATE_MENU;
        audio_play_bgm(audio, BGM_MENU);
    }
}

// ----------------------------------------
// Loop de renderização por estado
// ----------------------------------------
static void renderTopScreen(C3D_RenderTarget* top) {
    C2D_TargetClear(top, C2D_Color32(0x00, 0x00, 0x00, 0xFF));
    C2D_SceneBegin(top);
    
    switch (currentState) {
        case STATE_MENU:
            ui_draw_menu(ui, renderer);
            break;
        case STATE_CHARACTER_SELECT:
            ui_draw_char_select(ui, renderer);
            break;
        case STATE_FIGHT:
        case STATE_PAUSE:
        case STATE_ROUND_END:
            renderer_draw_background(renderer, game_get_stage(game));
            renderer_draw_fighters(renderer,
                game_get_fighter(game, 0),
                game_get_fighter(game, 1));
            renderer_draw_effects(renderer, game_get_effects(game));
            if (currentState == STATE_PAUSE) {
                ui_draw_pause(ui, renderer);
            } else if (currentState == STATE_ROUND_END) {
                ui_draw_round_end(ui, renderer, game_get_winner(game));
            }
            break;
        case STATE_GAME_OVER:
            ui_draw_game_over(ui, renderer, game_get_match_winner(game));
            break;
        default: break;
    }
}

static void renderBottomScreen(C3D_RenderTarget* bot) {
    C2D_TargetClear(bot, C2D_Color32(0x10, 0x10, 0x18, 0xFF));
    C2D_SceneBegin(bot);
    
    switch (currentState) {
        case STATE_FIGHT:
        case STATE_ROUND_END:
            // HUD na tela inferior: vida, energia, rounds
            ui_draw_hud_bottom(ui, renderer,
                game_get_fighter(game, 0),
                game_get_fighter(game, 1),
                game_get_round_info(game));
            break;
        case STATE_CHARACTER_SELECT:
            ui_draw_char_select_bottom(ui, renderer);
            break;
        default:
            ui_draw_controls_hint(ui, renderer, currentState);
            break;
    }
}

// ----------------------------------------
// Main
// ----------------------------------------
int main(int argc, char* argv[]) {
    if (!initSystems()) {
        // Falha crítica - exibe erro e sai
        gfxInitDefault();
        consoleDemoInit();
        printf("Erro ao inicializar sistemas!\n");
        printf("Pressione START para sair.\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) break;
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }
    
    // Cria render targets
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    // Inicia BGM do menu
    audio_play_bgm(audio, BGM_MENU);
    
    u64 lastTime = svcGetSystemTick();
    u64 accumulator = 0;
    
    // ---- Loop principal ----
    while (aptMainLoop()) {
        // Timing
        u64 currentTime = svcGetSystemTick();
        u64 deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        accumulator += deltaTime;
        
        // Captura input
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        
        // Saída de emergência
        if (kDown & KEY_SELECT) break;
        
        // Update com passo fixo
        while (accumulator >= TICKS_PER_FRAME) {
            switch (currentState) {
                case STATE_MENU:           updateMenu(kDown); break;
                case STATE_CHARACTER_SELECT: updateCharSelect(kDown); break;
                case STATE_FIGHT:          updateFight(kDown, kHeld); break;
                case STATE_PAUSE:          updatePause(kDown); break;
                case STATE_ROUND_END:      updateRoundEnd(kDown); break;
                case STATE_GAME_OVER:      updateGameOver(kDown); break;
                default: break;
            }
            accumulator -= TICKS_PER_FRAME;
            kDown = 0; // Processa kDown apenas uma vez
        }
        
        // Renderização
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        renderTopScreen(top);
        renderBottomScreen(bot);
        C3D_FrameEnd(0);
    }
    
    cleanupSystems();
    return 0;
}
