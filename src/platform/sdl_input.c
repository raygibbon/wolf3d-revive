/* SPDX-License-Identifier: GPL-2.0-only */
#include "sdl_input.h"
#include "engine/ID_IN.h"
#include <SDL3/SDL.h>
#include <string.h>
static bool physical[SDL_SCANCODE_COUNT];
unsigned IN_MapSDLScancode(unsigned code)
{
    static const unsigned letters[26] = {0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) return letters[code-SDL_SCANCODE_A];
    if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9) return 2 + code-SDL_SCANCODE_1;
    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10) return 0x3b + code-SDL_SCANCODE_F1;
    switch (code) {
        case SDL_SCANCODE_0: return 0x0b;
        case SDL_SCANCODE_ESCAPE: return sc_Escape;
        case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return sc_Enter;
        case SDL_SCANCODE_SPACE: return sc_Space;
        case SDL_SCANCODE_UP: case SDL_SCANCODE_KP_8: return sc_UpArrow;
        case SDL_SCANCODE_DOWN: case SDL_SCANCODE_KP_2: return sc_DownArrow;
        case SDL_SCANCODE_LEFT: case SDL_SCANCODE_KP_4: return sc_LeftArrow;
        case SDL_SCANCODE_RIGHT: case SDL_SCANCODE_KP_6: return sc_RightArrow;
        case SDL_SCANCODE_BACKSPACE: return 0x0e;
        case SDL_SCANCODE_TAB: return 0x0f;
        case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 0x1d;
        case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return 0x38;
        case SDL_SCANCODE_LSHIFT: return 0x2a;
        case SDL_SCANCODE_RSHIFT: return 0x36;
        default: return 0;
    }
}
void IN_SDLFocusLost(void) { memset(physical,0,sizeof(physical)); IN_ClearKeysDown(); }
void IN_SDLKey(unsigned code, bool down)
{
    if (code >= SDL_SCANCODE_COUNT || physical[code] == down) return;
    physical[code] = down;
    unsigned scan = IN_MapSDLScancode(code);
    if (!scan) return;
    bool held = false;
    for (unsigned i = 0; i < SDL_SCANCODE_COUNT; ++i)
        if (physical[i] && IN_MapSDLScancode(i) == scan) { held = true; break; }
    IN_KeyEvent(scan,held);
}
