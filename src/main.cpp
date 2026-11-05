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
    // Bloquear señales ANTES de crear hilos: todos los hilos heredan la máscara
    // y solo el hilo principal las recibe mediante sigwait().
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    sigaddset(&sigs, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigs, nullptr);

    auto discovery = createDiscoveryService();

    // Los callbacks se ejecutan en el hilo de Avahi; serializamos la salida.
    std::mutex outMutex;

    discovery->OnPeerFound = [&](const PeerInfo& p) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "[+] Peer encontrado: id=" << p.deviceId
                  << " addr=" << p.host << ":" << p.port << std::endl;
    };

    discovery->OnPeerLost = [&](const std::string& id) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "[-] Peer perdido: id=" << id << std::endl;
    };

    discovery->OnError = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cerr << "[!] Error: " << msg << std::endl;
    };

    std::atomic<bool> stopping{false};

    // start() bloquea (bucle de Avahi), así que corre en su propio hilo.
    std::thread worker([&] {
        discovery->start();
        // Si start() termina solo (p. ej. fallo al crear el cliente),
        // despertamos al hilo principal para que no se quede esperando.
        if (!stopping) kill(getpid(), SIGTERM);
    });

    {
        std::lock_guard<std::mutex> lock(outMutex);
        std::cout << "Buscando peers _miku._tcp... (Ctrl+C para salir)" << std::endl;
    }

    int sig = 0;
    sigwait(&sigs, &sig);

    stopping = true;
    discovery->stop();
    worker.join();

    std::cout << "Saliendo." << std::endl;
    return 0;
}