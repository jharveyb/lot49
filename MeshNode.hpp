#include "ImpliedTransaction.hpp"
#include <list>
#include <array>
#include <memory>
#include <fstream>
#include <unordered_map>
#include <deque>

#pragma once

namespace lot49
{

// Hashed GID
typedef uint16_t HGID;

// Mesh route
typedef std::list<HGID> MeshRoute;

static uint16_t COMMITTED_TOKENS = 2000;

//
// incentive headers
//

enum EChannelState 
{
    eSetup1,
    eSetup2,
    eNegotiate1,
    eNegotiate2,
    eNegotiate3,
    eReceipt1,
    eReceipt2,
    eClose1,
    eClose2
};

struct PeerChannel
{
    HGID mFundingPeer{};
    HGID mProposingPeer{};
    uint16_t mUnspentTokens{};
    uint16_t mSpentTokens{};
    int16_t mPromisedTokens{};
    uint16_t mLastNonce{}; // used to create unique shared 2-of-2 address
    EChannelState mState = eSetup1; // Setup1 -> Setup2 -> Negotiate1 -> Negotiate2 -> Receipt1 -> Receipt2 -> (Close1 -> Close2) or (back to Negotiate1)
    secp256k1_64 mRefundSignature{}; // two sigs for refund & settlement tx's
    secp256k1_64 mSecondRefundSignature{};
    secp256k1_32 mPayloadHash{};
    secp256k1_32 mWitnessHash{};

    bool mConfirmed; // setup tx confirmed    
};

static const uint8_t MAXRELAYS = 5;

struct L49Header
{
    friend std::ostream& operator<<(std::ostream& out, const L49Header& i);

    bool mWitness{}; // witness verification?
    EChannelState mType = eSetup1;
    uint8_t mPrepaidTokens{};
    std::vector<HGID> mRelayPath{}; // lot49::MAXRELAYS
    //uint8_t mRelayHops; // lot49::MAXRELAYS
    secp256k1_64 mSignature{};
    secp256k1_64 mSecondSignature{};

    // used by Witness to verify a transaction chain
    std::vector<uint8_t> Serialize() const;

    // reconstruct from serialized data
    void FromBytes(const std::vector<uint8_t>& inData);
};

//
// Mesh Message
//

struct MeshMessage
{
    friend std::ostream& operator<<(std::ostream& out, const MeshMessage& m);

    // provided by low-level GoTenna meshing protocol / Aspen Grove
    HGID mSender{};
    HGID mReceiver{};
    HGID mSource{};
    HGID mDestination{};
    L49Header mIncentive{};

    // payload, max 236 bytes
    std::vector<uint8_t> mPayloadData{};

    // used by Witness to verify a transaction chain
    std::vector<uint8_t> Serialize() const;

    // reconstruct from serialized data
    void FromBytes(const std::vector<uint8_t>& inData);
};

// custom hashing and comparison for the unordered map of pks->sigs
class ArrayHasher
{
    public:
        size_t operator() (secp256k1_33 const& key) const
        {
            std::string hashstring(key.begin(), key.end());
            return std::hash<std::string>{}(hashstring);
        }
};

class ArrayEqual
{
    public:
        bool operator() (secp256k1_33 const& key1, secp256k1_33 const& key2) const
        {
            return (key1 == key2);
        }
};

// compute SHA256 of a set of bytes using libsecp implementation
void GetSHA256(uint8_t* output, const uint8_t* data, size_t len);

void SavePayloadHash(PeerChannel& ioChannel, const std::vector<uint8_t>& inData);

void SaveWitnessHash(PeerChannel& ioChannel, const std::vector<uint8_t>& inData);

class MeshNode
{
  public:

    // create mesh nodes
    static void CreateNodes(const int inCount);

    static MeshNode &FromIndex(const int inIndex);

    // configure topology
    static void AddRoute(MeshRoute inRoute);
    
    static bool HasNeighbor(HGID inNode, HGID inNeighbor);

    // Lookup a node from a Hashed GID
    static MeshNode &FromHGID(const HGID &inHGID);

    // Lookup a node from a public key
    static MeshNode &FromPublicKey(const bls::PublicKey& inPk);

    static MeshNode &FromMultisigPublicKey(const secp256k1_33& inPk);

    static void ClearRoutes();

    static HGID GetNextHop(HGID inNode, HGID inDestination, int& outHops);

    static bool GetNextHop(HGID inNode, HGID inDestination, HGID& outNextHop, int& outHops);

    static void AddGateway(HGID inNode);    

    static void PrintTopology();

    static void UpdateSimulation();

    static void WriteStats(const std::string& inLabel, const MeshMessage& inMessage);

    static void CloseLogs();

    MeshNode();

    void NewHGID();

    HGID GetHGID() const;

    // access private key
    const bls::PrivateKey GetPrivateKey() const;

    // access or modify seed for the keypair used for multisig
    const secp256k1_32 GetSeed() const;

    // modify seed for multisig key; assume we want to redo keygen
    void SetMultisigSeed(const secp256k1_32 new_seed);

    // access public key
    const bls::PublicKey GetPublicKey() const;

    const secp256k1_33 GetMultisigPublicKey() const;

    void NewMultisigPublicKey(bool userandom);

    // dummy public versions for testing
    secp256k1_64 TestSignMultisigMessage(const std::vector<uint8_t>& inPayload);

