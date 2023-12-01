#include "client.h"

int main(int argc, char* argv[])
{
    if (argc != 2){
        std::cout << "Invalid arguments. Must be one args - host pid" << std::endl;
        return -1;
    }
    int host_pid = std::stoi(argv[1]);
    Client& client = Client::getInstance();
    if (!client.connectionRequest(host_pid)){
        std::cout << "Can not connect" << std::endl;
        return -1;
    }
    return client.start();
}
