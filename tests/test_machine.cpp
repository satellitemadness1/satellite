// Tests run in recording mode -- no root, no hardware, no keys actually sent.
#include "satellite/machine.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;
using namespace satellite::machine;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

// renders recorded events as  +CTRL +C -C -CTRL
static std::string render(const std::vector<KeyEvent>& evs) {
    auto name = [](int c) -> std::string {
        switch (c) {
            case key::CTRL:  return "CTRL";
            case key::SHIFT: return "SHIFT";
            case key::ALT:   return "ALT";
            case key::META:  return "META";
            case key::SPACE: return "SPACE";
            case key::ENTER: return "ENTER";
            case key::TAB:   return "TAB";
            case key::A: return "A"; case key::C: return "C"; case key::H: return "H";
            case key::I: return "I"; case key::T: return "T"; case key::Z: return "Z";
            case key::F5: return "F5";
            default: return std::to_string(c);
        }
    };
    std::string s;
    for (const auto& e : evs) {
        if (!s.empty()) s += ' ';
        s += (e.down ? "+" : "-") + name(e.code);
    }
    return s;
}

static void expect(Keyboard& kb, const std::string& want, const std::string& what) {
    ++checks;
    std::string got = render(kb.recorded());
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want %s\n        got  %s\n",
               what.c_str(), want.c_str(), got.c_str());
    }
    kb.clear_recorded();
}

int main() {
    printf("satellite.machine tests\n");

    Keyboard kb;
    kb.open_recording();
    check(kb.is_open(), "recording mode opens without hardware");

    // ---- one key -----------------------------------------------------------
    kb.press(key::A);
    expect(kb, "+A -A", "press(A) is down then up");

    kb.key_down(key::SHIFT);
    kb.key_up(key::SHIFT);
    expect(kb, "+SHIFT -SHIFT", "key_down / key_up separately");

    // ---- two keys ----------------------------------------------------------
    kb.press_combo(key::CTRL, key::C);
    expect(kb, "+CTRL +C -C -CTRL", "Ctrl+C holds ctrl, taps c, releases ctrl");

    kb.press_combo(key::ALT, key::TAB);
    expect(kb, "+ALT +TAB -TAB -ALT", "Alt+Tab");

    // ---- three keys --------------------------------------------------------
    kb.press_combo(key::CTRL, key::SHIFT, key::C);
    expect(kb, "+CTRL +SHIFT +C -C -SHIFT -CTRL",
           "Ctrl+Shift+C releases in reverse order");

    kb.press_combo(key::CTRL, key::ALT, key::DELETE);
    expect(kb, "+CTRL +ALT +111 -111 -ALT -CTRL", "Ctrl+Alt+Delete");

    // ---- any number --------------------------------------------------------
    kb.press_combo({key::CTRL, key::SHIFT, key::ALT, key::META, key::A});
    expect(kb, "+CTRL +SHIFT +ALT +META +A -A -META -ALT -SHIFT -CTRL",
           "five keys still release in reverse");

    kb.press_combo({key::F5});
    expect(kb, "+F5 -F5", "a one-key combo is just a press");

    check(!kb.press_combo({}), "an empty combo is refused");

    // ---- characters go through the satellite charmap ------------------------
    {
        int code; bool shift;
        check(Keyboard::key_for_char(charmap::from_ascii('a'), &code, &shift) &&
              code == key::A && !shift, "'a' is KEY_A unshifted");
        check(Keyboard::key_for_char(charmap::from_ascii('A'), &code, &shift) &&
              code == key::A && shift, "'A' is the SAME key, shifted");
        check(Keyboard::key_for_char(charmap::from_ascii('z'), &code, &shift) &&
              code == key::Z && !shift, "'z' is KEY_Z");
        check(Keyboard::key_for_char(charmap::from_ascii('1'), &code, &shift) &&
              code == 2 && !shift, "'1' is KEY_1 (scancode 2)");
        check(Keyboard::key_for_char(charmap::from_ascii('0'), &code, &shift) &&
              code == 11 && !shift, "'0' is KEY_0 (scancode 11), after the 9");
        check(Keyboard::key_for_char(charmap::from_ascii('!'), &code, &shift) &&
              code == 2 && shift, "'!' is shift+1");
        check(Keyboard::key_for_char(charmap::from_ascii(' '), &code, &shift) &&
              code == key::SPACE && !shift, "space maps to KEY_SPACE");
        check(Keyboard::key_for_char(charmap::from_ascii('-'), &code, &shift) &&
              code == 12 && !shift, "'-' is KEY_MINUS");
        check(Keyboard::key_for_char(charmap::from_ascii('_'), &code, &shift) &&
              code == 12 && shift, "'_' is the same key, shifted");
        check(!Keyboard::key_for_char(charmap::VOID, &code, &shift),
              "the void character has no keystroke");
        check(!Keyboard::key_for_char(charmap::OP_MINUS, &code, &shift),
              "a math token has no keystroke either");
    }

    kb.type_char(charmap::from_ascii('a'));
    expect(kb, "+A -A", "typing 'a'");

    kb.type_char(charmap::from_ascii('A'));
    expect(kb, "+SHIFT +A -A -SHIFT", "typing 'A' adds shift automatically");

    kb.type_text("Hi");
    expect(kb, "+SHIFT +H -H -SHIFT +I -I", "typing text shifts only where needed");

    kb.type_text("a c");
    expect(kb, "+A -A +SPACE -SPACE +C -C", "spaces are typed");

    // ---- the real device ---------------------------------------------------
    // Not opened here: /dev/uinput is root-only by default, so this asserts the
    // failure is reported clearly rather than crashing.
    {
        Keyboard real;
        std::string err;
        bool opened = real.open(&err);
        if (!opened) {
            check(!err.empty(), "a failed open explains why");
            check(err.find("uinput") != std::string::npos, "the error names the device");
            printf("  note: real device unavailable, as expected -- %s\n",
                   err.substr(0, err.find('\n')).c_str());
        } else {
            printf("  note: real uinput device opened successfully\n");
            check(real.is_open(), "real device reports open");
            real.close();
        }
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
