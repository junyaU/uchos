/*
 * @file hardware/keyboard.hpp
 */

#pragma once

static const int L_CONTROL = 0b00000001U;
static const int L_SHIFT = 0b00000010U;
static const int L_ALT = 0b00000100U;
static const int L_GUI = 0b00001000U;
static const int R_CONTROL = 0b00010000U;
static const int R_SHIFT = 0b00100000U;
static const int R_ALT = 0b01000000U;
static const int R_GUI = 0b10000000U;

void initialize_keyboard();