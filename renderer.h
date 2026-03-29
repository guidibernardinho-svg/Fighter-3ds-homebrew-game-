// renderer.h
#pragma once
#include <3ds.h>
#include <citro2d.h>
#include "fighter.h"
#include "game.h"

typedef struct Renderer Renderer;

#ifdef __cplusplus
extern "C" {
#endif

Renderer* renderer_init(int screen_w, int screen_h);
void      renderer_destroy(Renderer* r);

void renderer_draw_background(Renderer* r, int stage_id);
void renderer_draw_fighters(Renderer* r, Fighter* p1, Fighter* p2);
void renderer_draw_effects(Renderer* r, Effect* effects);

// Helpers para UI
void renderer_draw_rect(Renderer* r, float x, float y, float w, float h, u32 color);
void renderer_draw_rect_outline(Renderer* r, float x, float y, float w, float h, u32 color, float thick);
void renderer_draw_text(Renderer* r, float x, float y, float scale, u32 color, const char* text);
void renderer_draw_text_centered(Renderer* r, float cx, float y, float scale, u32 color, const char* text);

#ifdef __cplusplus
}
#endif
