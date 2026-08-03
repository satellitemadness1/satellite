#include "satellite/machine.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

namespace satellite {
namespace machine {

// ---------------------------------------------------------------------------
// satellite charmap code -> keystroke
//
// The charmap is laid out in blocks, so the letters and digits are arithmetic
// and only the symbols need a table.
// ---------------------------------------------------------------------------
namespace {

// KEY_A .. KEY_Z in alphabetical order -- the QWERTY scancodes are scattered
const int kLetterKeys[26] = {
    key::A, key::B, key::C, key::D, key::E, key::F, key::G, key::H, key::I,
    key::J, key::K, key::L, key::M, key::N, key::O, key::P, key::Q, key::R,
    key::S, key::T, key::U, key::V, key::W, key::X, key::Y, key::Z,
};

// '0'..'9' -- KEY_1 is 2 and runs up to KEY_9 = 10, then KEY_0 = 11
const int kDigitKeys[10] = {11, 2, 3, 4, 5, 6, 7, 8, 9, 10};

struct SymKey { char32_t code; int keycode; bool shift; };

// codes 63-94 plus whitespace, in charmap order
const SymKey kSymbols[] = {
    { 63, 2,   true  },  // !   shift+1
    { 64, 3,   true  },  // @   shift+2
    { 65, 4,   true  },  // #   shift+3
    { 66, 5,   true  },  // $   shift+4
    { 67, 6,   true  },  // %   shift+5
    { 68, 7,   true  },  // ^   shift+6
    { 69, 8,   true  },  // &   shift+7
    { 70, 9,   true  },  // *   shift+8
    { 71, 10,  true  },  // (   shift+9
    { 72, 11,  true  },  // )   shift+0
    { 73, 12,  false },  // -   KEY_MINUS
    { 74, 12,  true  },  // _
    { 75, 13,  false },  // =   KEY_EQUAL
    { 76, 13,  true  },  // +
    { 77, 26,  false },  // [   KEY_LEFTBRACE
    { 78, 26,  true  },  // {
    { 79, 27,  false },  // ]   KEY_RIGHTBRACE
    { 80, 27,  true  },  // }
    { 81, 43,  false },  // backslash  KEY_BACKSLASH
    { 82, 43,  true  },  // |
    { 83, 39,  false },  // ;   KEY_SEMICOLON
    { 84, 39,  true  },  // :
    { 85, 40,  false },  // '   KEY_APOSTROPHE
    { 86, 40,  true  },  // "
    { 87, 51,  false },  // ,   KEY_COMMA
    { 88, 51,  true  },  // <
    { 89, 52,  false },  // .   KEY_DOT
    { 90, 52,  true  },  // >
    { 91, 53,  false },  // /   KEY_SLASH
    { 92, 53,  true  },  // ?
    { 93, 41,  false },  // `   KEY_GRAVE
    { 94, 41,  true  },  // ~
    { 95, key::SPACE, false },
    { 96, key::ENTER, false },
    { 97, key::TAB,   false },
};

}  // namespace

bool Keyboard::key_for_char(char32_t c, int* keycode, bool* shift) {
    if (charmap::is_lower(c)) {                    // 1-26
        *keycode = kLetterKeys[c - 1];
        *shift   = false;
        return true;
    }
    if (charmap::is_upper(c)) {                    // 27-52 -- same key, shifted
        *keycode = kLetterKeys[c - 27];
        *shift   = true;
        return true;
    }
    if (charmap::is_digit(c)) {                    // 53-62
        *keycode = kDigitKeys[c - 53];
        *shift   = false;
        return true;
    }
    for (const auto& s : kSymbols)
        if (s.code == c) { *keycode = s.keycode; *shift = s.shift; return true; }
    return false;                                  // void, or a math token
}

// ---------------------------------------------------------------------------
// device lifecycle
// ---------------------------------------------------------------------------
Keyboard::~Keyboard() { close(); }

void Keyboard::open_recording() {
    close();
    recording_ = true;
    recorded_.clear();
}

bool Keyboard::open(std::string* err) {
    close();
    fd_ = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        if (err) {
            *err = std::string("cannot open /dev/uinput: ") + std::strerror(errno);
            if (errno == EACCES || errno == EPERM)
                *err += "\n  /dev/uinput is root-only by default. Either run as root, or:\n"
                        "    echo 'KERNEL==\"uinput\", GROUP=\"input\", MODE=\"0660\"' "
                        "| sudo tee /etc/udev/rules.d/99-uinput.rules\n"
                        "    sudo usermod -aG input $USER   # then log out and back in";
        }
        return false;
    }

    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0) {
        if (err) *err = std::string("UI_SET_EVBIT: ") + std::strerror(errno);
        close();
        return false;
    }
    for (int k = 1; k <= key::MAX; ++k) ioctl(fd_, UI_SET_KEYBIT, k);

