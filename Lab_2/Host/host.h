#include "../message_queue.h"
#include "../Connections/connection.h"
#include "../message.h"
#include <map>
#include <vector>
#include <signal.h>
#include <future>
#include <bits/types/siginfo_t.h>

class Host{
private:
    Host();
    ~Host();
    
    messageQueue input;
    messageQueue output;
    
    std::map<pid_t, pthread_t> clients;
    
    static Host instance;
    bool isRunning = false;
    Message outputMsg;
    bool messageEntered = false;
    std::vector<Connection*> connections;
    
    bool getMessage(pid_t pid, Message* msg);
    void sendMessage(pid_t pid, Message msg);
    std::function<Message()> getCinInput = []() -> Message{Message msg; std::cin.getline(msg.m_message, 256); return msg; };
  
    static void signalHandler(int signal_id, siginfo_t* info, void *ptr);
    static void* connectionWork(void*  argv);
    void endWork();

public:
    static Host& getInstance();
    int start();
};
