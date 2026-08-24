// .size() and .digits() (§8.7): what a value costs in the model's bytes, on a
// scalar, on a container that shares storage, and on a spacesuit instance that
// can close a cycle. Part of the eval_test binary; the harness these call is
// declared in eval_test.hpp.

#include "eval_test.hpp"

#include <string>

void eval_test_size_and_digits()
{
    // --- .size() and .digits(), §8.7 ---------------------------------------
    //
    // Every number here is the MODEL's, not the allocator's, and that is what
    // makes them assertable at all: 40 per node, 16 per handle, 2 per
    // character, 4 per limb. §8.7 froze those as the definition precisely so
    // that a test could pin them.
    {
        // A node and nothing else. bool and time own no storage.
        check_output("satellite.bool.true.size()\n", "40\n", "bool is a bare node");

        // 40 + 3 characters x 2 (§8.5). The gap between this and .length() is
        // the whole reason both exist.
        check_output("\"abc\".length()\n", "3\n", "length counts characters");
        check_output("\"abc\".size()\n", "46\n", "size counts bytes");

        // \home is ONE satellite character however long the path is, so it
        // costs one character's worth of bytes. A .size() that measured
        // decoded text would report the home directory's length here.
        check_output("\"hi\\home!\".size()\n", "48\n",
                     "\\home is one character, and costs two bytes");

        // The small form owns nothing; a boxed magnitude costs 4 per limb, and
        // base 10^9 packs nine digits into each, so 40 digits is 5 limbs.
        check_output("(1).size()\n", "40\n", "a small number is a bare node");
        check_output("satellite.random.ultra(40).size()\n", "60\n",
                     "40 digits is 5 limbs of 4 bytes");

        // Node, plus a handle and a node per element.
        check_output("satellite.container.list<satellite.variable.number> l\n"
                     "l.append(1)\n"
                     "l.append(2)\n"
                     "l.append(3)\n"
                     "l.size()\n",
                     "208\n", "list is 40 + 3 x (16 + 40)");

        // The point of the visited set: one sublist held twice is one sublist.
        // 40 + 2 handles + inner counted ONCE (152) = 224, not 40 + 32 + 304.
        check_output("satellite.container.list<satellite.variable.number> inner\n"
                     "inner.append(1)\n"
                     "inner.append(2)\n"
                     "inner.size()\n"
                     "satellite.container.list<satellite.container.list"
                     "<satellite.variable.number>> outer\n"
                     "outer.append(inner)\n"
                     "outer.append(inner)\n"
                     "outer.size()\n",
                     "152\n224\n", "shared storage counts once");

        // .digits() counts the digits the value is WRITTEN with, which is the
        // question a caller asking "how many digits" is asking. Storage
        // normalizes 100 to 1e2 -- trailing zeros are representation, not
        // value -- and these pin that the normalization stays invisible here.
        check_output("(123).digits()\n", "3\n", "digits counts digits");
        check_output("(100).digits()\n", "3\n", "100 has three digits");
        check_output("(0).digits()\n", "1\n", "zero is one digit");
        check_output("(1230).digits()\n", "4\n", "a trailing zero is a digit");
        check_output("(1000000000000).digits()\n", "13\n",
                     "a trailing run past the small form is counted too");

        // The fractional side. A digit after the point is still a digit, but
        // the zeros in 0.001 sit BEFORE the first significant one and are
        // padding, so they are not counted.
        check_output("(12.5).digits()\n", "3\n", "a fractional digit counts");
        check_output("(0.001).digits()\n", "1\n", "a leading zero is padding");
        check_output("(0.00105).digits()\n", "3\n",
                     "counting starts at the first significant digit");
        check_output("(0).minus(100).digits()\n", "3\n",
                     "the sign is not a digit");

        // The invariant that pins the rule for a value nobody chose: for a
        // whole number, .digits() is the length of what it prints. Under the
        // old significand rule this FAILED about one draw in ten, because a
        // draw ending in 0 normalized to 39 digits and an exponent while still
        // printing 40 characters. (`.digits() == 40` is still not the test to
        // write: a draw below 10^39 genuinely has 39 digits.)
        check_output("satellite.variable.number n = satellite.random.ultra(40)\n"
                     "n.digits().minus(n.to_string().length())\n",
                     "0\n", "a whole number's digits is what it prints");

        // A number REFUSES length() rather than giving the word a second
        // meaning, and the refusal names what the caller probably wanted.
        check_error("(5).length()\n", "no method length",
                    "a number has no length()");
        check_error("(5).length()\n", ".digits()",
                    "the refusal names .digits()");

        check_error("\"abc\".size(1)\n", "takes 0 arguments",
                    "size takes no arguments");
    }
}

void eval_test_spacesuit_size()
{
    // --- .size() on a spacesuit instance, §8.7 -----------------------------
    {
        // 40 node + 2 fields x 16 handles + a number node + a string node with
        // three characters = 40 + 32 + 40 + 46.
        check_output("satellite.spacesuit point()\n"
                     "{\n"
                     "    satellite.protected\n"
                     "    {\n"
                     "        satellite.variable.number x = 1\n"
                     "        satellite.variable.string label = \"abc\"\n"
                     "    }\n"
                     "    satellite.public\n"
                     "    {\n"
                     "    }\n"
                     "}\n"
                     "point p\n"
                     "p.size()\n",
                     "158\n", "an instance is billed for its fields");

        // §14's rule, unchanged: the suit answers first. A spacesuit that
        // already defines size() keeps its own, which is what makes adding
        // .size() to the language safe for every program that compiles today.
        check_output("satellite.spacesuit boxed()\n"
                     "{\n"
                     "    satellite.protected\n"
                     "    {\n"
                     "        satellite.variable.number v = 9\n"
                     "    }\n"
                     "    satellite.public\n"
                     "    {\n"
                     "        satellite.capsule size() "
                     "satellite.returns(satellite.variable.string)\n"
                     "        {\n"
                     "            satellite.return(\"mine\")\n"
                     "        }\n"
                     "    }\n"
                     "}\n"
                     "boxed z\n"
                     "z.size()\n",
                     "mine\n", "a spacesuit's own size() wins");

        // §12 says an object can close a cycle. This test is here to prove the
        // walk TERMINATES; the number is the honest consequence of counting
        // each piece once, and §8.7 spells out why it shrinks: the nil that
        // `next` held was a real 40-byte node, and the self-reference is
        // storage already counted.
        check_output("satellite.spacesuit node()\n"
                     "{\n"
                     "    satellite.protected\n"
                     "    {\n"
                     "        node next\n"
                     "    }\n"
                     "    satellite.public\n"
                     "    {\n"
                     "        satellite.capsule link(node n) "
                     "satellite.returns(satellite.variable.number)\n"
                     "        {\n"
                     "            next = n\n"
                     "            satellite.return(0)\n"
                     "        }\n"
                     "    }\n"
                     "}\n"
                     "node a\n"
                     "a.size()\n"
                     "a.link(a)\n"
                     "a.size()\n",
                     "96\n0\n56\n", "a cyclic object terminates");
    }
}
