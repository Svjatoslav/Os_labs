#include "../Connections/connection.h"
#include "../message_queue.h"
#include "../message.h"
#include <signal.h>
#include <functional>
#include <bits/types/siginfo_t.h>

class Client{
private:
    Client();
    ~Client();
    
    messageQueue input;
    messageQueue output;
    
    pid_t hostPid;
    static Client _instance;
    bool connected = false;
    bool isRunning = false;
    Message outputMsg;
    bool messageEntered = false;
    
    
    struct timespec connectRequestTime;
    static void signalHandler(int signal_id, siginfo_t* info, void* ptr);
    bool getMessage(pid_t pid, Message* msg);
    void sendMessage(pid_t pid, Message msg);
    bool getCinInputAvailavle();
    int endWork();
    
public:
    static Client& getInstance();
    int start();
    bool connectionRequest(pid_t pid);
    
};
