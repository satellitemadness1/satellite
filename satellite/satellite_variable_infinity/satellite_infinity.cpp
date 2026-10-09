// satellite/satellite_variable_infinity/satellite_infinity.cpp -- the count, the
// order and the display of satellite.variable.infinity (SATELLITE_INFINITY.md,
// INF-2). The header says what a value is; this file is the three walks over one.
//
// WHY THE WALKS ARE LOOPS OVER A std::vector AND NOT FUNCTIONS CALLING THEMSELVES:
// an exponent is a value, and a program can nest them as deep as memory allows
// (Part 4: 100,000 passes of `x = inf.power_of(x)` must print, not crash). A C++
// call per level spends the thread's stack, which is a few megabytes and a bound
// nobody chose; a std::vector spends the heap, which is the machine's limit and
// the only one satellite admits to.

#include "satellite_infinity.hpp"

#include "../machine/machine_codes.hpp"

#include <utility>

namespace satellite004 {
namespace {

satellite_number ten_to(std::size_t places)
{
    satellite_number out;
    satellite_number::power(satellite_number(10ull), satellite_number(static_cast<unsigned long long int>(places)), out);
    return out;
}

bool magnitude_is_one(const infinity_count &count)
{
    return count.places == 0 && count.whole == satellite_number(1ull);
}

// |left| against |right|. The wholes first; then the fractions, the shorter one
// brought up to the longer one's places -- 0.5 is 0.50, and 50 against 25 is the
// same answer as 5 against 25 would not be.
int compare_magnitude(const infinity_count &left, const infinity_count &right)
{
    const int wholes = satellite_number::compare(left.whole, right.whole);
    if (wholes != 0 || left.places == right.places)
        return wholes != 0 ? wholes : satellite_number::compare(left.fraction, right.fraction);
    if (left.places < right.places)
        return satellite_number::compare(left.fraction * ten_to(right.places - left.places), right.fraction);
    return satellite_number::compare(left.fraction, right.fraction * ten_to(left.places - right.places));
}

} // namespace

// ---------------------------------------------------------------------------
// THE COUNT
// ---------------------------------------------------------------------------

infinity_count infinity_count::of_whole(const satellite_number &value)
{
    infinity_count count;
    count.negative = value.negative();
    count.whole = value.negative() ? -value : value;
    return count;
}

signed long long int infinity_count::from_text(const std::string &text, infinity_count &out)
{
    const bool minus = !text.empty() && text[0] == '-';
    const std::string digits = minus ? text.substr(1) : text;
    const std::size_t point = digits.find('.');
    const std::string whole_part = digits.substr(0, point);
    std::string fraction_part = point == std::string::npos ? std::string() : digits.substr(point + 1);
    if (whole_part.empty() || (point != std::string::npos && fraction_part.empty()))
        return int_error;
    for (const char c : whole_part + fraction_part)
        if (c < '0' || c > '9')
            return int_error;

    infinity_count read;
    std::size_t bad_offset = 0;
    if (satellite_number::from_text(whole_part, read.whole, bad_offset) != success)
        return int_error;
    while (!fraction_part.empty() && fraction_part.back() == '0')   // 0.50 is 0.5: one spelling
        fraction_part.pop_back();
    read.places = fraction_part.size();
    if (!fraction_part.empty() && satellite_number::from_text(fraction_part, read.fraction, bad_offset) != success)
        return int_error;
    read.negative = minus && !read.is_zero();                          // -0 is 0
    out = std::move(read);
    return success;
}

infinity_count infinity_count::negated() const
{
    infinity_count turned = *this;
    turned.negative = !negative && !is_zero();
    return turned;
}

std::string infinity_count::magnitude_text() const
{
    std::string written = whole.to_text();
    if (places == 0)
        return written;
    std::string after = fraction.to_text();
    if (after.size() < places)
        after.insert(0, places - after.size(), '0');                   // 12.05: the 0 before the 5
    return written + "." + after;
}

std::string infinity_count::text() const
{
    return negative ? "-" + magnitude_text() : magnitude_text();
}

int infinity_count::compare(const infinity_count &left, const infinity_count &right)
{
    if (left.sign() != right.sign())
        return left.sign() < right.sign() ? -1 : 1;
    const int sizes = compare_magnitude(left, right);
    return left.negative ? -sizes : sizes;
}

// ---------------------------------------------------------------------------
// THE VALUE
// ---------------------------------------------------------------------------

// A CHAIN OF EXPONENTS IS LET GO ONE LINK AT A TIME. The default destructor would
// destroy an exponent from inside its owner's destructor, and that one's exponent
// from inside that, one C++ frame a level -- so a tower 100,000 deep would take the
// thread's stack with it on the way out. Instead every exponent this value is the
// LAST owner of is moved into a list here, and each one's own exponents are moved
// out of it before it goes, so no destructor ever finds anything deep below it.
//
// The const_cast is sound: every value is made by make() through
// std::make_shared<satellite_infinity>, so no object here was ever created const.
satellite_infinity::~satellite_infinity()
{
    std::vector<InfinityHandle> last_owned;
    for (infinity_term &term : terms)
        if (term.exponent != nullptr && term.exponent.use_count() == 1)
            last_owned.push_back(std::move(term.exponent));
    while (!last_owned.empty()) {
        const InfinityHandle going = std::move(last_owned.back());
        last_owned.pop_back();
        for (infinity_term &term : const_cast<satellite_infinity &>(*going).terms)
            if (term.exponent != nullptr && term.exponent.use_count() == 1)
                last_owned.push_back(std::move(term.exponent));
    }                                                  // `going` ends here, with nothing under it
}

InfinityHandle satellite_infinity::make(std::vector<infinity_term> terms, satellite_number nines_width)
{
    if (terms.empty())
        return nullptr;
    std::shared_ptr<satellite_infinity> made = std::make_shared<satellite_infinity>();
    made->terms = std::move(terms);
    made->nines_width = std::move(nines_width);
    // infinity-0 is infinity^1; infinity-k is infinity^(infinity-(k-1)). The
    // exponent already knows whether it is one, so this never walks.
    if (made->terms.size() == 1 && made->terms[0].exponent != nullptr && made->terms[0].count.is_one()) {
        const satellite_infinity &exponent = *made->terms[0].exponent;
        if (exponent.is_one()) {
            made->a_unit = true;
        } else if (exponent.a_unit) {
            made->a_unit = true;
            made->unit_rank = exponent.unit_rank + satellite_number(1ull);
        }
    }
    return made;
}

const InfinityHandle &satellite_infinity::one()
{
    static const InfinityHandle value = of_number(satellite_number(1ull));
    return value;
}

InfinityHandle satellite_infinity::infinity(const satellite_number &nines_width)
{
    return make({infinity_term{one(), infinity_count::of_whole(satellite_number(1ull))}}, nines_width);
}

InfinityHandle satellite_infinity::of_number(const satellite_number &value)
{
    if (value.is_zero())
        return nullptr;
    return make({infinity_term{nullptr, infinity_count::of_whole(value)}}, satellite_number());
}

InfinityHandle satellite_infinity::negated(const satellite_infinity *value)
{
    if (value == nullptr)
        return nullptr;
    std::vector<infinity_term> turned = value->terms;
    for (infinity_term &term : turned)
        term.count = term.count.negated();
    return make(std::move(turned), value->nines_width);
}

bool satellite_infinity::is_plain() const
{
    for (const infinity_term &term : terms)
        if (term.exponent != nullptr)
            return false;
    return true;
}

bool satellite_infinity::is_one() const
{
    return terms.size() == 1 && terms[0].exponent == nullptr && terms[0].count.is_one();
}

// THE ORDER IS THE SIGN OF THE FIRST TERM OF left - right, found without making
// left - right. Both lists are largest first, so they are walked side by side: where
// the exponents differ, the larger one's term leads the difference and its count's
// sign is the answer (turned over when it is right's); where they are equal and the
// counts differ, the counts decide; where both are equal, the terms cancel and the
// walk moves on. Both lists move together, so one position serves both.
//
// COMPARING TWO EXPONENTS IS THE SAME QUESTION ONE LEVEL DOWN, and that is the
// stack: a walk pushes the walk of its two exponents and waits for `order` to come
// back up. Two short cuts, both exact: one node against itself is equal without
// looking (exponents are shared, never copied), and two canonical infinity-k are
// ordered by their k.
int satellite_infinity::compare(const satellite_infinity *left, const satellite_infinity *right)
{
    struct Walk { const satellite_infinity *left; const satellite_infinity *right; std::size_t at; };
    std::vector<Walk> stack;
    stack.push_back({left, right, 0});
    int order = 0;
    bool came_back = false;              // `order` is the answer of the walk just finished
    while (!stack.empty()) {
        Walk &walk = stack.back();
        if (came_back) {                 // how the two exponents at walk.at compare
            came_back = false;
            const infinity_count &mine = walk.left->terms[walk.at].count;
            const infinity_count &theirs = walk.right->terms[walk.at].count;
            const int here = order > 0 ? mine.sign() : order < 0 ? -theirs.sign() : infinity_count::compare(mine, theirs);
            if (here != 0) {
                order = here;
                came_back = true;
                stack.pop_back();
                continue;
            }
            ++walk.at;
        } else if (walk.at == 0 && (walk.left == walk.right ||
                                    (walk.left != nullptr && walk.right != nullptr && walk.left->a_unit &&
                                     walk.right->a_unit))) {
            order = walk.left == walk.right ? 0 : satellite_number::compare(walk.left->unit_rank, walk.right->unit_rank);
            came_back = true;
            stack.pop_back();
            continue;
        }
        const std::size_t left_size = walk.left == nullptr ? 0 : walk.left->terms.size();
        const std::size_t right_size = walk.right == nullptr ? 0 : walk.right->terms.size();
        if (walk.at >= left_size || walk.at >= right_size) {   // what is left over leads
            order = walk.at < left_size    ? walk.left->terms[walk.at].count.sign()
                    : walk.at < right_size ? -walk.right->terms[walk.at].count.sign()
                                           : 0;
            came_back = true;
            stack.pop_back();
            continue;
        }
        const satellite_infinity *left_exponent = walk.left->terms[walk.at].exponent.get();
        const satellite_infinity *right_exponent = walk.right->terms[walk.at].exponent.get();
        if (left_exponent == right_exponent) {                 // shared, or both plain
            order = 0;
            came_back = true;
            continue;
        }
        stack.push_back({left_exponent, right_exponent, 0});   // `walk` is not read after this
    }
    return order;
}

// THE SAME ORDER AGAINST A PLAIN NUMBER, WITHOUT MAKING ONE -- `while (count < inf)`
// asks it every pass (Q27), and building the number as a value first cost an
// allocation a pass: 190 ns of a loop's time, measured. The number is one term with
// the exponent 0, so the first term of left - right is decided by the SIGN of left's
// first exponent: above 0, left's term leads; below 0 (INF-8's infinitely small),
// the number's does, unless it is 0; at 0, the counts meet, and on a tie the next
// term of left -- whose exponent can only be below 0 -- decides.
int satellite_infinity::compare(const satellite_infinity *left, const satellite_number &right)
{
    const int right_sign = right.is_zero() ? 0 : right.negative() ? -1 : 1;
    if (left == nullptr || left->terms.empty())
        return -right_sign;
    const infinity_term &first = left->terms[0];
    const satellite_infinity *exponent = first.exponent.get();
    const int exponent_sign = exponent == nullptr || exponent->terms.empty() ? 0 : exponent->terms[0].count.sign();
    if (exponent_sign > 0)
        return first.count.sign();
    if (exponent_sign < 0)
        return right_sign != 0 ? -right_sign : first.count.sign();
    const int counts = infinity_count::compare(first.count, infinity_count::of_whole(right));
    if (counts != 0 || left->terms.size() == 1)
        return counts;
    return left->terms[1].count.sign();
}

// ONE SET OF PARENTHESES A NUMBER, and an exponent in the family is its own set --
// the stack holds the numbers still open, and a `)` is written as each one ends.
// The first term shows its sign only when it is negative; every term after it shows
// `+` or `-`; a count of 1 in front of an infinity is not printed (Q36), and a plain
// number prints whatever it is, 1 included.
std::string satellite_infinity::display(const satellite_infinity *value)
{
    if (value == nullptr || value->terms.empty())
        return "0";
    if (value->is_plain())                             // the one term, bare
        return value->terms[0].count.text();

    struct Open { const satellite_infinity *value; std::size_t at; };
    std::vector<Open> stack;
    std::string out = "(";
    stack.push_back({value, 0});
    while (!stack.empty()) {
        Open &open = stack.back();
        if (open.at == open.value->terms.size()) {
            out += ')';
            stack.pop_back();
            continue;
        }
        const infinity_term &term = open.value->terms[open.at];
        const bool first = open.at == 0;
        ++open.at;                                     // before any push, which may move `open`
        if (!first)
            out += ", ";
        if (term.count.negative)
            out += '-';
        else if (!first)
            out += '+';
        const satellite_infinity *exponent = term.exponent.get();
        if (exponent == nullptr) {                     // a plain number, attached
            out += term.count.magnitude_text();
            continue;
        }
        if (!magnitude_is_one(term.count))
            out += term.count.magnitude_text() + " ";
        if (exponent->is_one()) {
            out += "infinity";
        } else if (exponent->a_unit) {                 // infinity ** infinity-k is infinity-(k+1)
            out += "infinity-" + (exponent->unit_rank + satellite_number(1ull)).to_text();
        } else if (exponent->is_plain()) {             // between the rungs, the exponent is shown (Q5)
            out += "infinity^" + exponent->terms[0].count.text();
        } else {
            out += "infinity^(";
            stack.push_back({exponent, 0});
        }
    }
    return out;
}

} // namespace satellite004
