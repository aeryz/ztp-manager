// Std
#include <iostream>
#include <unistd.h> // fork()
#include <filesystem>

// External global
#include <boost/asio.hpp> // boost::asio::io_service
#include <mongocxx/instance.hpp> // mongocxx::instance

// External local
#include "../external/easylogging++/easylogging++.h"

// Internal
#include "server.h"

// Easyloggingpp initializer (has to be done due to docs)
INITIALIZE_EASYLOGGINGPP

inline void RunServer()
{
    try
    {
        boost::asio::io_service io_service;
        TCPServer server(io_service);
        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
        LOG(FATAL) << e.what();
    }
}

inline void Daemonize()
{
    pid_t pid = 0, sid = 0;

    pid = fork();
    if (pid < 0)
    {
        // FATAL
        std::cerr << "Fork error." << '\n';
        exit(1);
    }

    // Kill parent to run in the background
    if (pid > 0)
    {
        std::cout << "Process started with the PID: " << pid << '\n';
        exit(0);
    }

    // Make child process the process root
    umask(0);
    sid = setsid();
    if (sid < 0)
    {
        // FATAL
        std::cerr << "Error in setsid()." << '\n';
        exit(1);
    }

    chdir("/tmp");

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

inline void InitializeLogging()
{
    const char *conf_path = "/etc/ztp/ztp-manager-logging.conf";
    if (!std::filesystem::exists(conf_path) && !std::filesystem::is_regular_file(conf_path))
    {
        // TODO: Error message
        exit(1);
    }
    el::Configurations conf(conf_path);
    el::Loggers::reconfigureAllLoggers(conf);
}

inline void CheckForRootPermission()
{
    // Effective uid check
    if (geteuid() != 0)
    {
        // FATAL
        std::cerr << "Must be run as root" << '\n';
        exit(1);
    }
}

int main()
{
    CheckForRootPermission();
    Daemonize();
    InitializeLogging();

    // This has to be done to enable mongo driver
    mongocxx::instance inst{};
    RunServer();
}