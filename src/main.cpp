#include <drogon/drogon.h>
#include <print>

#include "api/HashController.hpp"

using namespace drogon;

int main() {
    const std::string ip = "0.0.0.0";
    const uint16_t port = 8080;
    const std::string static_files_root = "./static"; 

    app().setLogPath("./logs")
         .setLogLevel(trantor::Logger::kInfo);

    app().addListener(ip, port)
         .setThreadNum(0)
         .setDocumentRoot(static_files_root)
         .setHomePage("index.html")
         .enableGzip(true) 
         .enableDateHeader(true)
         .setServerHeaderField("GitHasher-Server/1.0");

    std::println(
        "\n"
        "==========================================\n"
        "   GitHasher Professional Server (C++26)  \n"
        "==========================================\n"
        " [x] Reactor Threads: Auto\n"
        " [x] Static Root:     {}\n"
        " [x] Listening on:    http://{}:{}\n"
        "==========================================",
        static_files_root, ip, port
    );

    try {
        app().run();
    } catch (const std::exception& e) {
        std::println(std::cerr, "FATAL ERROR: {}", e.what());
        return 1;
    }

    return 0;
}
