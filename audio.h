// audio.h
#pragma once
#include <3ds.h>

typedef enum {
    BGM_MENU = 0,
    BGM_CHAR_SELECT,
    BGM_FIGHT,
    BGM_VICTORY,
    BGM_COUNT
} BGMTrack;

typedef enum {
    SFX_PUNCH = 0,
    SFX_KICK,
    SFX_BLOCK,
    SFX_HIT,
    SFX_KO,
    SFX_CONFIRM,
    SFX_CANCEL,
    SFX_SUPER,
    SFX_COUNT
} SFXId;

typedef struct AudioManager AudioManager;

#ifdef __cplusplus
extern "C" {
#endif

AudioManager* audio_init(void);
void          audio_destroy(AudioManager* a);
void          audio_play_bgm(AudioManager* a, BGMTrack track);
void          audio_stop_bgm(AudioManager* a);
void          audio_play_sfx(AudioManager* a, SFXId sfx);
void          audio_set_bgm_volume(AudioManager* a, float vol);
void          audio_set_sfx_volume(AudioManager* a, float vol);

#ifdef __cplusplus
}
#endif
