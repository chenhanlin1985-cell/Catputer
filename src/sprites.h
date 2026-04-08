#pragma once
#include <cstdint>
#include "utils.h"

// Sprite dimensions
constexpr int CHAR_W = 16;
constexpr int CHAR_H = 16;

// Orange cat palette
constexpr uint16_t _ = Color::TRANSPARENT;
constexpr uint16_t K = Color::BLACK;
constexpr uint16_t W = Color::WHITE;
constexpr uint16_t O = rgb565(232, 86, 64);

// Idle frame 1: front-facing kitten based on user reference
const uint16_t PROGMEM sprite_idle1[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, _, _,
    O, O, O, K, O, W, O, O, K, O, O, O, _, _, _, _,
    O, O, O, O, W, K, W, O, O, O, O, O, _, _, O, O,
    O, O, O, O, W, W, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Idle frame 2: blink
const uint16_t PROGMEM sprite_idle2[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, _, _,
    O, O, O, O, O, W, O, O, O, O, O, O, _, _, _, _,
    O, O, O, O, W, K, W, O, O, O, O, O, _, _, O, O,
    O, O, O, O, W, W, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Idle frame 3: slight bob / tail shift
const uint16_t PROGMEM sprite_idle3[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, _, _,
    O, O, O, K, O, W, O, O, K, O, O, O, _, _, _, _,
    O, O, O, O, W, K, W, O, O, O, O, O, _, O, O, _,
    O, O, O, O, W, W, W, O, O, O, O, O, O, W, O, _,
    _, O, O, O, O, O, O, O, O, O, O, _, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, _, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, O, O, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Happy frame 1: tail perked and bright expression
const uint16_t PROGMEM sprite_happy1[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, O, _,
    O, O, O, K, O, W, O, O, K, O, O, O, _, O, W, O,
    O, O, O, O, W, K, W, O, O, O, O, O, O, W, O, _,
    O, O, O, O, W, W, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Happy frame 2: alternate tail curl
const uint16_t PROGMEM sprite_happy2[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, O, _, _,
    O, O, O, K, O, W, O, O, K, O, O, O, O, W, O, _,
    O, O, O, O, W, K, W, O, O, O, O, O, W, O, _, _,
    O, O, O, O, W, W, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Sleep frame 1: curled up ball
const uint16_t PROGMEM sprite_sleep1[CHAR_W * CHAR_H] = {
    _, _, _, _, _, _, _, O, O, _, _, _, _, _, _, _,
    _, _, _, _, _, O, O, O, O, O, _, _, _, _, _, _,
    _, _, _, _, O, O, O, O, O, O, O, _, _, _, _, _,
    _, _, _, O, O, O, O, O, O, O, O, O, _, _, _, _,
    _, _, O, O, O, W, W, W, W, O, O, O, O, _, _, _,
    _, O, O, O, W, W, K, W, W, W, O, O, O, O, _, _,
    _, O, O, O, W, K, W, O, O, W, W, O, O, O, _, _,
    _, O, O, O, W, W, W, O, O, O, W, W, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, _, O, O, O, O, O, O, O, O, O, _, _, _, _,
    _, _, _, _, O, O, O, O, O, O, O, _, _, _, _, _,
    _, _, _, _, _, O, O, O, O, O, _, _, _, _, _, _,
    _, _, _, _, _, _, O, O, O, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Talk frame 1: open mouth
const uint16_t PROGMEM sprite_talk1[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, _, _,
    O, O, O, K, O, W, O, O, K, O, O, O, _, _, _, _,
    O, O, O, O, W, K, W, O, O, O, O, O, _, _, O, O,
    O, O, O, O, W, O, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

// Talk frame 2: closed mouth
const uint16_t PROGMEM sprite_talk2[CHAR_W * CHAR_H] = {
    _, _, O, _, _, _, _, _, _, O, _, _, _, _, _, _,
    _, O, W, O, _, _, _, _, O, W, O, _, _, _, _, _,
    O, W, W, W, O, O, O, O, W, W, W, O, _, _, _, _,
    O, W, W, O, O, O, O, O, O, W, W, O, _, _, _, _,
    O, W, O, O, O, O, O, O, O, O, W, O, _, _, _, _,
    _, O, O, K, O, O, O, O, K, O, O, _, _, _, _, _,
    O, O, O, K, O, W, O, O, K, O, O, O, _, _, _, _,
    O, O, O, O, W, K, W, O, O, O, O, O, _, _, O, O,
    O, O, O, O, W, W, W, O, O, O, O, O, _, O, W, O,
    _, O, O, O, O, O, O, O, O, O, O, _, O, W, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, O, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, O, _, _,
    _, _, O, O, O, O, O, O, O, O, O, O, O, _, _, _,
    _, _, O, O, _, W, O, O, O, W, _, O, O, _, _, _,
    _, O, O, O, O, O, O, O, W, O, O, O, O, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
};

const uint16_t* const idle_frames[] = { sprite_idle1, sprite_idle2, sprite_idle1, sprite_idle3 };
constexpr int IDLE_FRAME_COUNT = 4;

const uint16_t* const happy_frames[] = { sprite_happy1, sprite_happy2 };
constexpr int HAPPY_FRAME_COUNT = 2;

const uint16_t* const sleep_frames[] = { sprite_sleep1 };
constexpr int SLEEP_FRAME_COUNT = 1;

const uint16_t* const talk_frames[] = { sprite_talk1, sprite_talk2 };
constexpr int TALK_FRAME_COUNT = 2;
