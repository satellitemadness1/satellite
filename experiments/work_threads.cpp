#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <unistd.h>

using hrc = std::chrono::high_resolution_clock;

struct Job { unsigned long long from, to, answer; bool sleeping; };

static void *run(void *pointer)
{
    Job &job = *static_cast<Job *>(pointer);
    if (job.sleeping) { usleep(1000000); job.answer = 1; return nullptr; }     // waiting work: 1 second of waiting
    unsigned long long total = 0;                                              // processor work: mix every number
    for (unsigned long long x = job.from; x < job.to; x++) {
        unsigned long long z = x + 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        total += z ^ (z >> 31);
    }
    job.answer = total;
    return nullptr;
}

static double race(long threads, unsigned long long work, bool sleeping, unsigned long long &answer)
{
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, 64 * 1024);
    std::vector<Job> jobs(threads);
    std::vector<pthread_t> handles(threads);
    for (long i = 0; i < threads; i++)
        jobs[i] = {work / threads * i, i == threads - 1 ? work : work / threads * (i + 1), 0, sleeping};
    auto start = hrc::now();
    for (long i = 0; i < threads; i++) pthread_create(&handles[i], &attributes, run, &jobs[i]);
    for (long i = 0; i < threads; i++) pthread_join(handles[i], nullptr);
    double seconds = std::chrono::duration<double>(hrc::now() - start).count();
    answer = 0;
    for (const Job &job : jobs) answer += job.answer;
    return seconds;
}

int main()
{
    const unsigned long long work = 3000000000ULL;   // the same 3 billion steps, however many threads share them
    std::printf("PROCESSOR WORK: 3,000,000,000 steps in total, split across N threads\n");
    std::printf("%10s %10s %12s   %s\n", "threads", "seconds", "vs 1 thread", "answer (must match)");
    double one = 0;
    for (long threads : {1L, 2L, 4L, 12L, 24L, 48L, 1000L, 10000L}) {
        unsigned long long answer;
        double seconds = race(threads, work, false, answer);
        if (threads == 1) one = seconds;
        std::printf("%10ld %10.2f %11.1fx   %llu\n", threads, seconds, one / seconds, answer);
    }
    std::printf("\nWAITING WORK: each thread waits 1 second (like waiting on a disk, a network or a user)\n");
    for (long threads : {1L, 24L, 10000L}) {
        unsigned long long answer;
        double seconds = race(threads, 0, true, answer);
        std::printf("%10ld threads: %llu seconds of waiting finished in %.2f s\n", threads, answer, seconds);
    }
}
