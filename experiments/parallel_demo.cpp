// parallel_demo.cpp -- a working model of satellite-004's parallel groups
// (PLAN M2, M7 and M12), written 2026-09-14 to show the author's design running.
//
// WHAT IT MODELS
//   parallel_start:            the author's group: every line runs on its own
//   #1.1.1(args)               pool thread, and the group ends at the first
//   #1.1.1(args)               empty line
//
//   #1.1.1(a).#1.1.1(b)        the author's inline form: the commands joined by
//                              "." run in parallel
//
//   #loop N <body>             (model-only syntax) run <body> N times; a body of
//                              commands joined by ";" runs in order
//
//   256 threads: 192 RUNNERS execute groups and batches, 64 SEARCHERS look for
//   loops that can be batched while the main thread keeps running.
//
//   THE MONITOR UNMARKS: a marked group runs in parallel first; if it repeats, it is
//   timed once in order, and from then on runs whichever way was faster.
//
//   THE SEARCHERS MARK: a loop's first 1,000 iterations run in order while their
//   shared writes are recorded; a searcher thread studies the record while the
//   main thread carries on. If nothing shared was written, the rest of the loop
//   is split into batches on the runners, SPECULATIVELY: an iteration that
//   tries a shared write stops and flags itself, every iteration from that one
//   on is thrown away, and the loop finishes in order from there. Output is held
//   per iteration until it is known to be good, so the result is always
//   byte-identical to running everything in order.
//
// RUN:  ./parallel_demo program.sati sequential > a.txt
//       ./parallel_demo program.sati parallel   > b.txt   and  cmp a.txt b.txt

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using hrc = std::chrono::high_resolution_clock;
static double since(hrc::time_point t) { return std::chrono::duration<double>(hrc::now() - t).count(); }

// ---------------------------------------------------------------- the pools

class Pool {
public:
    explicit Pool(unsigned threads)
    {
        for (unsigned i = 0; i < threads; i++)
            workers_.emplace_back([this] { work(); });
    }
    ~Pool()
    {
        { std::lock_guard<std::mutex> lock(m_); stop_ = true; }
        wake_.notify_all();
        for (std::thread &worker : workers_) worker.join();
    }
    void submit(std::function<void()> job)
    {
        { std::lock_guard<std::mutex> lock(m_); jobs_.push_back(std::move(job)); }
        wake_.notify_one();
    }
    unsigned size() const { return static_cast<unsigned>(workers_.size()); }

private:
    void work()
    {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(m_);
                wake_.wait(lock, [this] { return stop_ || !jobs_.empty(); });
                if (stop_ && jobs_.empty()) return;
                job = std::move(jobs_.front());
                jobs_.pop_front();
            }
            job();
        }
    }
    std::vector<std::thread> workers_;
    std::deque<std::function<void()>> jobs_;
    std::mutex m_;
    std::condition_variable wake_;
    bool stop_ = false;
};

// Waits for a known number of jobs.
class Latch {
public:
    explicit Latch(long count) : left_(count) {}
    void done() { std::lock_guard<std::mutex> lock(m_); if (--left_ == 0) cv_.notify_all(); }
    void wait() { std::unique_lock<std::mutex> lock(m_); cv_.wait(lock, [this] { return left_ == 0; }); }
private:
    long left_;
    std::mutex m_;
    std::condition_variable cv_;
};

// ---------------------------------------------------------------- program state and commands

struct Context {
    std::string out;                        // this unit's own output
    std::vector<size_t> iteration_start;    // loops: where each iteration's output begins
    bool speculating = false;
    long long conflict = -1;                // first iteration that tried a shared write
    std::vector<int> *shared_writes = nullptr;
};

static std::vector<unsigned long long> slot;   // one per iteration: never shared
static unsigned long long total = 0;            // shared between iterations
static long long tally_at = 1700001;            // the one iteration whose branch writes `total`

static unsigned long long math(unsigned long long seed, long long steps)
{
    unsigned long long z = seed, sum = 0;
    for (long long s = 0; s < steps; s++) {
        z += 0x9e3779b97f4a7c15ULL;
        unsigned long long x = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        sum += x ^ (x >> 31);
    }
    return sum;
}

struct Cmd {
    int id = 0;
    long long arg = 0;
    std::string text;
    bool uses_i = false;
};

// #1.1.1 heavy math   #1.1.2 cheap   #1.5.1 display   #2.1 fill a slot   #2.2 tally into the shared total
static void run_cmd(const Cmd &c, Context &ctx, long long i)
{
    const long long arg = c.uses_i ? i : c.arg;
    switch (c.id) {
    case 111: ctx.out += "heavy " + std::to_string(math(arg, arg)) + "\n"; break;
    case 112: ctx.out += "cheap " + std::to_string(arg) + "\n"; break;
    case 151: ctx.out += c.text + "\n"; break;
    case 21:
        slot[arg] = math(arg, 200);
        if (arg % 400000 == 0) ctx.out += "slot " + std::to_string(arg) + " = " + std::to_string(slot[arg]) + "\n";
        break;
    case 22:
        if (arg == tally_at) {                      // a branch the first 1,000 iterations never take
            if (ctx.speculating) {                  // THE GUARD: stop before touching shared state
                if (ctx.conflict < 0 || arg < ctx.conflict) ctx.conflict = arg;
                return;
            }
            if (ctx.shared_writes) ctx.shared_writes->push_back(1);
            total += slot[arg];
            ctx.out += "total " + std::to_string(total) + " at " + std::to_string(arg) + "\n";
        }
        break;
    }
}

