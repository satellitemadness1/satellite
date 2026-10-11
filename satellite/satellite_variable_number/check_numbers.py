#!/usr/bin/env python3
# satellite/satellite_variable_number/check_numbers.py -- proves satellite_number
# against Python's int, which is exact at every size.
#
# Every case goes through build/number_cases (number_cases.cpp) and must match:
#   + - * compare negate digits bytes limbs, from_text/to_text, from_signed, the
#   constructor, and divide -- checked against TRUNCATION toward zero with the
#   remainder taking the dividend's sign. Python's // floors, so trunc_divide
#   below works it out itself.
# The fast path must not allocate: every + - * whose two numbers and answer all
# fit one limb must report 0 calls to operator new.
# Aliasing: a += a, a -= a, a *= a, divide(a, a, a, b), divide(a, b, q, q) (the
# object holds the remainder) and divide(a, b, a, b).
#
# Cases: 0, -0, +/-1, every 2^(64k) - 1, 2^(64k), 2^(64k) + 1 up to k = 8, carries
# and borrows across ten limbs, 27 nines, a 10,000-digit number, Knuth D's rare
# add-back step (constructed, see knuth_add_back_cases), 50,000 random pairs with a
# fixed seed, hostile text, and a 1,000,000-digit number (timed).
#
#     make build/number_cases && python3 satellite/satellite_variable_number/check_numbers.py [--quick] [harness]

import os
import random
import subprocess
import sys
import time

if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)  # Python's own guard; satellite has none

HERE = os.path.dirname(os.path.abspath(__file__))
QUICK = "--quick" in sys.argv  # skip the million-digit section (about a minute)
ARGUMENTS = [argument for argument in sys.argv[1:] if argument != "--quick"]
HARNESS = ARGUMENTS[0] if ARGUMENTS else os.path.join(HERE, "..", "..", "build", "number_cases")
LIMB = 2 ** 64


def trunc_divide(a, b):
    quotient = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        quotient = -quotient
    return quotient, a - quotient * b


