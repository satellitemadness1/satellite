/* hello-vte.c -- the smallest proof that the VENDORED, STATIC libvte works:
 * a window holding a VteTerminal, a shell spawned in it, one line printed by
 * the shell, the child's exit status read back through `child-exited`, and
 * the terminal's own text read back so that what VTE drew is on stdout --
 * including, with gnutls off, the WARNING line VTE feeds itself (Q-VTE-1).
 * No GtkApplication (it would register on a bus), no session bus needed. */
#include <gtk/gtk.h>
#include <vte/vte.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static GMainLoop *loop;
static int outcome = 1;

static void child_exited(VteTerminal *term, int status, gpointer window)
{
    printf("child exited: raw %d, WEXITSTATUS %d\n", status, WEXITSTATUS(status));
    char *text = vte_terminal_get_text_format(term, VTE_FORMAT_TEXT);
    printf("--- what the terminal holds ---\n%s\n--- end ---\n", text ? text : "(null)");
    if (text && strstr(text, "hello from the vendored vte") && WEXITSTATUS(status) == 7)
        outcome = 0;
    g_free(text);
    fflush(stdout);
    gtk_window_destroy(GTK_WINDOW(window));
    g_main_loop_quit(loop);
}

static void spawned(VteTerminal *term, GPid pid, GError *error, gpointer user_data)
{
    if (error) { fprintf(stderr, "spawn failed: %s\n", error->message); g_main_loop_quit(loop); return; }
    printf("spawned pid %d\n", (int)pid);
    fflush(stdout);
}

static gboolean too_long(gpointer data)
{
    fprintf(stderr, "no child-exited after 15 s\n");
    g_main_loop_quit(loop);
    return G_SOURCE_REMOVE;
}

int main(void)
{
    if (!gtk_init_check()) { fprintf(stderr, "no display\n"); return 50; }
    printf("vte %u.%u.%u, gtk %u.%u.%u, features: %s\n",
           vte_get_major_version(), vte_get_minor_version(), vte_get_micro_version(),
           gtk_get_major_version(), gtk_get_minor_version(), gtk_get_micro_version(),
           vte_get_features());
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "hello-vte");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 400);
    GtkWidget *term = vte_terminal_new();
    gtk_window_set_child(GTK_WINDOW(window), term);
    g_signal_connect(term, "child-exited", G_CALLBACK(child_exited), window);
    gtk_window_present(GTK_WINDOW(window));
    char *argv[] = {"/bin/sh", "-c", "printf 'hello from the vendored vte\\n'; exit 7", NULL};
    vte_terminal_spawn_async(VTE_TERMINAL(term), VTE_PTY_DEFAULT, NULL, argv, NULL,
                             G_SPAWN_DEFAULT, NULL, NULL, NULL, -1, NULL, spawned, NULL);
    g_timeout_add_seconds(15, too_long, NULL);
    loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);
    return outcome;
}