// ---------------------------------------------------------------- parsing

struct Statement {
    enum Kind { command, group, loop_in_order, loop_of_groups } kind = command;
    std::vector<Cmd> cmds;
    long long count = 0;
    // the monitor
    int runs = 0;
    double in_order_seconds = 0, parallel_seconds = 0;
};

static Cmd parse_cmd(const std::string &token)
{
    Cmd c;
    const size_t open = token.find('(');
    std::string number = token.substr(1, open - 1), arg = token.substr(open + 1, token.rfind(')') - open - 1);
    if (number == "1.1.1") c.id = 111;
    else if (number == "1.1.2") c.id = 112;
    else if (number == "1.5.1") c.id = 151;
    else if (number == "2.1") c.id = 21;
    else if (number == "2.2") c.id = 22;
    else { std::fprintf(stderr, "no command %s\n", number.c_str()); std::exit(2); }
    if (arg == "i") c.uses_i = true;
    else if (!arg.empty() && arg[0] == '"') c.text = arg.substr(1, arg.size() - 2);
    else c.arg = std::atoll(arg.c_str());
    return c;
}

// Split "a.#b.#c" (parallel) or "a;b" (in order) at the separator before a '#'.
static std::vector<Cmd> split_cmds(const std::string &line, char separator)
{
    std::vector<Cmd> cmds;
    size_t start = 0;
    for (size_t i = 0; i + 1 < line.size(); i++)
        if (line[i] == separator && line[i + 1] == '#' && i > 0 && line[i - 1] == ')') {
            cmds.push_back(parse_cmd(line.substr(start, i - start)));
            start = i + 1;
        }
    cmds.push_back(parse_cmd(line.substr(start)));
    return cmds;
}

static std::vector<Statement> parse(const char *path)
{
    std::ifstream file(path);
    std::vector<Statement> program;
    std::string line;
    bool in_group = false;
    while (std::getline(file, line)) {
        if (in_group) {
            if (line.empty()) { in_group = false; continue; }      // the author's rule: the group ends at an empty line
            program.back().cmds.push_back(parse_cmd(line));
            continue;
        }
        if (line.empty()) continue;
        if (line == "parallel_start:") { program.push_back({Statement::group}); in_group = true; continue; }
        Statement s;
        if (line.rfind("#loop ", 0) == 0) {
            const size_t space = line.find(' ', 6);
            s.count = std::atoll(line.substr(6, space - 6).c_str());
            const std::string body = line.substr(space + 1);
            if (body.find(").#") != std::string::npos) { s.kind = Statement::loop_of_groups; s.cmds = split_cmds(body, '.'); }
            else { s.kind = Statement::loop_in_order; s.cmds = split_cmds(body, ';'); }
        } else if (line.find(").#") != std::string::npos) {
            s.kind = Statement::group;
            s.cmds = split_cmds(line, '.');
        } else {
            s.kind = Statement::command;
            s.cmds.push_back(parse_cmd(line));
        }
        program.push_back(std::move(s));
    }
    return program;
}

// ---------------------------------------------------------------- running

static bool parallel_mode = false;
static Pool *runners = nullptr, *searchers = nullptr;

static void group_in_order(const std::vector<Cmd> &cmds, std::string &out, long long i)
{
    Context ctx;
    for (const Cmd &c : cmds) run_cmd(c, ctx, i);
    out += ctx.out;
}

static void group_in_parallel(const std::vector<Cmd> &cmds, std::string &out, long long i)
{
    std::vector<Context> parts(cmds.size());
    Latch latch(static_cast<long>(cmds.size()));
    for (size_t k = 0; k < cmds.size(); k++)
        runners->submit([&, k] { run_cmd(cmds[k], parts[k], i); latch.done(); });
    latch.wait();
    for (const Context &part : parts) out += part.out;             // printed in written order
}

// The monitor: a group the author marked runs in parallel first; if it runs again it is
// timed once in order, and from then on it keeps whichever way was faster.
static void group(Statement &s, std::string &out, long long i)
{
    if (!parallel_mode) { group_in_order(s.cmds, out, i); return; }
    auto t = hrc::now();
    if (s.runs == 0) { group_in_parallel(s.cmds, out, i); s.parallel_seconds = since(t); }
    else if (s.runs == 1) { group_in_order(s.cmds, out, i); s.in_order_seconds = since(t); }
    else if (s.parallel_seconds < s.in_order_seconds) group_in_parallel(s.cmds, out, i);
    else group_in_order(s.cmds, out, i);                               // UNMARKED: parallel did not pay
    s.runs++;
}

