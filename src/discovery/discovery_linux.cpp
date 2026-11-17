#include <memory>
#include <string>
#include <unordered_map>
#include <neru/discovery_service.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

class AvahiDiscovery : public DiscoveryService {
public:

    explicit AvahiDiscovery(AnnounceInfo i) : info(std::move(i)) {}

    ~AvahiDiscovery() override {
        stopBrowse();
    }

    void startBrowse() override { 
        if(!(poll = avahi_simple_poll_new())){
            fail("Failed to create simple poll object (Avahi).\n");
            return;
        }        

        client = avahi_client_new(avahi_simple_poll_get(poll), AVAHI_CLIENT_NO_FAIL, clientCallback, this, &error);
        
        if(!client){
            fail(std::string("Failed to create client: ") + avahi_strerror(error));
            return;
        }

        avahi_simple_poll_loop(poll);
        cleanup();
    }

    void stopBrowse() override {
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

    void createGroup(){

        if(!group) group = avahi_entry_group_new(client, groupCallback, this);
        if(!group && OnError) { OnError(std::string("Avahi entry group function failed") +
        avahi_strerror(avahi_client_errno(client))); return; }

        if(avahi_entry_group_is_empty(group)){
            int ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, 
            info.serviceName.c_str(), "_miku._tcp", nullptr, nullptr, 
            info.port, ("id=" + info.deviceId).c_str, nullptr);

            if(ret == AVAHI_ERR_COLLISION){
                char* n;

                n = avahi_alternative_service_name(self->info.serviceName.c_str());
                info.serviceName = n;
                avahi_free(n);
                createGroup();
            } else if(ret < 0){if(OnError(avahi_strerror(ret)); return;)}
            
            if((ret = avahi_entry_group_commit(group) < 0 && OnError)
                OnError(avahi_strerror(ret));
        }

    }

    static void groupCallback(AvahiEntryGroup* g, AvahiEntryGroupState state, void* userdata){
        auto* self = static_cast<AvahiDiscovery*>(userdata);
    
        self->group = g;

        switch(state){
            case AVAHI_ENTRY_GROUP_ESTABLISHED:
                if(self->OnAnnounce()) self->OnAnnounce();
                break;
            case AVAHI_ENTRY_GROUP_COLLISION: {
                char* n;

                n = avahi_alternative_service_name(self->info.serviceName.c_str());
                self->info.serviceName = n;
                avahi_free(n);
                self->createGroup();
                break;
            }
            case AVAHI_ENTRY_GROUP_FAILURE:
                if(self->OnError) self->OnError(avahi_strerror(
                    avahi_client_errno(avahi_entry_group_get_client(g))));
                avahi_simple_poll_quit(self->poll);
                break;
            case AVAHI_ENTRY_GROUP_UNCOMMITED:
            case AVAHI_ENTRY_GROUP_REGISTERING:
                break;
        }

    }

    static void clientCallback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
        auto* self = static_cast<AvahiDiscovery*>(userdata);
        self->client = c;

        switch(state){
            case AVAHI_CLIENT_S_RUNNING: {
                
                // Create service browser
                if(self->sb) return;
                self->sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, 
                    AVAHI_PROTO_INET, "_miku._tcp", NULL, (AvahiLookupFlags)0, browseCallback, this);
                if(!self->sb && self->OnError) self->OnError(std::string("Failed to create browser: ") + avahi_strerror(avahi_client_errno(c)));
                
                // Create group
                self->createGroup();
                break;
            }
            case AVAHI_CLIENT_S_COLLISION:
            case AVAHI_CLIENT_S_REGISTERING:
                if(self->group) avahi_entry_group_reset(self->group);
                break;
            case AVAHI_CLIENT_FAILURE:
                if(self->OnError) self->OnError(avahi_strerror(avahi_client_errno(c)));
                avahi_simple_poll_quit(self->poll);
                break;
            case AVAHI_CLIENT_CONNECTING:
                break;
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
                if(peer.deviceId == self->info.deviceId) break;
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
    AvahiEntryGroup* group = nullptr;
    AnnounceInfo info;
    int error = 0;
};

std::unique_ptr<DiscoveryService> createDiscoveryService(){
    return std::make_unique<AvahiDiscovery>(info);
}