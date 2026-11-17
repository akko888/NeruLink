#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <pthread.h>
#include <unistd.h>

#include <neru/discovery_service.h>

int main() {
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigs, nullptr);

    auto discovery = createDiscoveryService();

    std::mutex outMutex;

    discovery->OnPeerFound = [&](const PeerInfo& p) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "[+] Device found: id=" << p.deviceId
                  << " addr=" << p.host << ":" << p.port << std::endl;
    };

    discovery->OnPeerLost = [&](const std::string& id) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "[-] Device lost: id=" << id << std::endl;
    };

    discovery->OnError = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cerr << "[!] Error: " << msg << std::endl;
    };

    std::atomic<bool> stopping{false};

    std::thread worker([&] {
        discovery->startBrowse();
        if (!stopping) kill(getpid(), SIGTERM);
    });

    {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "Searching for new devices... (Ctrl+C to leave)" << std::endl;
    }

    int sig = 0;
    sigwait(&sigs, &sig);

    stopping = true;
    discovery->stopBrowse();
    worker.join();

    std::cout << "Leaving." << std::endl;
    return 0;
}