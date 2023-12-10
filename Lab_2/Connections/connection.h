#pragma once
#include <iostream>

class Connection
{
protected:
    int desc;
    bool _isHost;
public:
    static Connection* create(size_t id, bool isHost, size_t msg_size = 0);
    virtual bool Read(void* buf, size_t count) = 0;
    virtual bool Write(void* buf, size_t count) = 0;
    virtual ~Connection() = 0;
};



