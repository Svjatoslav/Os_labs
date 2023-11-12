#include "daemon.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <csignal>
#include <thread>
#include <sys/prctl.h>
#include <syslog.h>
#include <filesystem>
#include <variant>
#include <fcntl.h>



Daemon::Daemon() {
    data = {};
    config_file_path = std::filesystem::absolute(config_file_path);
    pid_t pid = fork();
    if (pid < 0) {exit(1);}  
    else if (pid > 0) {exit(0);} 
    else {  
        prctl(PR_SET_NAME, "dmn1", 0, 0, 0);

        umask(0);
        setsid(); 

        if (chdir("/") < 0) {exit(1);}

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO); 

        open("/dev/null", O_RDONLY);
        open("/dev/null", O_WRONLY);
        open("/dev/null", O_WRONLY);
    }
}


Daemon& Daemon::get_instance() {
    static Daemon instance;
    return instance;
}


void Daemon::daemon_start() {
    if (!reset_new_process()) {
        syslog(LOG_INFO, "ERROR IN KILLING PROCESS. EXIT.");
        return;
    }
    if (!write_pid(static_cast<int>(getpid()))) {
        syslog(LOG_INFO, "ERROR WITH PID FILE. EXIT.");
        return;
    }
    if (!read_config()) {
        syslog(LOG_INFO, "ERROR WITH CONFIG FILE. EXIT.");
        return;
    }
    

    std::string process_name = "my_daemon_" + std::to_string(getpid());
    openlog(process_name.c_str(), LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "DAEMON STARTED");

    struct sigaction sa_term;
    sa_term.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sa_term, nullptr);

    struct sigaction sa_hup;
    sa_hup.sa_handler = sighup_handler;
    sigaction(SIGHUP, &sa_hup, nullptr);



    while (is_it_time_to_finish == 0) { 
        
        copy_files(data[0], data[1]);


        my_sleep();
    }

    syslog(LOG_INFO, "DAEMON KILLED");
    closelog();

}

// bool Daemon::create_config(){
//     std::ifstream file(config_file_path);
//      if (!file.is_open()){
//         std::ofstream newFile(config_file_path.c_str());
//         return true
//     }
//     return false
// }

bool Daemon::read_config() {
    
    std::ifstream file(config_file_path);        
    if (file.is_open()) {
            std::vector<std::string> config_data;
            std::string line;
            while (std::getline(file, line)) {
                config_data.push_back(line);
            }
            file.close();
            data = config_data;
            return true;
        }
    return false;
}


int Daemon::read_pid() {
  
    std::ifstream file(pid_file_path);

       if (file.is_open()) {
        syslog(LOG_INFO, "FILE HAS OPENED");
        std::string line;
        if (!std::getline(file, line)) {
            file.close();
            return 2;
        }
        file.close();
        return std::stod(line); 
    }
    
    syslog(LOG_INFO, "FILE HAS NOT OPENED");
    return 1;

}


bool Daemon::write_pid(int out_pid) {
   
    std::ofstream outputFile(pid_file_path, std::ios::trunc);  // clean file and then write

    if (outputFile.is_open()) {
        outputFile << out_pid;
        outputFile.close();
        return true;
    } 
     return false;
}



int Daemon::get_file_age(std::string file_path) { 
    std::filesystem::file_time_type file_time = std::filesystem::last_write_time(file_path);
    std::filesystem::file_time_type now = std::filesystem::file_time_type::clock::now();
    std::filesystem::file_time_type::duration age = now - file_time;
    int fileAge = std::chrono::duration_cast<std::chrono::duration<double>>(age).count();
    return fileAge;
}


void Daemon::copy_files(const std::string& sourceDir, const std::string& destDir) {
    try {
        
            std::filesystem::create_directories(destDir + "/NEW");
            std::filesystem::create_directories(destDir + "/OLD");

            for (const auto& entry : std::filesystem::directory_iterator(sourceDir)) {
                if (std::filesystem::is_regular_file(entry.path())) {
                    std::string destinationPath;

                    int fileAge = get_file_age(entry.path().string());
                    if (abs(fileAge) > 3 * 60) { // 3 minutes in seconds
                        destinationPath = destDir + "/OLD/" + entry.path().filename().string();
                    }
                    else {
                        destinationPath = destDir + "/NEW/" + entry.path().filename().string();
                    }
                    std::filesystem::copy(entry.path(), destinationPath, std::filesystem::copy_options::overwrite_existing);
                }
            }
        
        syslog(LOG_INFO, "DONE");
    }
    catch (const std::filesystem::filesystem_error& e) {
        syslog(LOG_INFO, "ERROR WITH COPY FILES");
    }
}


bool Daemon::check_if_process_exists(pid_t pid) {
    std::string path = "/proc/" + std::to_string(pid);
    std::ifstream dir(path);
    if (dir) {return true;}
    return false;
}



void Daemon::sighup_handler(int signal_number) {
    read_config();
}


void Daemon::sigterm_handler(int signal_number) {
    is_it_time_to_finish = 1;
}


bool Daemon::reset_new_process() {
    int ex_pid = read_pid();
    syslog(LOG_INFO, "READ_PID");
    std::cout << ex_pid << std::endl;
    if (ex_pid == 2) {return false;}
    if (!check_if_process_exists(ex_pid)) {
        syslog(LOG_INFO, "CHECK PROCESS EXISTS");
        return true; 
    }
    int response = kill(ex_pid, SIGTERM);
    if (response == false) {
        syslog(LOG_INFO, "RESPONSE 0");
        return true;}
    else {
        syslog(LOG_INFO, "all bad");
        return false;}
}

void Daemon::my_sleep() {
    const int max_iterations = 30;
    int current_iteration = 0;

    while (current_iteration < max_iterations && is_it_time_to_finish == 0) {
        current_iteration++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}