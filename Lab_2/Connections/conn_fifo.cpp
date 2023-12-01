#include "connection.h"
#include "conn_fifo.h"

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Connection* Connection::create(size_t id, bool isHost, size_t msg_size){
    return new ConnectionFifo(id, isHost);
}

Connection::~Connection(){}


ConnectionFifo::ConnectionFifo(size_t id, bool isHost){
    fileName = path + std::to_string(id);
    _isHost = isHost;
    if (isHost && mkfifo(fileName.c_str(), 0777) < 0)
        std::cout << "Make fifo error" << std::endl;
    desc = open(fileName.c_str(), O_RDWR | O_NONBLOCK);
    if (desc == -1)
        std::cout << "Open error" << std::endl;
}

ConnectionFifo::~ConnectionFifo(){
    close(desc);
    if (_isHost) {
        remove(fileName.c_str());
    }
}

bool ConnectionFifo::Read(void* buf, size_t count){
    bool res = true;
    int lenght = read(desc, buf, count);
    if (lenght == -1) {
        res = false;
    }
    return res;
}

bool ConnectionFifo::Write(void* buf, size_t count){
    bool res = true;
    int length = write(desc, buf, count);
    if (length == -1) {
        res = false;
    }
    return res;
}
