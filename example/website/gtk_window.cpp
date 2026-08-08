#include <gtkmm.h>

class MainWindow : public Gtk::Window {
public:
    MainWindow() {
        set_title("GTKmm 4 Window");
        set_default_size(600, 400);
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create("com.example.gtkwindow");
    return app->make_window_and_run<MainWindow>(argc, argv);
}