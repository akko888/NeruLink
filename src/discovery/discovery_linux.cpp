#include <memory>
#include <string>
#include <unordered_map>
#include <neru/discovery_service.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

class AvahiDiscovery : public DiscoveryService {
public:

    ~AvahiDiscovery() override {
        stop();
    }

    void start() override { 
        if(!(poll = avahi_simple_poll_new())){
            fail("Failed to create simple poll object (Avahi).\n");
            return;
        }        

        client = avahi_client_new(avahi_simple_poll_get(poll), AVAHI_CLIENT_NO_FAIL, clientCallback, this, &error);
        
        if(!client){
            fail(std::string("Failed to create client: ") + avahi_strerror(error));
            return;
        }
            
        if(!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, "_miku._tcp", NULL, (AvahiLookupFlags)0, browseCallback, this))){
            fail(std::string("Failed to create service browser: ") + avahi_strerror(avahi_client_errno(client)));
            return;
        }

        avahi_simple_poll_loop(poll);
        cleanup();
    }

    void stop() override {
        if(poll) avahi_simple_poll_quit(poll);
    }

private:

    void cleanup(){
        if(sb){
            avahi_service_browser_free(sb);
            sb = nullptr;
        }
        if(client){
            avahi_client_free(client);
            client = nullptr;
        }
        if(poll){
            avahi_simple_poll_free(poll);
            poll = nullptr;
        }

    }

    void fail(const std::string& msg){
        if(OnError) OnError(msg);
        cleanup();
    }

    static void clientCallback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
        auto* self = static_cast<AvahiDiscovery*>(userdata);

        if (state == AVAHI_CLIENT_FAILURE && self->OnError){
            self->OnError(avahi_strerror(avahi_client_errno(c)));
            avahi_simple_poll_quit(self->poll);
        }
    }


    static void browseCallback(
        AvahiServiceBrowser *b,
        AvahiIfIndex interface,
        AvahiProtocol protocol,
        AvahiBrowserEvent event,
        const char *name,
        const char *type,
        const char *domain,
        AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
        void* userdata) {
        
        auto* self = static_cast<AvahiDiscovery*>(userdata);
        AvahiClient* c = avahi_service_browser_get_client(b);
        
        /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
        
        switch(event){
            case AVAHI_BROWSER_FAILURE:
            
                if(self->OnError)
                    self->OnError(avahi_strerror(avahi_client_errno(c)));
                avahi_simple_poll_quit(self->poll);
                return;
        
            case AVAHI_BROWSER_NEW:
                if (!(avahi_service_resolver_new
                    (c, interface, protocol, name, type, domain, AVAHI_PROTO_INET, (AvahiLookupFlags)0, resolveCallback, self)) 
                    && self->OnError)

                    self->OnError(std::string("Failed to start resolver for '") + name + "': " + 
                    avahi_strerror(avahi_client_errno(c)));
                break;
        
            case AVAHI_BROWSER_REMOVE: {
                    auto it = self->idByName.find(name);
                    if(it != self->idByName.end()){
                        if(self->OnPeerLost) self->OnPeerLost(it->second);
                        self->idByName.erase(it);
                    }
                break;
            } 

            case AVAHI_BROWSER_ALL_FOR_NOW:
            case AVAHI_BROWSER_CACHE_EXHAUSTED:
                break;
        }
    }

static void resolveCallback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata) {

        auto* self = static_cast<AvahiDiscovery*>(userdata);
    /* Called whenever a service has been resolved successfully or timed out */

    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            if(self->OnError)
                self->OnError(std::string("Failed to resolve '") + name + "': " +
                    avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char addr[AVAHI_ADDRESS_STR_MAX];

            avahi_address_snprint(addr, sizeof(addr), address);

            if(self->OnPeerFound){
                PeerInfo peer;
                if (AvahiStringList* l = avahi_string_list_find(txt, "id")) {
                    char *key, *value; size_t size;
                    if (avahi_string_list_get_pair(l, &key, &value, &size) == 0) {
                        if (value) peer.deviceId = value;
                        avahi_free(key); avahi_free(value);
                    }
                }
                if(peer.deviceId.empty()) break;
                peer.host = addr;
                peer.port = port;
                self->idByName[name] = peer.deviceId;
                self->OnPeerFound(peer);
            }
        }
    }

    avahi_service_resolver_free(r);
}

    std::unordered_map<std::string, std::string> idByName;
    AvahiSimplePoll* poll = nullptr;
    AvahiClient* client = nullptr;
    AvahiServiceBrowser* sb = nullptr;
    int error = 0;
};

std::unique_ptr<DiscoveryService> createDiscoveryService(){
    return std::make_unique<AvahiDiscovery>();
}