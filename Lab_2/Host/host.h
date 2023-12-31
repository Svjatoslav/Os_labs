#include "../message_queue.h"
#include "../Connections/connection.h"
#include "../message.h"
#include <map>
#include <vector>
#include <signal.h>
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
    bool getCinInputAvailavle();
  
    static void signalHandler(int signal_id, siginfo_t* info, void *ptr);
    static void* connectionWork(void*  argv);
    void endWork();

public:
    static Host& getInstance();
    int start();
};
