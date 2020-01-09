

// Gossip Layer <-> Membership Layer

class NodeAddr {
    private:
        // some form of ip address storage
        short port;
    public:
        NodeAddr(std::string ip, short port);
}

struct Payload {
    const static int MAX_PAYLOAD_SIZE;
    char data[MAX_PAYLOAD_SIZE];

    Payload(std::string data);
    Payload(char *data, int size);
}

enum receiveType {
    
};

class Net {
    public:
        // payload dissemination via gossip protocol
        int Gossip(std::list<NodeAddr> nodeList, Payload data);

        typedef void (*GossipHandler)(const NodeAddr &node, const Payload &data);
        RegisterGossipHandler(GossopHandler handler);

        int Push(const NodeAddr &node, const unsigned char *data, size_t size);

        typedef void (*PullHandler)(const NodeAddr &node, const unsigned char *data, size_t size);
        int Pull(NodeAddr node, const unsigned char *data, size_t size, PullHandler handler);

        typedef void (*OnReceive)(enum receiveType type, const NodeAddr &node, const unsigned char *data, size_t size);
        RegisterOnReceive(OnReceive handler);

        // receipt:
        // typedef void (*PushHandler)(const NodeAddr &node, const unsigned char *data, size_t size);
        // RegisterPushEventHandler(PushHandler handler);

        // receipt:
        // void PullReceiptHandler(NodeAddr node, const unsigned char *data, size_t size);
        // typedef void (*PullReceiptHandler)(const NodeAddr &node, const unsigned char *data, size_t size);
        // RegisterPullReceiptHandler(PullHandler handler);


};

// Membership Layer <-> RPC Layer

namespace node_keeper
{

enum NodeStatus {
    UP,
    SUSPECT,
    DOWN
};

class Member {
    std::string NodeName;
    struct NodeAddr Addr;
    enum NodeStatus Status;
};

struct Config {
    std::vector<Member> seedNodes;
};

class Membership {

    public:
        std::vector<member> getMembers();

        typedef void (*EventHandler)(const std::vector<member> &updatedMembers);

        int RegisterEventHandler(const EventHandler &handler);

        int Init(const struct Config &config);

        // node leave in destructor
        virtual ~Membership();
};


};