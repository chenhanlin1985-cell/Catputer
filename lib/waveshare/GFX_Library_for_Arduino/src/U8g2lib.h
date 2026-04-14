#pragma once

#include <Arduino.h>

// Arduino_GFX only needs these U8g2 font feature macros for its built-in
// UTF-8 font decoder; pulling the full U8g2 display-driver library is heavy.
#ifndef U8G2_WITH_UNICODE
#define U8G2_WITH_UNICODE
#endif

#ifndef U8G2_USE_LARGE_FONTS
#define U8G2_USE_LARGE_FONTS
#endif

#ifndef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name)
#endif
