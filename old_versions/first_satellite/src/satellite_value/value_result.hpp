#pragma once

#include "satellite_value/value_types.hpp"

namespace satellite {

// satellite.container.result — what Satellite Orbit hands back, and the whole
// state of the search that made it.
//
// A MAP UNDERNEATH AND ITS OWN THING TO A READER, which is exactly what
// `arguments` is one level over: an Arguments is a list<string> to matches()
// and satellite.container.arguments to anyone reading an error, with a method
// table a list does not have. A result is a MAP to matches(), so it type-checks,
// prints and is SEARCHED as one — by the very power that produced it, with no
// new code — and it answers .weight(), .attention(), .phases() and
// .alternatives(), which a map does not.
//
// It exists because a result could not be BOUND. Measured against ./satl before
// this type was built: the fields are heterogeneous — a list, a nil, a number, a
// bool, a string — so `list<map<string, string>> found = flags.orbit(want)` is
// refused and there is no spelling of the type that is not, which left results
// reachable only by chaining `flags.orbit(want)[0]["why"]`.
// satellite_orbit_search/result_type.txt is the design and the reasons.
//
// ---------------------------------------------------------------------------
// ATTENTION IS COUNTED. WEIGHT IS JUDGED. That split is the whole point of the
// type and it is not a naming choice:
//
//     attention   what is literally, checkably true about this result in the
//                 data in front of us. A COUNT of sightings. If every phase's
//                 opinion were deleted, attention would still be computable
//                 from the corpus alone.
//     weight      what orbit THINKS this result is, 0..100. The five phases'
//                 judgement: tightness, how many of them agreed, what memory
//                 says, and whether the engine proposed it rather than found it.
//
// Carrying BOTH rather than one blended number is the design, because THE TWO
// DISAGREEING IS THE USEFUL SIGNAL:
//
//     high attention, low weight   it is definitely in there, a lot, and orbit
//                                  does not think it is what you meant
//     high weight, low attention   orbit thinks this is the answer and there is
//                                  almost nothing concrete under it
//
// The second line is the dangerous one, and one `confidence` number cannot
// express it: a constructed neighbour three phases liked and a direct hit can
// land on the same number by different routes with nothing in the answer saying
// which happened. Splitting it is what makes "orbit is speculating" a thing a
// program can TEST FOR rather than a thing a person has to infer from `why`.
//
// They are deliberately not the same KIND of number — a raw count against a
// percentage — so that nobody can add them, average them, or sort on their sum.
// ---------------------------------------------------------------------------
//
// BUILT, THEN FROZEN, exactly as MapBody and Arguments are and for the same
// reason: a ResultBody is fully populated before make_shared and never written
// afterwards, which is what keeps the immutability contract and the lock-free
// publish protocol intact.
struct ResultBody {
    // The whole state, in order, as an ordinary map Value. NEVER NULL, and
    // always a map — orbit's builder is the only thing that makes one.
    //
    // THE ONE STORAGE. Every field is reached through here and there is no
    // second copy of any of it, so `r.why()` and `r["why"]` cannot disagree and
    // a field added to the builder is a method the same day. A struct with
    // sixteen typed members beside a map that repeated them would be two
    // sources of truth for one answer, which is the drift this whole tree is
    // arranged to avoid.
    ValuePtr fields;

    // Every result of the SAME resolution, ranked best first — including this
    // one, at `rank`. .alternatives() is this list with `rank` left out.
    //
    // The results in here are BARE: their own `ranked` is null, so an
    // alternative has no alternatives of its own. That bound is not laziness.
    // A result whose alternatives carried alternatives would have to point back
    // at results that point at it, which is a shared_ptr cycle — the same leak
    // value.hpp calls the honest cost of an Object, and the same reason the
    // search walker treats a spacesuit as a leaf. One level, decided here, is
    // cheaper than a hang discovered later.
    //
    // Null on a bare result and on a result built outside a resolution.
    ListRef ranked;

    // Where this result placed, 0 first. The index into `ranked` that
    // .alternatives() skips, and the `rank` field's own number.
    size_t rank = 0;
};

// Sixteen bytes, and the variant already holds five handles, so appending this
// alternative leaves sizeof(Value) at 40 — see the static_assert in
// value_layout.hpp and the note in value_variant.hpp about why that number is
// load-bearing.
using ResultRef = std::shared_ptr<const ResultBody>;

} // namespace satellite
