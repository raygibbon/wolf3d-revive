/* SPDX-License-Identifier: GPL-2.0-only */
#include "sdl_input.h"
#include "engine/ID_IN.h"
#include "engine/WL_CONTROLS.h"
#include "engine/WL_MAIN.h"
#include <math.h>
#include <SDL3/SDL.h>
#include <string.h>
static bool physical[SDL_SCANCODE_COUNT],mousebuttons[8],focused=true,captured,capture_failed;
static SDL_Window *capture_window;
static double menu_x,menu_y;
static ui_state mouse_state;
static char capture_error[256];
bool IN_SDLFocused(void) { return focused; }
void IN_SDLDiscardInput(void) { memset(physical,0,sizeof(physical));IN_ClearKeysDown(); }
bool IN_SDLCaptured(void) { return captured; }
const char *IN_SDLCaptureError(void) { return capture_error; }
void IN_SDLReset(void) { IN_SDLFocusLost();focused=true;capture_window=NULL;capture_error[0]=0; }
void IN_SDLFocusGained(void) { focused=true;IN_ClearKeysDown(); }
void IN_SDLCapture(SDL_Window *window,bool gameplay)
{
    if(window!=capture_window) capture_failed=false;
    capture_window=window;
    bool want=window && gameplay && focused && CT_MouseEnabled();
    if(!want) capture_failed=false;
    if(captured==want || (want && capture_failed)) return;
    if(window && !SDL_SetWindowRelativeMouseMode(window,want)) {
        (void)SDL_strlcpy(capture_error,SDL_GetError(),sizeof(capture_error));
        SDL_Log("Relative mouse unavailable: %s; keyboard controls remain active",capture_error);
        captured=false;capture_failed=true;return;
    }
    captured=want;CT_Clear();
    /* Ignore motion queued before a mode/menu/focus transition. */
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
}
void IN_SDLMouseMotion(double x) { if(captured && focused) CT_MouseMotion(x); }
static bool PointerMenu(void)
{
    ui_state state=DemoLoopState();
    if(state!=mouse_state) { menu_x=menu_y=0;mouse_state=state; }
    return state==UI_MENU || state==UI_EPISODE || state==UI_DIFFICULTY || state==UI_SOUND ||
        state==UI_CONTROLS || state==UI_DISPLAY || state==UI_VIEW || state==UI_SAVE || state==UI_LOAD || state==UI_JUKEBOX;
}
static void Pulse(unsigned scan)
{ if(!Keyboard[scan]) { IN_MenuKeyEvent(scan,true);IN_MenuKeyEvent(scan,false); } }
void IN_SDLPointerMotion(double x,double y)
{
    if(captured) { IN_SDLMouseMotion(x);return; }
    if(!focused || !PointerMenu() || !CT_MouseEnabled() || !isfinite(x) || !isfinite(y)) return;
    menu_x+=x;menu_y+=y;
    /* WL_MENU.C ReadAnyControl: strict 60-count threshold, horizontal wins. */
    unsigned scan=menu_x< -60?sc_LeftArrow:menu_x>60?sc_RightArrow:menu_y< -60?sc_UpArrow:menu_y>60?sc_DownArrow:0U;
    if(scan) { Pulse(scan);menu_x=menu_y=0; }
}
void IN_SDLMouseButton(unsigned button,bool down)
{
    if(button>=8 || mousebuttons[button]==down) return;
    mousebuttons[button]=down;
    if(captured && focused) CT_MouseButton(button,down);
    else if(focused && down) {
        if(PointerMenu() && CT_MouseEnabled()) {
            if(button==SDL_BUTTON_LEFT) Pulse(sc_Enter);
            else if(button==SDL_BUTTON_RIGHT) Pulse(sc_Escape);
        } else IN_AckEvent();
    }
}
unsigned IN_MapSDLScancode(unsigned code)
{
    static const unsigned letters[26] = {0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c};
    if (code >= SDL_SCANCODE_A && code <= SDL_SCANCODE_Z) return letters[code-SDL_SCANCODE_A];
    if (code >= SDL_SCANCODE_1 && code <= SDL_SCANCODE_9) return 2 + code-SDL_SCANCODE_1;
    if (code >= SDL_SCANCODE_F1 && code <= SDL_SCANCODE_F10) return 0x3b + code-SDL_SCANCODE_F1;
    switch (code) {
        case SDL_SCANCODE_PAUSE: return sc_Pause;
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
        case SDL_SCANCODE_CAPSLOCK: return 0x3a;
        case SDL_SCANCODE_MINUS: return 0x0c;
        case SDL_SCANCODE_EQUALS: return 0x0d;
        case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
        case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
        case SDL_SCANCODE_SEMICOLON: return 0x27;
        case SDL_SCANCODE_APOSTROPHE: return 0x28;
        case SDL_SCANCODE_GRAVE: return 0x29;
        case SDL_SCANCODE_BACKSLASH: return 0x2b;
        case SDL_SCANCODE_COMMA: return 0x33;
        case SDL_SCANCODE_PERIOD: return 0x34;
        case SDL_SCANCODE_SLASH: return 0x35;
        case SDL_SCANCODE_HOME: return 0x47;
        case SDL_SCANCODE_END: return 0x4f;
        case SDL_SCANCODE_DELETE: return 0x53;
        case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return 0x1d;
        case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return 0x38;
        case SDL_SCANCODE_LSHIFT: return 0x2a;
        case SDL_SCANCODE_RSHIFT: return 0x36;
        default: return 0;
    }
}
void IN_SDLFocusLost(void)
{
    focused=false;menu_x=menu_y=0;
    if(captured && capture_window) (void)SDL_SetWindowRelativeMouseMode(capture_window,false);
    captured=false;capture_failed=false;memset(physical,0,sizeof(physical));memset(mousebuttons,0,sizeof(mousebuttons));IN_ClearKeysDown();
}
void IN_SDLKey(unsigned code, bool down)
{
    if (code >= SDL_SCANCODE_COUNT || !focused || physical[code] == down) return;
    physical[code] = down;CT_Key(code,down);
    unsigned scan = IN_MapSDLScancode(code);
    if (!scan) return;
    bool held = false;
    for (unsigned i = 0; i < SDL_SCANCODE_COUNT; ++i)
        if (physical[i] && IN_MapSDLScancode(i) == scan) { held = true; break; }
    IN_MenuKeyEvent(scan,held);
}
