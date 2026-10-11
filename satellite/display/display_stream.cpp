// satellite/display/display_stream.cpp -- std::cout, written into the printing satellite.
//
// EVERYTHING satl ITSELF WRITES WITH std::cout -- a styled display, the prompt's words, a report's
// flush, satellite.access, a listing -- keeps its 8 KB here, as std::cout always did, and goes to
// the printing satellite as ONE piece when the 8 KB fills or something flushes: never a hand-off
// a line. The word libraries share satl's libstdc++ and its std::cout (make_support/048-link.mk),
// so what they write comes here too. printing_satellite.hpp says why one door keeps the order.

#include "printing_satellite.hpp"

#include "../machine/machine_codes.hpp"

#include <cstring>
#include <iostream>
#include <streambuf>
#include <string>

namespace satellite004 {
namespace {

constexpr std::size_t kStreamBytes = 8 * 1024;

class DisplayStream final : public std::streambuf {
public:
    DisplayStream()
    {
        setp(nullptr, nullptr);
        room();
    }

    // The 8 KB, as they stand, to the printing satellite. The string handed back is an old slot's,
    // and its room is used again.
    void hand_over()
    {
        const std::size_t used = static_cast<std::size_t>(pptr() - pbase());
        if (used == 0)
            return;
        buffer_.resize(used);
        const signed long long int said = display_bytes(std::move(buffer_));
        // THE PUT AREA POINTED INTO THE STRING THAT IS NOW A SLOT'S: nowhere, before anything that
        // can throw, so a failed allocation in room() leaves std::cout with no room -- the next
        // write asks again -- and never with pointers into memory it gave away.
        setp(nullptr, nullptr);
        if (said != success)
            refused_ = true;
        room();
    }

protected:
    int_type overflow(int_type c) override
    {
        hand_over();
        if (pbase() == nullptr)
            room();                          // an earlier room() found no memory: ask again
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        }
        return said_refused() ? traits_type::eof() : traits_type::not_eof(c);
    }

    std::streamsize xsputn(const char *bytes, std::streamsize size) override
    {
        if (size <= 0)
            return 0;
        const std::size_t count = static_cast<std::size_t>(size);
        if (count > static_cast<std::size_t>(epptr() - pptr())) {
            hand_over();
            // LONGER THAN THE 8 KB: handed over whole, after what came before it.
            if (count >= kStreamBytes) {
                if (display_bytes(std::string(bytes, count)) != success)
                    refused_ = true;
                return said_refused() ? 0 : size;
            }
            if (pbase() == nullptr)
                room();
        }
        std::memcpy(pptr(), bytes, count);
        pbump(static_cast<int>(count));
        return size;
    }

    // A FLUSH WAITS until everything before it is on the screen -- what every std::cout.flush() in
    // satl already meant, and what keeps a report after the lines it explains.
    int sync() override
    {
        hand_over();
        const signed long long int drained = display_drain();
        return (drained == success && !said_refused()) ? 0 : -1;
    }

private:
    void room()
    {
        if (buffer_.capacity() > 4 * kStreamBytes)
            std::string().swap(buffer_);   // a huge write's string is not kept for the rest of the run
        buffer_.clear();
        if (buffer_.capacity() < kStreamBytes)
            buffer_.reserve(kStreamBytes);
        buffer_.resize(kStreamBytes);
        setp(buffer_.data(), buffer_.data() + buffer_.size());
    }

    // A refusal the printing satellite answered, said ONCE as std::cout's failure -- its badbit
    // then says it until somebody clears it, as a refused write always did.
    bool said_refused()
    {
        const bool was = refused_;
        refused_ = false;
        return was;
    }

    std::string buffer_;
    bool refused_ = false;
};

// NEVER DESTROYED: std::cout is flushed after main() returns, through this.
DisplayStream *the_stream = nullptr;

} // namespace

void std_cout_goes_to_the_printing_satellite()
{
    if (the_stream == nullptr)
        the_stream = new DisplayStream;
    std::cout.rdbuf(the_stream);
}

void hand_over_what_std_cout_holds()
{
    if (the_stream != nullptr)
        the_stream->hand_over();
}

} // namespace satellite004
