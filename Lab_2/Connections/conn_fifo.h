#pragma once
class ConnectionFifo : public Connection{
private:
    static std::string const path;
    std::string fileName;
public:
    ConnectionFifo(size_t id, bool isHost);
    ~ConnectionFifo();
    bool Read(void* buf, size_t count) override;
    bool Write(void* buf, size_t count) override;
};

std::string const ConnectionFifo::path = "/tmp/myfifo";

