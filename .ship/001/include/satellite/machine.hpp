// satellite.machine -- synthetic input, via the kernel rather than the display
// server.
//
// /dev/uinput creates a virtual keyboard at the evdev layer, BELOW X11 and
// Wayland, so the same code works on both and on a bare console.  (XTEST, what
// xdotool uses, is X11-only and would not work on a Wayland session.)
//
// /dev/uinput is root-only by default.  To use it as a normal user:
//     # /etc/udev/rules.d/99-uinput.rules
//     KERNEL=="uinput", GROUP="input", MODE="0660"
// then add the user to the `input` group and replug/reboot.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "satellite/container.hpp"

namespace satellite {
namespace machine {

// Linux key codes (from <linux/input-event-codes.h>).  These are scancodes --
// unrelated to ASCII and unrelated to the satellite charmap.
namespace key {
constexpr int ESC = 1, BACKSPACE = 14, TAB = 15, ENTER = 28, SPACE = 57;
constexpr int LEFTCTRL = 29, LEFTSHIFT = 42, LEFTALT = 56, LEFTMETA = 125;
constexpr int RIGHTCTRL = 97, RIGHTSHIFT = 54, RIGHTALT = 100;
constexpr int CTRL = LEFTCTRL, SHIFT = LEFTSHIFT, ALT = LEFTALT, META = LEFTMETA;
constexpr int A = 30, B = 48, C = 46, D = 32, E = 18, F = 33, G = 34, H = 35;
constexpr int I = 23, J = 36, K = 37, L = 38, M = 50, N = 49, O = 24, P = 25;
constexpr int Q = 16, R = 19, S = 31, T = 20, U = 22, V = 47, W = 17, X = 45;
constexpr int Y = 21, Z = 44;
constexpr int F1 = 59, F2 = 60, F3 = 61, F4 = 62, F5 = 63, F6 = 64;
constexpr int F7 = 65, F8 = 66, F9 = 67, F10 = 68, F11 = 87, F12 = 88;
constexpr int UP = 103, DOWN = 108, LEFT = 105, RIGHT = 106;
constexpr int HOME = 102, END = 107, PAGEUP = 104, PAGEDOWN = 109, DELETE = 111;
constexpr int MAX = 255;
}  // namespace key

struct KeyEvent {
    int  code;
    bool down;
};

class Keyboard {
public:
    Keyboard() = default;
    ~Keyboard();
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    // Creates the virtual device.  Returns false and fills `err` on failure --
    // permission denied is the usual one, see the udev note above.
    bool open(std::string* err = nullptr);

    // Test mode: record what WOULD be sent, touch no hardware.  This is what
    // lets the key logic be tested without root.
    void open_recording();

    bool is_open() const { return fd_ >= 0 || recording_; }
    void close();

    // --- raw ---------------------------------------------------------------
    bool key_down(int keycode);
    bool key_up(int keycode);
    bool press(int keycode);                    // down then up

    // --- combinations ------------------------------------------------------
    // Any number of keys.  All but the last are held down in order, the last is
    // tapped, then the held keys are released in REVERSE order -- which is what
    // applications expect from a real keyboard.
    //
    //   press_combo({key::CTRL, key::C})                    -> Ctrl+C
    //   press_combo({key::CTRL, key::SHIFT, key::C})        -> Ctrl+Shift+C
    bool press_combo(const std::vector<int>& keys);
    bool press_combo(int a, int b)        { return press_combo({a, b}); }
    bool press_combo(int a, int b, int c) { return press_combo({a, b, c}); }

    // --- text --------------------------------------------------------------
    // Takes a SATELLITE charmap code and works out the keystroke, including
    // whether shift is needed: 'A' becomes Shift+A, '!' becomes Shift+1.
    bool type_char(char32_t satellite_code);
    bool type_text(const SatString& s);
    bool type_text(const std::string& ascii);

    // --- recording (test mode) ---------------------------------------------
    const std::vector<KeyEvent>& recorded() const { return recorded_; }
    void clear_recorded() { recorded_.clear(); }

    // satellite charmap code -> (keycode, needs shift).  False if unmappable.
    static bool key_for_char(char32_t satellite_code, int* keycode, bool* shift);

private:
    int  fd_        = -1;
    bool recording_ = false;
    std::vector<KeyEvent> recorded_;

    bool emit(int type, int code, int value);
    bool sync();
};

}  // namespace machine
}  // namespace satellite