    bool TestVerifyMultisig(const secp256k1_33 pubkey, const secp256k1_64 sig, const secp256k1_32 msg32);

    // open a channel with neighbor node
    void ProposeChannel(HGID inNeighbor);

    // originate new message
    bool OriginateMessage(const HGID inDestination, const std::vector<uint8_t> &inPayload);

    // set/get correspondent node
    void SetCorrespondentNode(const HGID inHGID);
    HGID GetCorrespondentNode() const;

    // is gateway node?
    bool GetIsGateway() const;

    bool GetNearestGateway(HGID& outGatewayNode);

    bool IsWithinRange(HGID inNode2);

    friend std::ostream &operator<<(std::ostream &out, const MeshNode &n);  
    static std::string sParametersString;

    static double sGatewayPercent; // percent of nodes that are also internet gateways
    static double sOriginatingPercent; // percent of nodes that originate messages
    static int sMaxSize; // meters width
    static double sMoveRate; // meters per minute
    static int sPauseTime; // minutes of simulation
    static int sPayloadSize; // bytes
    static int sRadioRange; // radio communication range in meters

    static int sCurrentTime; // minutes of simulation

  private:

    // recursively find shortest route to a node
    bool FindRoute(const HGID inDestination, const int inDepth, MeshRoute& ioRoute, std::list<HGID>& ioVisited, double& ioDistance);

    // return true if channel exists with this neighbor
    bool HasChannel(HGID inProposer, HGID inFunder) const;

    // get existing peer channel open with neighbor 
    PeerChannel& GetChannel(HGID inProposer, HGID inFunder);
    const PeerChannel& GetChannel(HGID inProposer, HGID inFunder) const;

    //
    bls::Signature GetAggregateSignature(const MeshMessage& inMessage, const bool isSigning) const;

    std::array<secp256k1_64, 2> UpdateMultisigSignatures(const MeshMessage& inMessage, const bool isSigning);
    // 
    static std::vector<ImpliedTransaction> GetTransactions(const MeshMessage& inMessage);

    // 
    void UpdateIncentiveHeader(MeshMessage& ioMessage);

    // 
    bls::Signature SignTransaction(const ImpliedTransaction& inTransaction) const;

    secp256k1_64 SignMultisigTransaction(const ImpliedTransaction& inTransaction);

    // destination node signs payload 
    bls::Signature SignMessage(const std::vector<uint8_t>& inPayload) const;

    secp256k1_64 SignMultisigMessage(const std::vector<uint8_t>& inPayload);

    secp256k1_64 SignMultisig(const secp256k1_32 msg32);

    // transmit message
    void SendTransmission(const MeshMessage& inMessage);

    // receive message
    void ReceiveTransmission(const MeshMessage& inMessage);

    // check that aggregate signature is valid
    bool VerifyMessage(const MeshMessage &inMessage);

    // Check a secp256k1 signature
    bool VerifyMultisig(const secp256k1_33 pubkey, const secp256k1_64 sig, const secp256k1_32 msg32);

    // relay a message
    void RelayMessage(const MeshMessage& inMessage);

    // fund a channel
    void FundChannel(const MeshMessage& inMessage);

    // receive message
    void ReceiveMessage(const MeshMessage& inMessage);

    // relay delivery receipt
    void RelayDeliveryReceipt(const MeshMessage& inMessage);

    // verify the setup transaction for a payment channel with a witness node (via inGateway)
    void ConfirmSetupTransaction(const MeshMessage& inMessage, const HGID inGateway);

    void ChannelsBalances(int& outInChannels, int& outReceivedTokens, int& outOutChannels, int& outSpentTokens) const;

    bool VerifySetupTransaction(const MeshMessage& inMessage);

    // compute serialization of the Mesh Message for Witness verification
    std::vector<uint8_t> Serialize() const;

    static int sSeed;
    static std::vector<MeshNode> sNodes;

    // routes computed by routing protocol
    static std::list<MeshRoute> sRoutes;

    // state of the channel with a peer (receiving, next hop) node
    std::map<std::pair<HGID,HGID>, PeerChannel> mPeerChannels;

    // nearby node with setup transaction pending witness node confirmation
    HGID mPendingChannelNode;

    // used to create private key
    std::vector<uint8_t> mSeed;
    secp256k1_32 secp_seed;

    // CSPRNG & libsecp-specific objects we can use repeatedly; using dev/urandom
    std::string csprng_source;
    uint16_t hgid;

    size_t serial_pubkeysize;

    // key material used for standard multisig - pair contexts to keys, but clone instead of regenerating
    secp256k1_context* context_multisig_clean;
    secp256k1_context* context_multisig;
    secp256k1_context* context_serdes;
    secp256k1_32 btc_sk;
    secp256k1_33 btc_pk;

    // store signatures for state channel updates from other nodes we have channels with
    std::unordered_map<secp256k1_33, std::deque<secp256k1_64>, ArrayHasher, ArrayEqual> multisigstore;

    // key material used for MuSig
    secp256k1_context* context_musig;
    secp256k1_32 musig_sk;
    secp256k1_33 musig_pk;

    // gateway
    bool mIsGateway;

    // coordinates
    std::pair<double, double> mCurrentPos; // meters from origin
    std::pair<double, double> mWaypoint; // meters from origin
    HGID mCorrespondent;
    int mPausedUntil; // simulation minutes

    friend std::ostream& operator<<(std::ostream& out, const MeshNode& n);
};

}; // namespace lot49