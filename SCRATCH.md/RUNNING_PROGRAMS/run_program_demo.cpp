// Proof of the plan: posix_spawnp + raw pidfd_open + poll, no shell.
#include <spawn.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <cstdio>
#include <string>
#include <cstring>
extern char **environ;

int main(int argc, char **argv) {
    int out[2], err[2];
    pipe2(out, O_CLOEXEC); pipe2(err, O_CLOEXEC);
    posix_spawn_file_actions_t acts;
    posix_spawn_file_actions_init(&acts);
    posix_spawn_file_actions_adddup2(&acts, out[1], 1);
    posix_spawn_file_actions_adddup2(&acts, err[1], 2);
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[1], &acts, nullptr, argv + 1, environ);
    if (rc != 0) { std::printf("could not start %s: %s\n", argv[1], strerror(rc)); return 1; }
    close(out[1]); close(err[1]);
    int pidfd = syscall(SYS_pidfd_open, pid, 0);

    std::string text[2];
    pollfd fds[3] = {{out[0], POLLIN, 0}, {err[0], POLLIN, 0}, {pidfd, POLLIN, 0}};
    int open_pipes = 2;
    while (open_pipes > 0) {
        poll(fds, 3, -1);
        for (int i = 0; i < 2; i++) {
            if (fds[i].fd < 0 || !(fds[i].revents & (POLLIN | POLLHUP))) continue;
            char buf[4096];
            ssize_t n = read(fds[i].fd, buf, sizeof buf);
            if (n > 0) text[i].append(buf, n);
            else { close(fds[i].fd); fds[i].fd = -1; open_pipes--; }
        }
    }
    siginfo_t info{};
    waitid((idtype_t)P_PIDFD, pidfd, &info, WEXITED);
    std::printf("---- output (%zu bytes) ----\n%s", text[0].size(), text[0].c_str());
    std::printf("---- errors (%zu bytes) ----\n%s", text[1].size(), text[1].c_str());
    std::printf("---- exit code %d\n", info.si_status);
}
