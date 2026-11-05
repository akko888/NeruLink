#include <string>
#include <functional>
#include <memory>
#include <cstdint>

struct PeerInfo{
    std::string deviceId;
    std::string host;
    uint16_t port;
};

class DiscoveryService{
public:
    virtual ~DiscoveryService() = default;
    virtual void start() = 0;
    virtual void stop() = 0;

    std::function<void(const PeerInfo&)> OnPeerFound;
    std::function<void(const std::string&)> OnPeerLost;
    std::function<void(const std::string&)> OnError;
};

std::unique_ptr<DiscoveryService> createDiscoveryService();