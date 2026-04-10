#pragma once
#include <cstdint>
#include "sprites.h"
#include "utils.h"

// Alternate purple cat sprite set sampled from the user-exported 16x16 sprite.
// These resources are kept separate so gameplay can switch breeds/colors later
// without touching the currently active orange-cat defaults.

constexpr uint16_t P0 = Color::TRANSPARENT;
constexpr uint16_t P1 = rgb565(84, 56, 169);     // dark purple outline
constexpr uint16_t P2 = rgb565(199, 141, 226);   // main lavender fill
constexpr uint16_t P3 = rgb565(226, 198, 255);   // bright lavender highlight
constexpr uint16_t P4 = rgb565(255, 226, 255);   // brightest muzzle highlight
constexpr uint16_t P5 = rgb565(235, 235, 235);   // soft eye white
constexpr uint16_t P6 = rgb565(252, 252, 252);   // bright eye white

const uint16_t PROGMEM purple_sprite_idle1[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P1, P1, P1,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P2, P2, P4, P2, P4, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P0, P1, P3, P3, P1,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P1, P3, P1, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_idle2[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P1, P1, P1,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1,
    P1, P2, P2, P5, P2, P5, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P2, P2, P3, P2, P3, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P0, P1, P3, P3, P1,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P1, P3, P1, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_idle3[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P1, P1, P1, P0,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P3, P1, P0,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P1, P2, P2, P4, P2, P4, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P1, P0,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P3, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P0, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P0, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_happy1[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P1, P1, P1,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P2, P2, P4, P2, P3, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P0, P1, P3, P3, P1,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P1, P3, P1, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_happy2[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P1, P1, P1, P0,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P3, P1, P0,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P1, P2, P2, P4, P2, P3, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P1, P0, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P1, P3, P1, P0,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_sleep1[CHAR_W * CHAR_H] = {
    P0, P0, P0, P0, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P1, P2, P1, P1, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P1, P2, P3, P2, P1, P1, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P0, P0,
    P0, P0, P1, P2, P2, P5, P6, P2, P2, P2, P1, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P3, P2, P2, P2, P3, P2, P2, P1, P0, P0, P0, P0,
    P0, P1, P2, P2, P2, P1, P1, P2, P2, P2, P2, P1, P0, P0, P0, P0,
    P0, P0, P1, P1, P1, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P1, P1, P1, P1, P1, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P1, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_talk1[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P1, P1, P1,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P2, P2, P4, P2, P3, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P3, P2, P2, P2, P3, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P0, P1, P3, P3, P1,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P1, P3, P1, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t PROGMEM purple_sprite_talk2[CHAR_W * CHAR_H] = {
    P0, P0, P1, P1, P0, P0, P1, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P1, P0, P1, P2, P1, P0, P0, P0, P0, P0, P0, P0, P0,
    P0, P1, P2, P2, P1, P1, P3, P2, P1, P0, P0, P0, P0, P0, P0, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P1, P1, P0, P0, P0, P1, P1, P1,
    P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P3, P1,
    P1, P2, P2, P5, P2, P6, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P2, P2, P4, P2, P4, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P1, P3, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1, P0,
    P0, P1, P2, P2, P2, P2, P2, P2, P1, P0, P0, P0, P1, P3, P3, P1,
    P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P0, P0, P1, P3, P1,
    P0, P0, P1, P2, P2, P2, P2, P2, P2, P2, P2, P1, P1, P3, P1, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P2, P2, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P1, P3, P2, P2, P2, P1, P1, P2, P2, P3, P1, P0, P0, P0,
    P0, P0, P0, P1, P2, P2, P1, P1, P1, P1, P1, P1, P0, P0, P0, P0,
    P0, P0, P0, P1, P1, P1, P1, P3, P1, P1, P0, P0, P0, P0, P0, P0,
};

const uint16_t* const purple_idle_frames[] = {
    purple_sprite_idle1,
    purple_sprite_idle2,
    purple_sprite_idle1,
    purple_sprite_idle3,
};
constexpr int PURPLE_IDLE_FRAME_COUNT = 4;

const uint16_t* const purple_happy_frames[] = {
    purple_sprite_happy1,
    purple_sprite_happy2,
};
constexpr int PURPLE_HAPPY_FRAME_COUNT = 2;

const uint16_t* const purple_sleep_frames[] = { purple_sprite_sleep1 };
constexpr int PURPLE_SLEEP_FRAME_COUNT = 1;

const uint16_t* const purple_talk_frames[] = {
    purple_sprite_talk1,
    purple_sprite_talk2,
};
constexpr int PURPLE_TALK_FRAME_COUNT = 2;
