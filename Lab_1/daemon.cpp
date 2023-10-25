#include "daemon.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <csignal>
#include <ctime>
#include <thread>
#include <sys/prctl.h>
#include <syslog.h>
#include <filesystem>
#include <vector>
#include <variant>







Daemon::Daemon() {
    data = {};
    pid_t pid = fork();
    if (pid == 0) { prctl(PR_SET_NAME, "dmn1", 0, 0, 0);
        umask(0);
        
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        setsid();  } 
    else {exit(0)}
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
    if (!write_pid(std::to_string(getpid()))) {
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

    while (is_it_time_to_finish == 0) { 
        
        copy_files()
        signal(SIGHUP, sighup_handler);  // reread config
        signal(SIGTERM, sigterm_handler);  // kill daemon

        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    closelog();
    syslog(LOG_INFO, "DAEMON KILLED");
}


bool Daemon::read_config() {
    std::string filename = "config.txt";
    std::ifstream file(filename);

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
    else {return false;}
}


double Daemon::read_pid() {
    std::string filename = "pid.txt";
    std::ifstream file(filename);

       if (file.is_open()) {
        std::string line;
        if (!std::getline(file, line)) {
            file.close();
            return 2;
        }
        file.close();
        return std::stod(line); 
    }
    else {return 1;}

}


bool Daemon::write_pid(std::string out_pid) {
    std::string filePath = "pid.txt";
    std::ofstream outputFile(filePath, std::ios::trunc);  // clean file and then write

    if (outputFile.is_open()) {
        outputFile << out_pid;
        outputFile.close();
        return true;
    } else {
        return false;
    }
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
    double ex_pid = read_pid();
    if (ex_pid == -2) {return 0;}
    if (!check_if_process_exists(ex_pid)) {
        return 1; 
    }
    int response = kill(ex_pid, SIGTERM);
    if (response == 0) {return 1;}
    else {return 0;}
}