def limb_count(n):
    return max(1, (abs(n).bit_length() + 63) // 64)


def fits(n):
    return abs(n) < LIMB


def text_answer(raw):
    first = 1 if raw[:1] == b"-" else 0
    if first == len(raw):
        return f"3 {first}"
    for index in range(first, len(raw)):
        if not 0x30 <= raw[index] <= 0x39:
            return f"3 {index}"
    value = int(raw[first:].decode("ascii"))
    value = -value if first else value
    return f"ok {value}" + (" negative" if value < 0 else "")


cases = []  # (line, expected answer, kind)


def binary(a, b):
    for operation, value in (("add", a + b), ("sub", a - b), ("mul", a * b)):
        cases.append((f"{operation} {a} {b}", value, "arithmetic", a, b))
    if b == 0:
        cases.append((f"divide {a} {b}", "22 division_by_zero", "exact"))
        cases.append((f"divide_same {a} {b}", "22 division_by_zero", "exact"))
        cases.append((f"divide_inputs {a} {b}", "22 division_by_zero", "exact"))
    else:
        q, r = trunc_divide(a, b)
        assert q * b + r == a and abs(r) < abs(b) and (r == 0 or (r < 0) == (a < 0))
        cases.append((f"divide {a} {b}", f"{q} {r}", "exact"))
        cases.append((f"divide_same {a} {b}", f"{r}", "exact"))
        cases.append((f"divide_inputs {a} {b}", f"{q} {r} {q} {r}", "exact"))
    cases.append((f"compare {a} {b}", str((a > b) - (a < b)), "exact"))


def unary(a):
    cases.append((f"negate {a}", str(-a), "exact"))
    cases.append((f"digits {a}", str(len(str(abs(a)))), "exact"))
    cases.append((f"bytes {a}", str(limb_count(a) * 8), "exact"))
    cases.append((f"limbs {a}", f"{limb_count(a)} {1 if fits(a) else 0}", "exact"))
    limbs = ",".join(format((abs(a) >> (64 * i)) & (LIMB - 1), "x") for i in range(limb_count(a)))
    cases.append((f"limbvalues {a}", f"{1 if a < 0 else 0} {limbs}", "exact"))
    cases.append((f"self {a}", f"{2 * a} 0 {a * a} {2 * a} 0 {a * a} {a} {a}", "exact"))
    cases.append((f"self_divide {a}", "22 division_by_zero" if a == 0 else "1 0", "exact"))
    cases.append((f"text :{str(a).encode().hex()}", text_answer(str(a).encode()), "exact"))


# power, whose answer Python gives exactly with ** -- except at a negative
# exponent, where Python answers a FLOAT and satellite answers a machine code.
# The three cases that stay whole are worked out here rather than asked of **.
def power_expected(a, b):
    if b >= 0:
        return str(a ** b)
    if a == 0:
        return "22 division_by_zero"          # 1/0, said as the sharper of the two
    if a == 1:
        return "1"
    if a == -1:
        return "-1" if b % 2 else "1"
    return "24 answer_is_not_whole"           # a real answer a WHOLE number cannot hold


def power_cases():
    bases = [0, 1, -1, 2, -2, 3, -3, 7, -7, 10, -10, 123456789, -123456789,
             LIMB - 1, -(LIMB - 1), LIMB, LIMB + 1, -(LIMB + 1), 2 ** 128 - 1, -(2 ** 128)]
    exponents = list(range(0, 34)) + [40, 63, 64, 65, 100, 127, 128, 200, 1000]
    for a in bases:
        for b in exponents:
            # Keep the ANSWER a sane size -- this is the referee's budget, not a
            # limit on satellite: 2 ^ 1000 is checked, 2 ^ 1000000 is not asked for.
            if abs(a) > 1 and b * abs(a).bit_length() > 30000:
                continue
            cases.append((f"power {a} {b}", power_expected(a, b), "exact"))
    for a in [0, 1, -1, 2, -2, 10, LIMB]:
        for b in [-1, -2, -3, -63, -64, -100]:
            cases.append((f"power {a} {b}", power_expected(a, b), "exact"))
    # AN EXPONENT OF MORE THAN ONE LIMB, which only |base| <= 1 can survive: it
    # walks the whole squaring loop over two limbs and must still answer.
    for b in [LIMB, LIMB + 5, 2 ** 128 + 1]:
        for a in [0, 1, -1]:
            cases.append((f"power {a} {b}", power_expected(a, b), "exact"))
        for a in [1, -1]:
            cases.append((f"power {a} {-b}", power_expected(a, -b), "exact"))


# base 2 and base 16: the spelling Python gives, and the harness also reads every
# one of them back, so each case is a round trip as well as a comparison.
def radix_cases(values):
    for a in values:
        for base, spelling in ((2, "b"), (16, "X")):
            cases.append((f"radix {a} {base}",
                          ("-" if a < 0 else "") + format(abs(a), spelling), "exact"))


def knuth_add_back_cases(rng, count):
    # With divisor limbs (v2, v1, v0 = 2^64 - 1), v2 >= 2^63, and dividend
    # (q + 1) * (v2 * 2^64 + v1) * 2^64, the two-limb estimate is exactly q + 1 and
    # passes the third-limb test, but (q + 1) * v is larger than the dividend by
    # (q + 1) * v0: the step that adds the divisor back. Checked here in Python.
    # Limbs appended below the dividend leave that top step unchanged.
    made = []
    for _ in range(count):
        v2, v1, q = rng.randrange(2 ** 63, LIMB), rng.randrange(LIMB), rng.randrange(1, LIMB - 1)
        v = (v2 * LIMB + v1) * LIMB + (LIMB - 1)
        u = (q + 1) * (v2 * LIMB + v1) * LIMB
        assert u // v == q and (q + 1) * v > u
        below = rng.randint(0, 3)
        made.append(((u << (64 * below)) + rng.getrandbits(64 * below) if below else u, v))
    return made


def build_cases():
    special = [0, 1, 2 ** 32, 2 ** 63 - 1, 2 ** 63, LIMB - 1, 10 ** 19 - 1, 10 ** 19, 10 ** 20, 10 ** 27 - 1]
    for k in range(1, 9):
        special += [2 ** (64 * k) - 1, 2 ** (64 * k), 2 ** (64 * k) + 1]
    special += [2 ** 640 - 1, 2 ** 640, 2 ** 640 + 2 ** 320 - 1, (2 ** 640 - 1) // (LIMB - 1)]  # all-ones, 1-limb pattern
    rng = random.Random(64)
    special.append(int("".join(rng.choice("123456789") + "".join(rng.choice("0123456789") for _ in range(9999)))))
    signed = sorted(set(special + [-n for n in special]))
    for a in signed:
        unary(a)
        for b in signed:
            binary(a, b)

    def random_number():
        roll = rng.random()
        if roll < 0.25:
            n = rng.randrange(2 ** 32)
        elif roll < 0.5:
            n = rng.randrange(LIMB)
        elif roll < 0.6:
            n = rng.choice([LIMB, 2 ** 128, 2 ** 192]) + rng.randint(-3, 3)
        elif roll < 0.7:  # long runs of all-ones and all-zero limbs: carries and borrows
            n = sum(rng.choice([0, LIMB - 1, 1]) << (64 * i) for i in range(rng.randint(2, 12)))
        else:
            n = rng.getrandbits(64 * rng.choice([2, 2, 3, 4, 6, 9, 16, 40]))
        return -n if rng.random() < 0.5 else n

    for _ in range(50000):
        a, b = random_number(), random_number()
        if rng.random() < 0.1 and b != 0:  # dividends close to a multiple of the divisor
            a = b * rng.getrandbits(64 * rng.randint(1, 4)) + rng.randint(-2, 2)
        binary(a, b)
        if rng.random() < 0.2:
            unary(a)
    for u, v in knuth_add_back_cases(rng, 400):
        binary(u, v)
        binary(-u, v)
    # digits() counts from the bit length and checks against powers of ten: every
    # side of every power of ten from 10^18 to 10^399, and three far larger ones.
    for k in list(range(18, 400)) + [1000, 4000, 10000]:
        for n in (10 ** k - 1, 10 ** k, 10 ** k + 1):
            for signed_n in (n, -n):
                cases.append((f"digits {signed_n}", str(len(str(n))), "exact"))

    hostile = [b"", b"-", b"+1", b" 1", b"1 ", b"1e5", b"1.5", b"0x10", "\u0661\u0662".encode(), b"1\x002",
               b"--1", b"-0", b"-000", b"007", b"0", b"-a", b"1,000", b"1_000", b"\t1", b"1\n", "\uff11".encode(),
               "\u22121".encode(), b"9" * 20, b"-" + b"9" * 40, b"0" * 50 + b"1", b"-" + b"0" * 30, b"12a", b"\x00"]
    for raw in hostile:
        cases.append((f"text :{raw.hex()}", text_answer(raw), "exact"))
    power_cases()

    # radix: the same edge values the rest of the file uses, plus bit lengths
    # that are NOT a multiple of 4, which is where a hex top digit goes wrong.
    radix_values = sorted(set(signed + [2 ** k + rng.randrange(2 ** k) for k in (1, 3, 5, 7, 13, 61, 62, 63, 65, 127, 129, 255)]))
    radix_values += [-n for n in radix_values if n > 0]
    for _ in range(2000):
        radix_values.append(random_number())
    radix_cases(radix_values)

    # Hostile text at a radix, as `text` does for decimal. b and x prefixes are
    # NOT accepted here on purpose: the lexer strips them before this ever sees
    # the digits, so a prefix reaching from_radix_text means something is wrong.
    radix_hostile = [(b"", 2), (b"-", 2), (b"", 16), (b"-", 16), (b"2", 2), (b"12", 2), (b"-2", 2),
                     (b"g", 16), (b"G", 16), (b"0x10", 16), (b"b1100", 2), (b"xFF", 16), (b" 1", 2),
                     (b"1 ", 2), (b"1\x000", 2), (b"-0", 2), (b"-0", 16), (b"000", 2), (b"0", 16),
                     (b"ff", 16), (b"FF", 16), (b"fF", 16), (b"+1", 2), (b"--1", 16), (b"1.0", 16)]
    for raw, base in radix_hostile:
        text = raw.decode("latin-1")
        digits = "0123456789ABCDEF"[:base]
        body = text[1:] if text[:1] == "-" else text
        if body and all(c.upper() in digits for c in body):
            value = int(body, base) * (-1 if text[:1] == "-" else 1)
            expected = "ok " + str(value) + (" negative" if value < 0 else "")
        else:
            bad = 1 if text[:1] == "-" else 0
            while bad < len(text) and text[bad].upper() in digits:
                bad += 1
            expected = f"3 {bad}"
        cases.append((f"fromradix :{raw.hex()} {base}", expected, "exact"))
    # A radix this does not handle answers nothing at all -- base 10 included,
    # because decimal has its own to_text() and does not come through here.
    for base in (0, 1, 8, 10, 32):
        cases.append((f"radix 255 {base}", "empty", "exact"))

    for value in [-2 ** 63, -2 ** 63 + 1, -1, 0, 1, 2 ** 63 - 1]:
        cases.append((f"signed {value}", str(value), "exact"))
    for magnitude in [0, 1, LIMB - 1]:
        for negative in (0, 1):
            value = -magnitude if negative else magnitude
            cases.append((f"make {magnitude} {negative}", str(value) + (" negative" if value < 0 else ""), "exact"))


# A harness that hangs must fail the check, not hang it (a broken multiply once made
# digits() loop for ever). The limits are the referee's, not satellite's: about ten
# times the longest run measured (7 s for the cases, 88 s for the million digits).
def run(lines, seconds_allowed):
    started = time.time()
    try:
        done = subprocess.run([HARNESS], input="\n".join(lines) + "\n", capture_output=True, text=True, check=True,
                              timeout=seconds_allowed)
    except subprocess.TimeoutExpired:
        print(f"FAIL: the harness ran longer than {seconds_allowed} s")
        sys.exit(1)
    except subprocess.CalledProcessError as failed:
        print(f"FAIL: the harness stopped with exit status {failed.returncode}: {failed.stderr.strip()[-200:]}")
        sys.exit(1)
    return done.stdout.split("\n")[:-1], time.time() - started


def main():
    build_cases()
    answers, seconds = run([case[0] for case in cases], 120)
    if len(answers) != len(cases):
        print(f"FAIL: {len(cases)} cases but {len(answers)} answers")
        return 1
    failures, allocation_free = [], 0
    for case, got in zip(cases, answers):
        line, expected, kind = case[0], case[1], case[2]
        if kind == "arithmetic":
            value, allocations = got.rsplit(" ", 1) if " " in got else (got, "?")
            a, b = case[3], case[4]
            if value != str(expected) or not allocations.isdigit():
                failures.append((line, str(expected), got))
            elif fits(a) and fits(b) and fits(expected):
                allocation_free += 1
                if allocations != "0":
                    failures.append((line, f"{expected} 0 (no allocation)", got))
        elif got != expected:
            failures.append((line, expected, got))
    for line, expected, got in failures[:10]:
        print(f"FAIL: {line[:120]}\n  expected {expected[:120]}\n  got      {got[:120]}")
    print(f"{len(cases):,} cases, {len(failures)} failed, in {seconds:.2f} s; "
          f"{allocation_free:,} one-limb + - * checked to make no allocation")

    if QUICK:
        return 1 if failures else 0
    # A million digits: text in, text out, digits, and arithmetic on it.
    rng = random.Random(1000000)
    digits = rng.choice("123456789") + "".join(rng.choice("0123456789") for _ in range(999999))
    big = int(digits)
    answers, seconds = run([f"timed_text -{digits}", f"add {digits} 1", f"divide {digits} {LIMB + 7}",
                            f"text :{('-000' + digits).encode().hex()}"], 900)
    text, from_ns, to_ns, digits_ns, count = answers[0].split(" ")
    q, r = trunc_divide(big, LIMB + 7)
    big_failures = [text != f"-{digits}", count != "1000000", answers[1].split(" ")[0] != str(big + 1),
                    answers[2] != f"{q} {r}", answers[3] != f"ok -{digits} negative"]
    print(f"1,000,000 digits: from_text {int(from_ns) / 1e9:.3f} s, to_text {int(to_ns) / 1e9:.3f} s, "
          f"digits() {int(digits_ns) / 1e9:.3f} s; whole run of 4 cases {seconds:.2f} s; "
          f"{sum(big_failures)} failed")
    return 1 if failures or any(big_failures) else 0


if __name__ == "__main__":
    sys.exit(main())