static void loop_in_order(Statement &s, std::string &out, std::string &report)
{
    const long long n = s.count;
    slot.assign(n, 0);
    Context main_ctx;
    long long i = 0;
    if (parallel_mode) {
        // 1. record the first iterations' shared writes, in order
        auto writes = std::make_shared<std::vector<int>>();
        main_ctx.shared_writes = writes.get();
        const long long recorded = std::min(n, 1000LL);
        for (; i < recorded; i++) for (const Cmd &c : s.cmds) run_cmd(c, main_ctx, i);
        main_ctx.shared_writes = nullptr;
        // 2. a searcher studies the record while the main thread keeps running
        auto verdict = std::make_shared<std::promise<bool>>();
        std::future<bool> answer = verdict->get_future();
        searchers->submit([writes, verdict] { verdict->set_value(writes->empty()); });
        while (i < n && answer.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            const long long until = std::min(n, i + 1024);
            for (; i < until; i++) for (const Cmd &c : s.cmds) run_cmd(c, main_ctx, i);
        }
        const bool batchable = i < n && answer.get();
        report += "  recorded 1,000 iterations; the searcher answered after " + std::to_string(i) + "; ";
        if (!batchable || n - i < 192 * 1000) {
            report += "not batched\n";
        } else {
            // 3. speculative batches on the runners
            const unsigned batches = runners->size();
            const long long from = i, span = n - from;
            std::vector<Context> parts(batches);
            Latch latch(batches);
            for (unsigned k = 0; k < batches; k++) {
                const long long a = from + span / batches * k, b = k + 1 == batches ? n : from + span / batches * (k + 1);
                parts[k].speculating = true;
                runners->submit([&, k, a, b] {
                    Context &ctx = parts[k];
                    for (long long j = a; j < b; j++) {
                        ctx.iteration_start.push_back(ctx.out.size());
                        for (const Cmd &c : s.cmds) run_cmd(c, ctx, j);
                        if (ctx.conflict >= 0) break;                 // this iteration touched shared state
                    }
                    latch.done();
                });
            }
            latch.wait();
            long long conflict = -1;
            for (const Context &p : parts) if (p.conflict >= 0 && (conflict < 0 || p.conflict < conflict)) conflict = p.conflict;
            // 4. keep output only for iterations before the conflict
            for (unsigned k = 0; k < batches; k++) {
                const long long a = from + span / batches * k;
                if (conflict >= 0 && a >= conflict) break;
                const size_t keep = conflict >= 0 && conflict - a < (long long)parts[k].iteration_start.size()
                                        ? parts[k].iteration_start[conflict - a] : parts[k].out.size();
                main_ctx.out.append(parts[k].out, 0, keep);
            }
            i = conflict >= 0 ? conflict : n;
            report += "batched " + std::to_string(span) + " iterations on " + std::to_string(batches) + " runners; ";
            report += conflict >= 0 ? "iteration " + std::to_string(conflict) + " tried a shared write, so " +
                                          std::to_string(n - conflict) + " iterations re-ran in order\n"
                                    : "no shared write\n";
        }
    }
    for (; i < n; i++) for (const Cmd &c : s.cmds) run_cmd(c, main_ctx, i);   // in order (or the rest after a conflict)
    out += main_ctx.out;
}

int main(int argc, char **argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: parallel_demo program.sati sequential|parallel [tally_at]\n"); return 2; }
    parallel_mode = std::strcmp(argv[2], "parallel") == 0;
    if (argc > 3) tally_at = std::atoll(argv[3]);
    std::vector<Statement> program = parse(argv[1]);
    Pool run_pool(192), search_pool(64);
    runners = &run_pool;
    searchers = &search_pool;

    std::string out;
    auto whole = hrc::now();
    for (size_t k = 0; k < program.size(); k++) {
        Statement &s = program[k];
        std::string report;
        auto t = hrc::now();
        switch (s.kind) {
        case Statement::command: group_in_order(s.cmds, out, 0); break;
        case Statement::group: group(s, out, 0); break;
        case Statement::loop_of_groups: for (long long i = 0; i < s.count; i++) group(s, out, i); break;
        case Statement::loop_in_order: loop_in_order(s, out, report); break;
        }
        std::fprintf(stderr, "%-10s statement %zu (%s): %.3f s\n%s", argv[2], k + 1,
                     s.kind == Statement::command ? "command" : s.kind == Statement::group ? "group" :
                     s.kind == Statement::loop_of_groups ? "loop of groups" : "loop", since(t), report.c_str());
        if (s.kind == Statement::loop_of_groups && parallel_mode)
            std::fprintf(stderr, "  monitor: in order %.0f ns, parallel %.0f ns -> %s\n", s.in_order_seconds * 1e9,
                         s.parallel_seconds * 1e9, s.parallel_seconds < s.in_order_seconds ? "kept parallel" : "UNMARKED, runs in order");
    }
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fprintf(stderr, "%-10s whole program: %.3f s, %zu bytes of output\n", argv[2], since(whole), out.size());
}
