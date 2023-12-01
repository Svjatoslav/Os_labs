
#include "host.h"

int main(int argc, char* argv[]) {
    std::cout << getpid() << std::endl;
    return Host::getInstance().start();
}
