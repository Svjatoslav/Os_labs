#include "daemon.h"
#include <iostream>

int main() {
    Daemon& my_daemon = Daemon::get_instance();
    my_daemon.daemon_start();
    return 0;
}
