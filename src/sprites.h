#pragma once
#include <cstdint>
#include "utils.h"

// Sprite dimensions
constexpr int CHAR_W = 16;
constexpr int CHAR_H = 16;

// Orange cat palette sampled from the latest user-exported 16x16 sprite
constexpr uint16_t _ = Color::TRANSPARENT;
constexpr uint16_t K = rgb565(48, 48, 47);      // dark eye line
constexpr uint16_t B = rgb565(18, 22, 26);      // nose / darkest shadow
constexpr uint16_t W = rgb565(255, 255, 255);   // pure white
constexpr uint16_t D = rgb565(208, 71, 2);      // dark orange outline
constexpr uint16_t O = rgb565(210, 93, 31);     // main orange
constexpr uint16_t L = rgb565(229, 130, 61);    // light orange highlight
constexpr uint16_t C = rgb565(253, 226, 197);   // cream fur

// Idle frame 1: direct lift from the latest 16x16 pixel reference
const uint16_t PROGMEM sprite_idle1[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, O, _,
    O, O, K, L, C, C, L, K, L, O, O, _, _, O, C, O,
    O, O, K, C, B, B, C, K, L, O, O, _, _, O, W, O,
    _, O, L, C, K, K, C, C, L, O, _, _, _, O, L, O,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, _, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, _, D, _, D, _, _,
};

// Idle frame 2: blink
const uint16_t PROGMEM sprite_idle2[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, O, _,
    O, O, K, L, C, C, L, K, L, O, O, _, _, O, C, O,
    O, O, K, C, O, O, C, K, L, O, O, _, _, O, W, O,
    _, O, L, C, O, O, C, C, L, O, _, _, _, O, L, O,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, _, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, _, D, _, D, _, _,
};

// Idle frame 3: gentle bob with slightly shifted tail
const uint16_t PROGMEM sprite_idle3[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, O, _, _,
    O, O, K, L, C, C, L, K, L, O, O, _, O, C, O, _,
    O, O, K, C, B, B, C, K, L, O, O, _, O, W, O, _,
    _, O, L, C, K, K, C, C, L, O, _, _, O, L, O, _,
    _, _, O, C, C, C, C, C, O, O, O, O, O, O, _, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, O, _,
    _, _, _, O, L, C, C, L, L, L, L, L, O, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, O, _, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, O, _, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, O, _, _, _,
    _, _, _, _, O, _, _, O, _, _, D, _, D, _, _, _,
};

// Happy frame 1: brighter mouth / tail perked
const uint16_t PROGMEM sprite_happy1[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, O, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, O, C, O,
    O, O, K, L, C, C, L, K, L, O, O, O, O, W, O, _,
    O, O, K, C, B, W, C, K, L, O, O, _, _, O, W, O,
    _, O, L, C, B, B, C, C, L, O, _, _, O, W, O, _,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, O, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, _, D, _, D, _, _,
};

// Happy frame 2: alternate tail swing
const uint16_t PROGMEM sprite_happy2[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, O, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, O, W, O, _,
    O, O, K, L, C, C, L, K, L, O, O, _, _, O, W, O,
    O, O, K, C, B, W, C, K, L, O, O, _, O, W, O, _,
    _, O, L, C, K, K, C, C, L, O, _, _, _, O, O, _,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, _, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, D, _, D, _, _, _,
};

// Sleep frame 1: curled up version preserving the new face proportions
const uint16_t PROGMEM sprite_sleep1[CHAR_W * CHAR_H] = {
    _, _, _, _, _, _, O, O, D, _, _, _, _, _, _, _,
    _, _, _, _, _, O, O, O, O, O, _, _, _, _, _, _,
    _, _, _, _, O, O, L, L, L, O, O, _, _, _, _, _,
    _, _, _, O, O, L, C, C, C, L, O, O, _, _, _, _,
    _, _, O, O, L, C, K, K, C, C, L, O, O, _, _, _,
    _, O, O, L, C, K, B, B, K, C, C, L, O, O, _, _,
    _, O, O, L, C, C, C, O, O, C, C, C, O, O, _, _,
    _, O, O, O, L, L, O, O, O, O, L, L, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, _, O, O, O, O, O, O, O, O, O, _, _, _, _,
    _, _, _, _, O, O, O, O, O, O, O, _, _, _, _, _,
    _, _, _, _, _, O, O, O, O, O, _, _, _, _, _, _,
    _, _, _, _, _, _, O, O, O, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Talk frame 1: mouth open
const uint16_t PROGMEM sprite_talk1[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, O, _,
    O, O, K, L, C, C, L, K, L, O, O, _, _, O, C, O,
    O, O, K, C, B, D, C, K, L, O, O, _, _, O, W, O,
    _, O, L, C, K, K, C, C, L, O, _, _, _, O, L, O,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, _, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, _, D, _, D, _, _,
};

// Talk frame 2: neutral mouth
const uint16_t PROGMEM sprite_talk2[CHAR_W * CHAR_H] = {
    _, O, _, _, _, _, _, _, _, D, _, _, _, _, _, _,
    O, C, O, _, _, _, _, _, O, C, O, _, _, _, _, _,
    O, C, C, O, O, O, O, O, C, C, O, _, _, _, _, _,
    O, C, L, O, L, O, O, L, L, C, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, _, _,
    O, L, L, L, L, L, L, L, L, L, O, _, _, _, O, _,
    O, O, K, L, C, C, L, K, L, O, O, _, _, O, C, O,
    O, O, K, C, B, B, C, K, L, O, O, _, _, O, W, O,
    _, O, L, C, K, K, C, C, L, O, _, _, _, O, L, O,
    _, _, O, C, C, C, C, C, O, O, O, O, O, L, O, _,
    _, _, _, O, O, O, O, D, L, L, O, L, O, O, _, _,
    _, _, _, O, L, C, C, L, L, L, L, L, L, O, _, _,
    _, _, _, O, L, C, L, L, O, C, C, L, L, O, _, _,
    _, _, _, O, L, O, O, L, O, L, L, L, L, O, _, _,
    _, _, _, O, C, D, D, C, O, D, O, C, D, C, _, _,
    _, _, _, _, O, _, _, O, _, _, _, D, _, D, _, _,
};

const uint16_t* const idle_frames[] = { sprite_idle1, sprite_idle2, sprite_idle1, sprite_idle3 };
constexpr int IDLE_FRAME_COUNT = 4;

const uint16_t* const happy_frames[] = { sprite_happy1, sprite_happy2 };
constexpr int HAPPY_FRAME_COUNT = 2;

const uint16_t* const sleep_frames[] = { sprite_sleep1 };
constexpr int SLEEP_FRAME_COUNT = 1;

const uint16_t* const talk_frames[] = { sprite_talk1, sprite_talk2 };
constexpr int TALK_FRAME_COUNT = 2;
