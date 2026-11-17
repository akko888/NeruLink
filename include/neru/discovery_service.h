#include <string>
#include <functional>
#include <memory>
#include <cstdint>

struct PeerInfo{
    std::string deviceId;
    std::string host;
    uint16_t port;
};

struct AnnounceInfo{
    std::string deviceId;
    std::string serviceName;
    uint16_t port;
}

class DiscoveryService{
public:
    virtual ~DiscoveryService() = default;
    virtual void startBrowse() = 0;
    virtual void stopBrowse() = 0;
    virtual void startAnnounce(const std::string& deviceId, uint16_t port) = 0; 

    std::function<void()> OnAnnounce;
    std::function<void(const PeerInfo&)> OnPeerFound;
    std::function<void(const std::string&)> OnPeerLost;
    std::function<void(const std::string&)> OnError;
};

std::unique_ptr<DiscoveryService> createDiscoveryService(const AnnounceInfo& info);