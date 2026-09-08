// The shared checks, the module rows under `satellite.file` `1 8`, and
// `satellite.system.delete` `1 22 1`. See satellite_file/handlers.hpp for why
// the third of those is installed from here.

#include "satellite_file/handlers.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "error_reporter/report.hpp"
#include "satellite_file/file_internal.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <cerrno>
#include <cstring>
#include <string>

namespace satellite::file {

std::string asked(eval::Machine &m)
{
    return std::string(m.text_of(m.here()));
}

bool path_at(eval::Machine &m, const Value *arguments, uint32_t who,
             std::string *out)
{
    const Value &value = arguments[who];
    if (const Str *text = std::get_if<Str>(&value)) {
        *out = *text ? decode(**text) : std::string();
        return true;
    }
    if (value.is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.string` path",
        type_name(value)));
    return false;
}

bool handle_at(eval::Machine &m, const Value *arguments, uint32_t who,
               FileHandle **out)
{
    const Value &value = arguments[who];
    if (const Fil *held = std::get_if<Fil>(&value)) {
        // A null handle inside the arm is not how an unopened file is spelled
        // -- a declared file holds nothing, which is the arm below -- but the
        // null is checked because the cost is one branch and the alternative is
        // dereferencing it.
        if (*held) {
            *out = held->get();
            return true;
        }
    }
    if (value.is_nothing() && who == 0) {
        m.refuse(errors::make<errors::Code::EVAL_HOLDING_NOTHING>(
            m.span_of(m.here()), asked(m)));
        return false;
    }
    m.refuse(errors::make<errors::Code::EVAL_WRONG_TYPE>(
        m.span_of(m.here()), asked(m), "a `satellite.variable.file`",
        type_name(value)));
    return false;
}

Value opened(const std::string &name, const Mode &mode, int flags)
{
    Fil handle = std::make_shared<FileHandle>();
    handle->path = name;
    handle->readable = mode.readable;
    handle->writable = mode.writable;
    handle->reopen_flags = mode.flags;

    const int fd = ::open(name.c_str(), flags, 0644);
    if (fd < 0) {
        handle->last_error.store(errno);
    } else {
        handle->descriptor.store(fd);
        handle->ever_open = true;
    }
    return Value(std::move(handle));
}

bool wrong_direction(eval::Machine &m, const FileHandle &handle, bool allowed,
                     const char *verb, const std::string &modes)
{
    if (allowed)
        return false;
    m.refuse(errors::make<errors::Code::FILE_WRONG_DIRECTION>(
        m.span_of(m.here()), asked(m), verb, handle.path, modes));
    return true;
}

int descriptor_of(eval::Machine &m, FileHandle &handle)
{
    const int fd = handle.descriptor.load();
    if (fd >= 0)
        return fd;

    // TWO STATES, TWO SENTENCES. A descriptor of -1 means "not open" and that
    // is two different things to the person reading the message: a handle they
    // closed, which `open` reopens, and a handle that never opened, where the
    // fix is at the constructor and `error` already holds the reason.
    if (!handle.ever_open) {
        m.refuse(errors::make<errors::Code::FILE_NEVER_OPENED>(
            m.span_of(m.here()), asked(m), handle.path,
            std::strerror(handle.last_error.load())));
        return fd;
    }
    m.refuse(errors::make<errors::Code::FILE_IS_CLOSED>(
        m.span_of(m.here()), asked(m)));
    return fd;
}

namespace {

// The mode word an argument names, or a refusal and nullptr. ONE SITE, so
// S1201 is raised in one place and `new` and `open` cannot come to disagree
// about what "append" means.
const Mode *mode_at(eval::Machine &m, const Value *arguments, uint32_t who)
{
    std::string word;
    if (!path_at(m, arguments, who, &word))
        return nullptr;
    if (const Mode *found = mode_named(word))
        return found;
    m.refuse(errors::make<errors::Code::FILE_BAD_MODE>(
        m.span_of(m.here()), asked(m), "\"" + word + "\""));
    return nullptr;
}

// `satellite.file.open(path, mode)` `1 8 2` -- one of the two ways a file value
// comes into existence, and the reason a DECLARATION is neither: opening needs
// a path and a place to report failure, and a declaration has neither.
bool file_open(eval::Machine &m, const Value *arguments, uint32_t, Value *answer)
{
    std::string name;
    if (!path_at(m, arguments, 0, &name))
        return false;
    const Mode *mode = mode_at(m, arguments, 1);
    if (mode == nullptr)
        return false;
    *answer = opened(name, *mode, mode->flags);
    return true;
}

// `satellite.file.new(path)` `1 8 1` and `new(path, mode)` `1 8 4` -- make a
// file that is NOT there, and hand back a handle already open on it.
//
// IT REFUSES TO CLOBBER, and the argument is not squeamishness. The truncating
// spelling already exists -- `open(path, "write")` is O_CREAT | O_TRUNC -- so a
// `new` that truncated would be a SECOND spelling of an operation the language
// already has, while leaving "make this only if it is not there" with no
// spelling at all. And O_EXCL is the kernel answering atomically; a
// check-then-create written in satellite would be a TOCTOU race, which is not a
// thing to hand a language whose file surface exists to be used from scripts.
//
// A PATH THAT EXISTS IS A VALUE AND NOT AN ERROR -- the handle comes back
// holding EEXIST and the caller asks `ok`. PLAN M19's done-when rests on it:
// "confirm a second `new` on the same path comes back as a value that is not ok
// rather than clobbering the file".
//
// THE MODE DEFAULTS TO "read_append", which is what makes `open` never REQUIRED
// on a fresh handle: what comes back is already open, in the one mode that can
// both write the file you just made and read it back.
bool file_new(eval::Machine &m, const Value *arguments, uint32_t count,
              Value *answer)
{
    std::string name;
    if (!path_at(m, arguments, 0, &name))
        return false;

    const Mode *mode = count == 2 ? mode_at(m, arguments, 1)
                                  : mode_named("read_append");
    if (mode == nullptr)
        return false;

    // O_TRUNC is dropped rather than carried: a file O_EXCL just created is
    // empty, so truncating it is a no-op that would only confuse anyone reading
    // the flags. O_EXCL is NOT stored in reopen_flags -- file_handle.hpp says
    // why: a reopen would otherwise fail with EEXIST against the program's own
    // work.
    const int create = (mode->flags & ~O_TRUNC) | O_CREAT | O_EXCL;
    *answer = opened(name, *mode, create);
    return true;
}

// `satellite.file.clear(path)` `1 8 3` -- a file back to zero bytes, BY NAME.
//
// THE MODULE FACE IS THE ONLY FACE. v1 had `clear` as a handle method and no
// module form; the author settled on 2026-09-08 that the handle keeps none, so
// there is one spelling and nothing to keep in step. `truncate` rather than
// open-with-O_TRUNC because there is no handle here to leave behind and no
// offset to rewind -- the whole reason v1's version had to seek was that it
// acted through a descriptor a program went on using.
//
// TRUE OR FALSE AND NEVER AN ERROR, `satellite.directory.change`'s contract:
// the file not being there is an answer a cleanup step has to survive.
bool file_clear(eval::Machine &m, const Value *arguments, uint32_t,
                Value *answer)
{
    std::string name;
    if (!path_at(m, arguments, 0, &name))
        return false;
    *answer = Value::boolean(::truncate(name.c_str(), 0) == 0);
    return true;
}

// `satellite.file.exists(path)` `1 8 5` -- is there a file there, asked with no
// handle. Minted 2026-09-08, mirroring `satellite.directory.exists(d)`
// `1 18 3`, because `exists` `1 6 2 7` sits on the TYPE node and "does this
// file exist" is asked before a handle exists.
//
// A DIRECTORY IS NOT A FILE AND ANSWERS FALSE, which is the difference from
// `1 18 3` and is the whole reason both rows exist. stat and not lstat: a
// symlink to a regular file is a file you can open, and `open` is what the
// caller is about to do.
bool file_exists(eval::Machine &m, const Value *arguments, uint32_t,
                 Value *answer)
{
    std::string name;
    if (!path_at(m, arguments, 0, &name))
        return false;
    struct stat info;
    *answer = Value::boolean(::stat(name.c_str(), &info) == 0 &&
                             !S_ISDIR(info.st_mode));
    return true;
}

// `satellite.system.delete(x)` `1 22 1` -- unlink a file, rmdir an EMPTY
// directory. Two shapes of argument, ONE question, so they are one name.
//
// EVERYTHING ELSE IS REFUSED RATHER THAN STRINGIFIED, v1's sharpest note:
// rendering 7 gives "7", which is a filename the filesystem would accept, so a
// number that reached here by mistake would delete a file named after itself
// and answer true. S1207 covers nil in the same sentence, because the way a
// program arrives at nil here is almost always a `satellite.variable.file` that
// was declared and never opened.
bool system_delete(eval::Machine &m, const Value *arguments, uint32_t,
                   Value *answer)
{
    std::string name;
    if (const Str *text = std::get_if<Str>(&arguments[0])) {
        name = *text ? decode(**text) : std::string();
    } else if (const Fil *held = std::get_if<Fil>(&arguments[0]); held && *held) {
        // The handle answers with the path it was opened on, so a program that
        // already holds one does not have to write `.delete(f.path())` to say a
        // thing it has already said.
        name = (*held)->path;
    } else {
        m.refuse(errors::make<errors::Code::DELETE_NOT_A_NAME>(
            m.span_of(m.here()), type_name(arguments[0])));
        return false;
    }

    // lstat, NEVER stat. A symlink pointing at a directory is the one case
    // where the two disagree and the disagreement is destructive: stat follows
    // the link and answers "directory", which would aim rmdir at the TARGET --
    // so deleting a link would either remove somebody else's directory or, more
    // often, fail with ENOTDIR and leave the link standing. The name given is
    // the link, so the link is what goes.
    struct stat info;
    if (::lstat(name.c_str(), &info) != 0) {
        *answer = Value::boolean(false);
        return true;
    }

    // rmdir for a directory and unlink for everything else, which is the whole
    // of the dispatch: POSIX has two calls and each refuses the other's
    // argument. TRUE OR FALSE, NEVER AN ERROR -- a destructive call is the last
    // place to hand a caller a refusal they did not ask for, and false is a TRUE
    // answer in every case it collapses: nothing was there, the directory is not
    // empty, it is not ours to delete. "It is still there" is what false means.
    //
    // THIS DOES NOT RECURSE AND THERE IS NO FLAG TO ASK IT TO. The language has
    // no flags, so one name gets one behaviour, and a delete that silently
    // descends is the single mistake in this module nobody gets to take back. A
    // caller who means a tree walks `satellite.directory.list` and says each
    // name out loud.
    const bool gone = S_ISDIR(info.st_mode) ? ::rmdir(name.c_str()) == 0
                                            : ::unlink(name.c_str()) == 0;
    *answer = Value::boolean(gone);
    return true;
}

} // namespace

void install_handlers()
{
    using words::NodeId;
    auto &table = eval::Handlers::table();

    // No module row binds a receiver: `file` and `system` are namespaces, not
    // values. The handle's rows do, and install_file_methods sets the tag.
    table.install(static_cast<words::PathId>(NodeId::FILE_NEW_PATH),
                  eval::Handler{file_new, false, 1, "M19"});
    table.install(static_cast<words::PathId>(NodeId::FILE_OPEN),
                  eval::Handler{file_open, false, 2, "M19"});
    table.install(static_cast<words::PathId>(NodeId::FILE_CLEAR),
                  eval::Handler{file_clear, false, 1, "M19"});
    table.install(static_cast<words::PathId>(NodeId::FILE_NEW_PATH_MODE),
                  eval::Handler{file_new, false, 2, "M19"});
    table.install(static_cast<words::PathId>(NodeId::FILE_EXISTS),
                  eval::Handler{file_exists, false, 1, "M19"});
    table.install(static_cast<words::PathId>(NodeId::SYSTEM_DELETE),
                  eval::Handler{system_delete, false, 1, "M19"});

    install_file_methods();
}

} // namespace satellite::file