    uinput_setup setup{};
    setup.id.bustype = BUS_USB;
    setup.id.vendor  = 0x5341;                     // 'SA'
    setup.id.product = 0x5442;                     // 'TB'
    std::strncpy(setup.name, "satellite virtual keyboard", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(fd_, UI_DEV_SETUP, &setup) < 0) {
        if (err) *err = std::string("UI_DEV_SETUP: ") + std::strerror(errno);
        close();
        return false;
    }
    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        if (err) *err = std::string("UI_DEV_CREATE: ") + std::strerror(errno);
        close();
        return false;
    }

    // udev needs a moment to notice the new device; keys sent before it does
    // are silently dropped
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return true;
}

void Keyboard::close() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        ::close(fd_);
        fd_ = -1;
    }
    recording_ = false;
}

// ---------------------------------------------------------------------------
// emitting
// ---------------------------------------------------------------------------
bool Keyboard::emit(int type, int code, int value) {
    if (recording_) {
        if (type == EV_KEY) recorded_.push_back({code, value != 0});
        return true;
    }
    if (fd_ < 0) return false;
    input_event ev{};
    ev.type  = (uint16_t)type;
    ev.code  = (uint16_t)code;
    ev.value = value;
    return ::write(fd_, &ev, sizeof(ev)) == (ssize_t)sizeof(ev);
}

bool Keyboard::sync() { return emit(EV_SYN, SYN_REPORT, 0); }

bool Keyboard::key_down(int keycode) { return emit(EV_KEY, keycode, 1) && sync(); }
bool Keyboard::key_up(int keycode)   { return emit(EV_KEY, keycode, 0) && sync(); }

bool Keyboard::press(int keycode) {
    return key_down(keycode) && key_up(keycode);
}

// Hold every key but the last, tap the last, then release the held ones in
// reverse order.  Reverse matters: releasing Ctrl before C on a Ctrl+C would
// look to the application like a bare C after the shortcut ended.
bool Keyboard::press_combo(const std::vector<int>& keys) {
    if (keys.empty()) return false;
    if (keys.size() == 1) return press(keys[0]);

    size_t last = keys.size() - 1;
    for (size_t k = 0; k < last; ++k)
        if (!key_down(keys[k])) return false;

    bool ok = press(keys[last]);

    for (size_t k = last; k-- > 0;)
        if (!key_up(keys[k])) ok = false;          // always release, even on error

    return ok;
}

bool Keyboard::type_char(char32_t satellite_code) {
    int keycode;
    bool shift;
    if (!key_for_char(satellite_code, &keycode, &shift)) return false;
    return shift ? press_combo({key::SHIFT, keycode}) : press(keycode);
}

bool Keyboard::type_text(const SatString& s) {
    bool ok = true;
    for (uint32_t k = 0; k < s.len; ++k)
        if (!type_char(s.at(k))) ok = false;
    return ok;
}

bool Keyboard::type_text(const std::string& ascii) {
    SatString s;
    s.append_text(ascii);
    return type_text(s);
}

}  // namespace machine
}  // namespace satellite
