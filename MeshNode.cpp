
#include "MeshNode.hpp"
#include "Utils.hpp"
#include <random>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace lot49;

//
// simulation parameters
//

int MeshNode::sSeed = 0; 
double MeshNode::sGatewayPercent = 0.2; // percent of nodes that are also internet gateways
double MeshNode::sOriginatingPercent = 1.0;
int MeshNode::sMaxSize = 3500; // meters width
double MeshNode::sMoveRate = 85; // meters per minute
int MeshNode::sPauseTime = 5; // minutes of simulation
int MeshNode::sCurrentTime = 0; // minutes of simulation
int MeshNode::sPayloadSize = 50; // bytes
int MeshNode::sRadioRange = 1600; // radio communication range in meters

std::vector<lot49::MeshNode> MeshNode::sNodes;
std::list<lot49::MeshRoute> MeshNode::sRoutes;
std::string MeshNode::sParametersString;

//static std::default_random_engine rng(std::random_device{}());
static std::default_random_engine rng(0); // deterministic random seed

namespace lot49
{

void MeshNode::WriteStats(const std::string& inLabel, const lot49::MeshMessage& inMessage)
{
    MeshNode& sender = MeshNode::FromHGID(inMessage.mSender);
    MeshNode& receiver = MeshNode::FromHGID(inMessage.mReceiver);
    double tx_distance = Distance(sender.mCurrentPos, receiver.mCurrentPos);
 
    // time
    _stats << sCurrentTime << ", ";
    // label
    _stats << inLabel << ", ";
    // sender
    _stats << std::hex << std::setw(4) << (int) inMessage.mSender << ", ";    
    // receiver
    _stats << std::hex << std::setw(4) << (int) inMessage.mReceiver << ", ";
    // source
     _stats << std::hex << std::setw(4) << (int) inMessage.mSource << ", ";   
    // destination
    _stats << std::hex << std::setw(4) << (int) inMessage.mDestination << ", ";
    // distance
    _stats << std::dec << (int) tx_distance << ", ";
    // incentive_type
    _stats << inMessage.mIncentive.mType << ", ";
    // prepaid_tokens
    _stats << std::dec << (int) inMessage.mIncentive.mPrepaidTokens << ", ";
    // relay_path_size
    _stats << std::dec << (int) inMessage.mIncentive.mRelayHops << ", ";
    // agg_signature_size - constant 128 for eltoo + classic multisig since always two signatures
    // no signature data on receipts
    if (inMessage.mIncentive.mType == eReceipt1 || inMessage.mIncentive.mType == eReceipt2) {
        _stats << std::dec << 0 << ", ";
    } else {
        _stats << std::dec << (inMessage.mIncentive.mSignature.size() + inMessage.mIncentive.mSecondSignature.size()) << ", ";
    }
    // is_witness
    _stats << (inMessage.mIncentive.mWitness ? "witness" : (inMessage.mIncentive.mType == eSetup1 ? "setup" : "payload")) << ", ";
    // payload_data_size
    // 16 byte preimage on return path
    if (inMessage.mIncentive.mType == eReceipt1 || inMessage.mIncentive.mType == eReceipt2) {
        _stats << std::dec << 16 << ", ";
    } else {
        _stats << std::dec << inMessage.mPayloadData.size() << ", ";
    }
    // receiver_unspent_tokens, receiver_channel_state, receiver_channel_confirmed
    if (receiver.HasChannel(inMessage.mReceiver, inMessage.mSender)) {
        PeerChannel& channel = receiver.GetChannel(inMessage.mReceiver, inMessage.mSender);
        _stats << std::dec << channel.mUnspentTokens << ", ";
        _stats << std::dec << channel.mState << ", ";
        _stats << (channel.mConfirmed ? "true" : "false") << std::endl;
    }
    else {
        _stats << std::dec << "-, -, -" << std::endl;
    }
}

// create mesh nodes
void MeshNode::CreateNodes(const int inCount)
{
    sCurrentTime = 0;
    sSeed = 0;
    sNodes.clear();
    sNodes.resize(inCount); 

    // default witness node
    HGID witness_node = 0xFFFF;

    //std::generate(sNodes.begin(), sNodes.end(), [&] {return MeshNode();});

    // set the last node as a gateway for verifying setup transactions
    int num_gateways = sNodes.size() * sGatewayPercent;
    
    // each node corresponds with one other node
    for (int i = 0; i < inCount; i++) {
        MeshNode& n = MeshNode::FromIndex(i);
        HGID correspondent_node = MeshNode::FromIndex((i+1) % inCount).GetHGID();
        n.SetCorrespondentNode(correspondent_node);

        // gateways evenly spaced in range
        if (num_gateways == 0) {
            n.mIsGateway = false;
        } else if ((i % (inCount/num_gateways)) == 0) {
            n.mIsGateway = true;
        }

        _log << n;
        _log << endl;
    }    
}

MeshNode &MeshNode::FromIndex(const int inIndex)
{
    if (inIndex > sNodes.size())
    {
        CreateNodes(inIndex + 1);
    }
    return sNodes[inIndex];
}

//  Lookup a node from a Hashed GID
MeshNode &MeshNode::FromHGID(const HGID &inHGID)
{
    for (auto node = sNodes.begin(); node != sNodes.end(); ++node)
    {
        if (node->GetHGID() == inHGID) {
            return *node;
        }
    }
    throw std::invalid_argument("invalid HGID");
}

MeshNode &MeshNode::FromMultisigPublicKey(const secp256k1_33& inPk)
{
    for (auto node = sNodes.begin(); node != sNodes.end(); ++node)
    {
        if (node->GetMultisigPublicKey() == inPk) {
            return *node;
        }
    }
    throw std::invalid_argument("invalid Public Key");
}
void MeshNode::ClearRoutes() {
    sRoutes.clear();
}

// recursively find shortest route to a node
bool MeshNode::FindRoute(const HGID inDestination, const int inDepth, MeshRoute& ioRoute, std::list<HGID>& ioVisited, double& ioDistance) 
{
    // at destination node
    if (inDestination == GetHGID()) {
        ioRoute.clear();
        ioRoute.push_back(GetHGID());
        return true;
    }

    if (inDepth <= 0) {
        return false;
    }

    /*
    // check cached routes
    for (auto cached_route : sRoutes) {
        if (cached_route.front() == GetHGID() && cached_route.back() == inDestination) {
            // use saved sub-route
            ioRoute.clear();
            ioRoute.insert(ioRoute.begin(), cached_route.begin(), cached_route.end());
            auto next_node = ioRoute.begin();
            std::advance(next_node,1);
            double distance = Distance(mCurrentPos, MeshNode::FromHGID(*next_node).mCurrentPos);
            ioDistance = distance;
            return true;
        }
        if (cached_route.front() == inDestination && cached_route.back() == GetHGID()) {
            // use reverse of saved sub-route
            ioRoute.clear();
            cached_route.reverse();
            ioRoute.insert(ioRoute.begin(), cached_route.begin(), cached_route.end());
            auto next_node = ioRoute.begin();
            std::advance(next_node,1);
            double distance = Distance(mCurrentPos, MeshNode::FromHGID(*next_node).mCurrentPos);
            ioDistance = distance;
            return true;
        }
    }    
    */

    ioVisited.push_back(GetHGID());

    double min_distance = std::numeric_limits<double>::max();
    MeshRoute min_route;
    bool found = false;
    for ( auto& n : sNodes) {
        // skip self
        if (n.GetHGID() == GetHGID()) {
            continue;
        }

        // find shortest distance to destination from nodes within radio range
        double radio_range = Distance(mCurrentPos, n.mCurrentPos);
        if (radio_range > sRadioRange) {
            continue;
        }

        // no loops, skip routes already searched by this node or previous nodes
        if (std::find(ioVisited.begin(), ioVisited.end(), n.GetHGID()) != ioVisited.end()) {
            continue;
        }
         
        // find route from candidate node to destination, depth-first-search
        MeshRoute route;
        double distance = 0;
        int depth = inDepth - 1;
        if (n.FindRoute(inDestination, depth, route, ioVisited, distance) && (distance + radio_range) < min_distance) {
            route.insert(route.begin(), GetHGID());
            min_route = route;
            min_distance = distance + radio_range;
            found = true;
        }
    }

    // remove current node from visited list ??
    ioVisited.pop_back();

    if (found) {
        ioDistance = min_distance;
        ioRoute = min_route;
    }
    return found;
}

void MeshNode::CacheRelay(HGID inSource, HGID inDestination, HGID inSender, HGID inReceiver) {
    uint32_t relaykey = inSource + (inDestination << 16);
    uint32_t relaynodes = inSender + (inReceiver << 16);
    relaystore[relaykey] = relaynodes;
    return;
}

void MeshNode::FetchRelay(HGID inSource, HGID inDestination, HGID &inSender, HGID &inReceiver) {
    uint32_t relaykey = inSource + (inDestination << 16);
    uint32_t relaynodes = relaystore[relaykey];
    inSender = relaynodes & 0x0000FFFF;
    inReceiver = (relaynodes >> 16);
    return;
}

bool MeshNode::IsWithinRange(HGID inNode2)
{
    double distance = Distance(mCurrentPos, MeshNode::FromHGID(inNode2).mCurrentPos);
    return (distance < MeshNode::sRadioRange);
}

HGID MeshNode::GetNextHop(HGID inNode, HGID inDestination, int& outHops)
{
    HGID next_hop;
    if (GetNextHop(inNode, inDestination, next_hop, outHops)) {
        return next_hop;
    }
    throw std::invalid_argument("No route to destination.");
}

bool MeshNode::GetNextHop(HGID inNode, HGID inDestination, HGID& outNextHop, int& outHops)
{
        // next hop along shortest depth-first search route
    double distance = 0;
    MeshNode& node = MeshNode::FromHGID(inNode);
    MeshRoute route;
    std::list<HGID> visited;
    int depth = 5;
    bool found = node.FindRoute(inDestination, depth, route, visited, distance);
    if (found) {
        auto iter = route.begin();
        iter++;
        outNextHop = *iter;
        //_log << "Next Hop: " << std::hex << inNode << " -> " << std::hex << inDestination << " = " << std::hex << outNextHop << endl;

        // only add route if not already added
        if (std::find(sRoutes.begin(), sRoutes.end(), route) == sRoutes.end()) {
            AddRoute(route);
        }
        outHops = route.size();
    }

    return found;
}

void MeshNode::AddGateway(const HGID inNode)
{
    MeshNode::FromHGID(inNode).mIsGateway = true;
}

// configure topology
void MeshNode::AddRoute(MeshRoute inRoute)
{
    // do not add if already added
    if (std::find(sRoutes.begin(), sRoutes.end(), inRoute) != sRoutes.end()) {
        return;
    }
    sRoutes.push_front(inRoute);

    auto route_end = inRoute.begin();
    std::advance(route_end, inRoute.size() - 1);
    for (auto hgid_iter = inRoute.begin(); hgid_iter != route_end; ++hgid_iter) {
        //_log << "Node " << std::hex << *hgid_iter << ", Neighbor " << *(hgid_iter+1) << endl;
        auto neighbor_iter = hgid_iter;
        std::advance(neighbor_iter, 1);
        // propose channels with neighbors (forward)
        FromHGID(*hgid_iter).ProposeChannel(*neighbor_iter);
        // propose channels with neighbors (backwards)
        FromHGID(*neighbor_iter).ProposeChannel(*hgid_iter);
    }
}

bool MeshNode::HasNeighbor(HGID inNode, HGID inNeighbor)
{
    bool found = false;

    // if no simulated coordinates, check routes
    for (auto route = sRoutes.begin(); !found && route != sRoutes.end(); ++route)
    {
        auto node_iter = std::find(route->begin(), route->end(), inNode);
        auto neighbor_iter = std::find(route->begin(), route->end(), inNeighbor);
        if (node_iter != route->end() && neighbor_iter != route->end()) {
            // check if nodes are adjacent
            found = (*(++node_iter) == inNeighbor || *(++neighbor_iter) == inNode);
        }
    }
    return found;
}

// ctor
MeshNode::MeshNode()
{
    mSeed.resize(32);
    // std::generate_n(mSeed.begin(), 32, [&] { return dist(rng); });

    // DEBUG
    std::fill(mSeed.begin(), mSeed.end(), sSeed++);
    secp_seed.fill(sSeed++);
    context_multisig_clean = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    csprng_source = "/dev/urandom";
    serial_pubkeysize = pubkeysize;
    multisigstore.reserve(lot49::MAXRELAYS); // one entry per node we have a channel with, # of sigs should be fixed size
    NewMultisigPublicKey(true); // also initializes the secp context we use
    
    // if no pending channel node or correspondent set, then use same hgid as node
    mPendingChannelNode = GetHGID();
    mCorrespondent = GetHGID();

    mIsGateway = false;
 
    // initialize position and waypoint
    std::uniform_int_distribution<int> pos(-sMaxSize/2, sMaxSize/2);
    mWaypoint.first = pos(rng);
    mWaypoint.second = pos(rng);
    mCurrentPos.first = pos(rng);
    mCurrentPos.second = pos(rng);

    // only pause if at waypoint
    mPausedUntil = 0;
}

// limit of 2^16 unique devices in once mesh - may want to add prefix for region / support more devices
void MeshNode::NewHGID()
{
    secp256k1_32 hashbuf;
    GetSHA256(hashbuf.data(), btc_pk.data(), seckeysize);
    hgid = (hashbuf[0] << 8) + hashbuf[1];
}

HGID MeshNode::GetHGID() const
{
    return hgid;
}
// access serialized public key
const secp256k1_33 MeshNode::GetMultisigPublicKey() const
{
    return btc_pk;
}
// generate & store keypair; initialize global context but then use local ones via cloning (destroy after)?
void MeshNode::NewMultisigPublicKey(bool userandom)
{
    // secp contexts are paired with a key
    context_multisig = secp256k1_context_clone(context_multisig_clean);
    secp256k1_32 seckey;
    secp256k1_pubkey pubkey;
    if (userandom) {
        ReadCSPRNG(csprng_source, reinterpret_cast<char*>(seckey.data()), seckeysize);
    } else {
        memcpy(seckey.data(), secp_seed.data(), seckeysize);
    }
    if (!secp256k1_ec_seckey_verify(context_multisig, seckey.data())) {
        throw std::invalid_argument("Invalid multisig private key");
    }
    if (!secp256k1_ec_pubkey_create(context_multisig, &pubkey, seckey.data())) {
        throw std::invalid_argument("Error deriving pubkey");
    }
    btc_sk = seckey;
    secp256k1_ec_pubkey_serialize(context_multisig, btc_pk.data(), &serial_pubkeysize, &pubkey, SECP256K1_EC_COMPRESSED);
    if (serial_pubkeysize != pubkeysize) {
        throw std::invalid_argument("Error serializing pubkey");
    }
    // using this keypair for node identification
    NewHGID();
}

// access seed used for multisig key
const secp256k1_32 MeshNode::GetSeed() const
{
    secp256k1_32 cloned_secp_seed;
    std::copy(secp_seed.begin(), secp_seed.end(), cloned_secp_seed.begin());
    return cloned_secp_seed;
}

// modify seed for multisig key; assume we want to redo keygen
void MeshNode::SetMultisigSeed(const secp256k1_32 new_seed)
{
    std::copy(new_seed.begin(), new_seed.end(), secp_seed.begin());
}

// compute SHA256 of a set of bytes using libsecp implementation
void GetSHA256(uint8_t* output, const uint8_t* data, size_t len)
{
    secp256k1_sha256 hashstate;
    secp256k1_sha256_initialize(&hashstate);
    secp256k1_sha256_write(&hashstate, static_cast<const unsigned char*>(data), len);
    secp256k1_sha256_finalize(&hashstate, static_cast<unsigned char*>(output));
}

// initiate a payment channel if one doesn't already exist with this neighbor
void MeshNode::ProposeChannel(HGID inNeighbor)
{
    if (!HasNeighbor(GetHGID(), inNeighbor)) {
        assert(0);
        return;
    }

    // has this node already proposed a channel to inNeighbor?
    if (MeshNode::FromHGID(inNeighbor).HasChannel(GetHGID(), inNeighbor)) {
        return;
    }

    _log << "Node " << GetHGID() << ", ";
    _log << "ProposeChannel to " << inNeighbor << endl;
    _log << "Proposing Peer: " << GetHGID() << endl;
 
    if (HasChannel(GetHGID(), inNeighbor)) {
        assert(0);
    }
    else {
        PeerChannel theChannel;
        theChannel.mFundingPeer = inNeighbor;
        theChannel.mProposingPeer = GetHGID();
        theChannel.mUnspentTokens = COMMITTED_TOKENS;
        theChannel.mSpentTokens = 0;
        theChannel.mPromisedTokens = 0;
        theChannel.mLastNonce = 0;
        theChannel.mState = eSetup1;
        theChannel.mConfirmed = false;
    
        mPeerChannels[make_pair(theChannel.mProposingPeer, theChannel.mFundingPeer)] = theChannel;
    }

    MeshMessage theMessage;
    theMessage.mSource = inNeighbor;
    theMessage.mSender = GetHGID();
    theMessage.mReceiver = inNeighbor;
    theMessage.mDestination = inNeighbor;
        
    // initialize incentive aggregate signature by signing refund tx
    theMessage.mIncentive.mType = eSetup1;
    theMessage.mIncentive.mPrepaidTokens = 0;

    std::vector<ImpliedTransaction> theImpliedTransactions = GetTransactions(theMessage);
    secp256k1_64 refund_sig = SignMultisigTransaction(theImpliedTransactions.front());

    std::copy(refund_sig.begin(), refund_sig.end(), theMessage.mIncentive.mSignature.begin());

    WriteStats("Propose_Channel", theMessage);
    SendTransmission(theMessage);
}

// return true if channel exists with this neighbor
bool MeshNode::HasChannel(HGID inProposer, HGID inFunder) const
{
    return (mPeerChannels.find(make_pair(inProposer, inFunder)) != mPeerChannels.end());
}

// get existing channel
PeerChannel& MeshNode::GetChannel(HGID inProposer, HGID inFunder)
{
    auto channel_iter = mPeerChannels.find(make_pair(inProposer, inFunder));
    if (channel_iter == mPeerChannels.end()) {
        throw std::invalid_argument("No channel exists for neighbor.");
    }
    return channel_iter->second;
}

const PeerChannel& MeshNode::GetChannel(HGID inProposer, HGID inFunder) const
{
    auto channel_iter = mPeerChannels.find(make_pair(inProposer, inFunder));
    if (channel_iter == mPeerChannels.end()) {
        throw std::invalid_argument("No channel exists for neighbor.");
    }
    return channel_iter->second;
}

void SavePayloadHash(PeerChannel& ioChannel, const std::vector<uint8_t>& inData)
{
    GetSHA256(&(ioChannel.mPayloadHash[0]), inData.data(), inData.size());
    _log << "\tSave payload hash(" << std::hex << ioChannel.mProposingPeer << ", " << ioChannel.mFundingPeer << "): [";
    for (int v: ioChannel.mPayloadHash) { _log << std::hex << v; }
    _log << "] ";
    _log << endl;   
} 

void SaveWitnessHash(PeerChannel& ioChannel, const std::vector<uint8_t>& inData)
{
    GetSHA256(&(ioChannel.mWitnessHash[0]), inData.data(), inData.size());
    _log << "\tSave witness hash(" << std::hex << ioChannel.mProposingPeer << ", " << ioChannel.mFundingPeer << "): [";
    for (int v: ioChannel.mWitnessHash) { _log << std::hex << v; }
    _log << "] ";
    _log << endl;   
} 

// originate new message
bool MeshNode::OriginateMessage(const HGID inDestination, const std::vector<uint8_t>& inPayload)
{
    std::string payload_text(reinterpret_cast<const char*>(inPayload.data()), inPayload.size());
    _log << "Node " << GetHGID() << ", ";
    _log << "OriginateMessage, Destination: " << inDestination << ", Payload: [" << payload_text << "]" << endl << endl;

    // do not send messages if correspondent is within radio range, no incentive costs for local transmissions
    if (IsWithinRange(inDestination)) {
        _log << "!! within radio range, no incentives !!" << endl;
        return true;
    }

    HGID next_hop;
    int hops;
    if (!GetNextHop(GetHGID(), inDestination, next_hop, hops)) {
        _log << "!! No route found !!" << endl;
        return false;
    }

    MeshMessage theMessage;
    theMessage.mSender = GetHGID();
    theMessage.mReceiver = next_hop;
    theMessage.mSource = GetHGID();
    theMessage.mDestination = inDestination;
    theMessage.mPayloadData = inPayload;

    PeerChannel &theChannel = GetChannel(theMessage.mReceiver, GetHGID());

    //assert(theChannel.mState == eSetup2 || theChannel.mState == eReceipt2 || theChannel.mState == eReceipt1);
    
    if (theChannel.mUnspentTokens < hops) {
         _log << "!! insufficient funds, unspent tokens = " << theChannel.mUnspentTokens << " !!" << endl;
        return false;       
    }

    //theChannel.mUnspentTokens -= hops;
    //theChannel.mSpentTokens += hops;
    theChannel.mPromisedTokens += hops;
    theChannel.mLastNonce += 1;
    theChannel.mState = (theMessage.mDestination == theMessage.mReceiver) ? eNegotiate2 : eNegotiate1;
    
    theMessage.mIncentive.mWitness = false;
    theMessage.mIncentive.mPrepaidTokens = hops;
    theMessage.mIncentive.mSignature = theChannel.mRefundSignature;
    theMessage.mIncentive.mType = theChannel.mState;

    // save a local copy of the payload hash for confirming receipt2 messages
    assert(theMessage.mIncentive.mType < eReceipt1 );
    SavePayloadHash(theChannel, theMessage.mPayloadData);

    UpdateIncentiveHeader(theMessage);

    WriteStats("Originate_Message", theMessage);
    SendTransmission(theMessage);
    return true;
}

// relay a message
void MeshNode::RelayMessage(const MeshMessage& inMessage)
{    
    _log << "Node " << GetHGID() << ", ";
    _log << "RelayMessage: " << inMessage << endl;

    // confirm setup transaction on the blockchain
    PeerChannel &theSenderChannel = GetChannel(GetHGID(), inMessage.mSender);
    if (theSenderChannel.mConfirmed == false) {
        HGID gateway;
        if (!GetNearestGateway(gateway)) {
            _log << "!! No route to gateway !! " << endl;    
        }
        else {
            ConfirmSetupTransaction(inMessage, gateway);
        }
    }
    assert(theSenderChannel.mConfirmed == true);

    // receive payment from sender
    uint8_t received_tokens = (inMessage.mIncentive.mPrepaidTokens - inMessage.mIncentive.mRelayHops);
    //theSenderChannel.mUnspentTokens -= received_tokens;
    //theSenderChannel.mSpentTokens += received_tokens;
    theSenderChannel.mPromisedTokens += received_tokens;
    theSenderChannel.mLastNonce += 1;
    theSenderChannel.mState = inMessage.mIncentive.mType;

    int hops;
    HGID next_hop = GetNextHop(GetHGID(), inMessage.mDestination, hops);

    // TODO: check if payment enough to reach destination

    // pay next hop
    uint8_t spent_tokens = (inMessage.mIncentive.mPrepaidTokens - inMessage.mIncentive.mRelayHops) - 1;
    PeerChannel &theChannel = GetChannel(next_hop, GetHGID());
    //theChannel.mUnspentTokens -= spent_tokens;
    //theChannel.mSpentTokens += spent_tokens;
    theChannel.mPromisedTokens -= spent_tokens;
    theChannel.mLastNonce += 1;
    theChannel.mState = inMessage.mIncentive.mType;
    
    // save a local copy of the payload hash for confirming receipt2 messages
    assert ( inMessage.mIncentive.mType < eReceipt1 );
    if (!inMessage.mIncentive.mWitness) {
        SavePayloadHash(theChannel, inMessage.mPayloadData);
    }
    else {
        SaveWitnessHash(theChannel, inMessage.mPayloadData);
    }

    // new relay message
    MeshMessage outMessage = inMessage;
    outMessage.mSender = GetHGID();
    outMessage.mReceiver = next_hop;
    // RelayCache tag - add entry to cache
    CacheRelay(outMessage.mSource, outMessage.mDestination, inMessage.mSender, outMessage.mReceiver);

    if (outMessage.mReceiver == outMessage.mDestination && outMessage.mIncentive.mType == eNegotiate1 ) {
        // next node is destination node
        outMessage.mIncentive.mType = eNegotiate2;
        theChannel.mState = eNegotiate2;
    }

    UpdateIncentiveHeader(outMessage);

    WriteStats("Relay_Message", outMessage);

    // send message to next hop
    SendTransmission(outMessage);
}

// fund a channel
void MeshNode::FundChannel(const MeshMessage& inMessage)
{    
    _log << "Node " << GetHGID() << ", ";
    _log << "FundChannel: " << inMessage << endl;
    _log << "Proposing Peer: " << inMessage.mSender << endl;
    
    // TODO: check that funds exist, etc.

    if (HasChannel(inMessage.mSender, GetHGID())) {
        assert(0);
    }
    else {
        // create channel entry for peer that proposed the channel
        PeerChannel theChannel;
        theChannel.mFundingPeer = GetHGID();
        theChannel.mProposingPeer = inMessage.mSender;
        theChannel.mUnspentTokens = COMMITTED_TOKENS; // always commit default amount when funding a channel
        theChannel.mSpentTokens = 0;
        theChannel.mPromisedTokens = 0;
        theChannel.mLastNonce = 0;
        theChannel.mState = eSetup2;
        theChannel.mRefundSignature = inMessage.mIncentive.mSignature;
        theChannel.mConfirmed = true;

        // save a local copy of the payload hash for confirming receipt2 messages
        assert ( inMessage.mIncentive.mType < eReceipt1 );
        SavePayloadHash(theChannel, inMessage.mPayloadData);           
   
        mPeerChannels[make_pair(theChannel.mProposingPeer, theChannel.mFundingPeer)] = theChannel;
    }
}

bool MeshNode::VerifySetupTransaction(const MeshMessage& inMessage)
{
    std::vector<ImpliedTransaction> theTransactions = GetTransactions(inMessage);
    if (!VerifyMessage(inMessage)) {
        return false;
    }

    // check transaction type
    assert(theTransactions[1].GetType() == eSetup);
    return true;
}

// receive message
void MeshNode::ReceiveMessage(const MeshMessage& inMessage)
{    
    _log << "Node " << GetHGID() << ", ";
    _log << "ReceiveMessage: " << inMessage << endl;

    /*
    // confirm setup transaction on the blockchain if channel needed to relay or originate a message
    PeerChannel &upstream_channel = GetChannel(GetHGID(), inMessage.mSender);
    if (upstream_channel.mConfirmed == false && inMessage.mIncentive.mType != eSetup1) {
        HGID gateway;
        if (!GetNearestGateway(gateway)) {
            _log << "!! No route to gateway !! " << endl;    
        }
        else {
            ConfirmSetupTransaction(inMessage, gateway);
        }
    }
    assert(upstream_channel.mConfirmed == true);
    */

    // channel for sending return receipt
    PeerChannel& downstream_channel = GetChannel(inMessage.mSender, GetHGID());
 
    // save a local copy of the payload hash for confirming receipt2 messages
    if (!inMessage.mIncentive.mWitness) {
        SavePayloadHash(downstream_channel, inMessage.mPayloadData);
    }
    else {
        SaveWitnessHash(downstream_channel, inMessage.mPayloadData);
    }
    
    // message received and marked for signing witness node ? 
    if (inMessage.mIncentive.mWitness) {
        assert(GetIsGateway());
        // verify the setup transaction on the blockchain
        MeshMessage witness_message;
        witness_message.FromBytes(inMessage.mPayloadData);
        bool valid_setup = VerifySetupTransaction(witness_message);
        if(valid_setup) {
            _log << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction CONFIRMED:" << endl << "\t" << witness_message << endl;
            cout << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction CONFIRMED:" << endl << "\t" << witness_message << endl;
        }
        else {
            _log << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction FAILED:" << endl << "\t" << witness_message << endl;
            cout << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction FAILED:" << endl << "\t" << witness_message << endl;
        }
    }
    else {
        std::string payload_text(reinterpret_cast<const char*>(inMessage.mPayloadData.data()), inMessage.mPayloadData.size());
        _log << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << " received message: [" << payload_text << "] !" << endl;
        cout << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << " received message: [" << payload_text << "] !" << endl;
    }
     
    // send return receipt
    MeshMessage theMessage = inMessage;
    theMessage.mSender = GetHGID();
    theMessage.mReceiver = inMessage.mSender;
    theMessage.mIncentive.mType = eReceipt1;

    // TODO: fix so that UpdateIncentiveHeader does not change mIncentive.mState as a side effect using value of theChannel.mState
    PeerChannel &theChannel = GetChannel(theMessage.mReceiver, GetHGID());
    theChannel.mState = theMessage.mIncentive.mType;
    UpdateIncentiveHeader(theMessage);

    // no need to send payload, hash is cached by nodes
    theMessage.mPayloadData.clear();

    WriteStats("Send_Receipt", theMessage);

    // send proof of receipt to previous hop
    SendTransmission(theMessage);

    // !! update channel state(s) AFTER sending transmission because Verify() looks at current nodes channel state

    // receive remaining tokens from sender
    uint8_t remaining_tokens = (inMessage.mIncentive.mPrepaidTokens - inMessage.mIncentive.mRelayHops);
    PeerChannel &upstream_channel = GetChannel(GetHGID(), inMessage.mSender);
    upstream_channel.mUnspentTokens -= remaining_tokens;
    upstream_channel.mSpentTokens += remaining_tokens;
    upstream_channel.mLastNonce += 1;
    upstream_channel.mState = inMessage.mIncentive.mType;
}

// receive delivery receipt
void MeshNode::RelayDeliveryReceipt(const MeshMessage& inMessage)
{    
    _log << "Node " << GetHGID() << ", ";
    _log << "RelayDeliveryReceipt: " << inMessage << endl;

    // destination node confirms message hash matches
    if (!VerifyMessage(inMessage)) {
        return;
    }

    if (GetHGID() != inMessage.mSource) {
        // relay return receipt
        MeshMessage theMessage = inMessage;
        theMessage.mSender = GetHGID();

        // determine next hop from the relay path, no searching
        // theMessage.mReceiver = GetNextHop(GetHGID(), inMessage.mSource);
        //int outHops{};
        //theMessage.mReceiver = GetNextHop(GetHGID(), inMessage.mSource, outHops);
        // also need to detect wich end of the route we're on here; maybe cache sender/receiver on forward direction
        auto pos_iter = std::find(inMessage.mIncentive.mRelayPath.begin(), inMessage.mIncentive.mRelayPath.end(), GetHGID());
        ptrdiff_t pos = std::distance(inMessage.mIncentive.mRelayPath.begin(), pos_iter);
        HGID theNextHop = (pos > 0 ? theMessage.mIncentive.mRelayPath[pos-1] : inMessage.mSource);
        _log << "Next Hop (relay path): " << std::hex << GetHGID() << " -> " << std::hex << inMessage.mSource << " = " << std::hex << theNextHop << endl;
        theMessage.mReceiver = theNextHop;
        HGID cachedNextHop, cachedPreviousHop;
        FetchRelay(theMessage.mSource, theMessage.mDestination, cachedNextHop, cachedPreviousHop);
        if (cachedPreviousHop != inMessage.mSender) {
            cout << "Sending Delivery Receipt: Sender is not cached previous hop!" << endl;
        }
        if (theNextHop != cachedNextHop) {
            cout << "Sending Delivery Receipt: Receiver is not cached next hop!" << endl;
        }
        theMessage.mReceiver = cachedNextHop;

        // use cached hash of payload, do not resend payload with receipt
        theMessage.mPayloadData.clear();

        // message destination signs before relaying eReceipt1, all others just relay
        assert(theMessage.mIncentive.mType == eReceipt1 || theMessage.mIncentive.mType == eReceipt2);
        theMessage.mIncentive.mType = eReceipt2;
        // no need to call UpdateIncentiveHeader(theMessage) because receipts don't need any extra incentives

        WriteStats("Relay_Receipt", theMessage);

        // send proof of receipt to previous hop
        SendTransmission(theMessage);

        // !! update channel state(s) AFTER sending transmission because Verify() looks at current nodes channel state

        uint8_t prepaid_tokens = inMessage.mIncentive.mPrepaidTokens;

        // credit payment from upstream node and update nonce
        assert (GetHGID() != theMessage.mSource);
        //uint8_t received_tokens = prepaid_tokens - outHops;
        //PeerChannel& upstream_channel = GetChannel(GetHGID(), theMessage.mReceiver);
        uint8_t received_tokens = prepaid_tokens - pos;
        PeerChannel& upstream_channel = GetChannel(GetHGID(), theNextHop);
        upstream_channel.mUnspentTokens -= received_tokens;
        upstream_channel.mSpentTokens += received_tokens;
        upstream_channel.mPromisedTokens -= received_tokens;
        upstream_channel.mLastNonce += 1;
        upstream_channel.mState = eReceipt2;
        
        //  credit payment to downstream node from this node
        assert (GetHGID() != theMessage.mDestination);
        uint8_t spent_tokens = received_tokens - 1;
        PeerChannel& downstream_channel = GetChannel(inMessage.mSender, GetHGID());
        downstream_channel.mUnspentTokens -= spent_tokens;
        downstream_channel.mSpentTokens += spent_tokens;
        downstream_channel.mPromisedTokens -= spent_tokens;
        downstream_channel.mLastNonce += 1;
        downstream_channel.mState = eReceipt2;
    }
    else {
        if (inMessage.mIncentive.mWitness) {
            _log << "Confirmation of channel setup received by relay " << std::setw(4) << std::setfill('0') << inMessage.mSource << " from Witness Node " << std::setw(4) << std::setfill('0') << inMessage.mDestination << "!" << endl;
            cout << "Confirmation of channel setup received by relay " << std::setw(4) << std::setfill('0') << inMessage.mSource << " from Witness Node " << std::setw(4) << std::setfill('0') << inMessage.mDestination << "!" << endl;
            
            // pending channel node must have been previously set
            assert(mPendingChannelNode != GetHGID()); 
            PeerChannel& pending_channel = GetChannel(GetHGID(), mPendingChannelNode);
            pending_channel.mConfirmed = true;
            // unset pending channel node
            mPendingChannelNode = GetHGID();
        }
        else {
            _log << "Delivery Receipt received by source " << std::setw(4) << std::setfill('0') << inMessage.mSource << " from message destination " << std::setw(4) << std::setfill('0') << inMessage.mDestination << "!" << endl;
            cout << "Delivery Receipt received by source " << std::setw(4) << std::setfill('0') << inMessage.mSource << " from message destination " << std::setw(4) << std::setfill('0') << inMessage.mDestination << "!" << endl;
            // TODO: keep stats on message delivery here
        }

        // credit payment to first hop from message originator or witness requester
        PeerChannel& downstream_channel = GetChannel(inMessage.mSender, GetHGID());
        uint8_t prepaid_tokens = inMessage.mIncentive.mPrepaidTokens;
        downstream_channel.mUnspentTokens -= prepaid_tokens;
        downstream_channel.mSpentTokens += prepaid_tokens;
        downstream_channel.mPromisedTokens -= prepaid_tokens;
    }
}

// confirm the setup transaction for a payment channel with a witness node (via inGateway) 
void MeshNode::ConfirmSetupTransaction(const MeshMessage& inMessage, const HGID inGateway)
{
    _log << "Node " << GetHGID() << ", ";
    _log << "ConfirmSetupTransaction, Gateway: " << inGateway << ", Message Hash: [" << inMessage << "]" << endl << endl;

    if (GetHGID() == inGateway) {
        // handle special case of confirming transactions when relaying through a gateway
        bool valid_setup = VerifySetupTransaction(inMessage);
        if(valid_setup) {
            _log << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction CONFIRMED:" << endl << "\t" << inMessage << endl;
            cout << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction CONFIRMED:" << endl << "\t" << inMessage << endl;
        }
        else {
            _log << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction FAILED:" << endl << "\t" << inMessage << endl;
            cout << "Node " << std::setw(4) << std::setfill('0') << inMessage.mReceiver << ": setup transaction FAILED:" << endl << "\t" << inMessage << endl;
        }
        PeerChannel& theSenderChannel = GetChannel(GetHGID(), inMessage.mSender);
        theSenderChannel.mConfirmed = valid_setup;
        return;
    }

    // pending channel node should be currently unset
    assert(mPendingChannelNode == GetHGID()); 
    mPendingChannelNode = inMessage.mSender;

    // if this node is not a gateway, send the setup transaction to be confirmed to a nearby gateway node, potentially via relay nodes
    HGID next_hop;
    int hops;
    if (!GetNextHop(GetHGID(), inGateway, next_hop, hops)) {
        _log << "!! No route found !!" << endl;
        return;
    }

    MeshMessage theMessage;
    theMessage.mSender = GetHGID();
    theMessage.mReceiver = next_hop;
    theMessage.mSource = GetHGID();
    theMessage.mDestination = inGateway;

    theMessage.mPayloadData = inMessage.Serialize();

    PeerChannel& theChannel = GetChannel(theMessage.mReceiver, GetHGID());

    // assert(theChannel.mState == eSetup2 || theChannel.mState == eReceipt2 || theChannel.mState == eReceipt1);

    if (theChannel.mUnspentTokens < hops) {
         _log << "!! insufficient funds, unspent tokens = " << theChannel.mUnspentTokens << " !!" << endl;
        return;       
    }

    //theChannel.mUnspentTokens -= hops;
    //theChannel.mSpentTokens += hops;
    theChannel.mPromisedTokens += hops;
    // theChannel.mLastNonce += 1;
    theChannel.mState = (theMessage.mDestination == theMessage.mReceiver ? eNegotiate2 : eNegotiate1);

    theMessage.mIncentive.mWitness = true;
    theMessage.mIncentive.mPrepaidTokens = hops;
    theMessage.mIncentive.mSignature = theChannel.mRefundSignature;
    theMessage.mIncentive.mType = theChannel.mState;

    // save a local copy of the payload hash for confirming receipt2 messages
    assert(theMessage.mIncentive.mType < eReceipt1 );
    SaveWitnessHash(theChannel, theMessage.mPayloadData);

    UpdateIncentiveHeader(theMessage);

    WriteStats("Send_Witness", theMessage);

    SendTransmission(theMessage);
}

/*
bls::Signature MeshNode::GetAggregateSignature(const MeshMessage& inMessage, const bool isSigning) const
{
    const MeshNode& theSigningNode = MeshNode::FromHGID(inMessage.mSender);

    _log << "\t\tNode " << GetHGID() << ", ";
    _log << "\t\tGetAggregateSignature, " << inMessage << endl;

    // calculate aggregation info from implied transaction hashes and signing public keys  
    std::vector<ImpliedTransaction> theImpliedTransactions = GetTransactions(inMessage);
    vector<bls::Signature> sigs;

    std::deque< std::deque<bls::AggregationInfo> > aggregation_queue(1);
    bls::PublicKey sender_pk = MeshNode::FromHGID(inMessage.mSender).GetPublicKey();
    bool isOtherSigner = !isSigning;
    bool isSkip = inMessage.mIncentive.mType < eReceipt1;
    bool isSenderSigned = false;
    ImpliedTransaction previous_aggregated_tx;
    for (auto tx = theImpliedTransactions.rbegin(); tx != theImpliedTransactions.rend(); tx++) {
        bls::PublicKey tx_signer_pk = tx->GetSigner();
        
        isSkip &= (tx_signer_pk != sender_pk && !isSenderSigned);

        if (!isSkip) {
            // only add aggregation_info for previously aggregated signatures
            isOtherSigner |= tx_signer_pk != theSigningNode.GetPublicKey() || tx->GetType() == eRefund;

            if (isOtherSigner) {
                if (!aggregation_queue.empty() && tx->GetSigner() != previous_aggregated_tx.GetSigner()) {
                    // group current aggregation information when signer changes
                    aggregation_queue.push_front(std::deque<bls::AggregationInfo>());
                    _log << "\t\t\t---------- " << endl;
                }
                aggregation_queue.front().push_front( bls::AggregationInfo::FromMsgHash(tx_signer_pk, tx->GetHash().data()) );
                previous_aggregated_tx = *tx;
            }
            else {
                // push current signature contained in the message
                sigs.insert( sigs.begin(), theSigningNode.SignTransaction(*tx) );
            }
        }

        // after sender has signed their transactions, skip any later transactions signed by other nodes
        isSenderSigned |= (tx_signer_pk == theSigningNode.GetPublicKey());

        _log << "\t\tSigner: " << MeshNode::FromPublicKey(tx_signer_pk).GetHGID() << " Type: " << tx->GetType() << (isSkip ? "- " : (isOtherSigner ? "* " : " "));
        _log << "\t\t tx: [";
        for (int v: tx->GetHash()) { _log << std::setfill('0') << setw(2) << std::hex << v; }
        _log << "\t\t] ";
        _log << endl;
    }

    // add aggregation info for destination's signature for payload message
    if (inMessage.mIncentive.mType >= eReceipt2 || (inMessage.mIncentive.mType == eReceipt1 && !isSigning)) {
        const MeshNode& destination = MeshNode::FromHGID(inMessage.mDestination);
        bls::PublicKey pk = destination.GetPublicKey();

        HGID direction_hgid = inMessage.mDestination;
        if (GetHGID() == direction_hgid) {
            // sending receipt back to source
            direction_hgid = inMessage.mSource;
        }
        int hops;
        HGID next_hop_hgid = GetNextHop(GetHGID(), direction_hgid, hops);

        const PeerChannel& theChannel = GetChannel( next_hop_hgid, GetHGID());

        std::vector<uint8_t> hash = theChannel.mPayloadHash;
        if (inMessage.mIncentive.mWitness) {
            hash = theChannel.mWitnessHash;
        }

        _log << endl << "\t\tSigner: " << inMessage.mDestination << " Type: sign_payload* ("<< std::hex << theChannel.mProposingPeer << ", " << theChannel.mFundingPeer << ") ";
        _log << "hash: [";
        for (int v: hash) { _log << std::hex << v; }
        _log << "] ";
        _log << endl;

        aggregation_queue.back().push_back( bls::AggregationInfo::FromMsgHash(pk, hash.data()));
    }

    _log << endl;

    // combine and group aggregation info in the same way as when the original signature was created
    bls::AggregationInfo merged_aggregation_info = bls::AggregationInfo::MergeInfos({aggregation_queue.front().begin(), aggregation_queue.front().end()});
    aggregation_queue.pop_front();
    while (aggregation_queue.begin() != aggregation_queue.end()) {
        vector<bls::AggregationInfo> tmp_agg_info = { merged_aggregation_info };
        tmp_agg_info.insert(tmp_agg_info.end(), aggregation_queue.front().begin(), aggregation_queue.front().end());
        merged_aggregation_info = bls::AggregationInfo::MergeInfos(tmp_agg_info);
        aggregation_queue.pop_front();
    }
    
    // add aggregate signature from previous transactions
    bls::Signature agg_sig = bls::Signature::FromBytes(inMessage.mIncentive.mSignature.data());
    agg_sig.SetAggregationInfo(merged_aggregation_info);
    sigs.insert(sigs.begin(), agg_sig);

    // message receiver signs the payload data
    if (inMessage.mIncentive.mType == eReceipt1 && isSigning) {
        assert(theSigningNode.GetHGID() == GetHGID());
        assert(inMessage.mSender == inMessage.mDestination);
        sigs.push_back( theSigningNode.SignMessage(inMessage.mPayloadData) );
    }

    // update aggregate signature
    try {
        agg_sig = bls::Signature::AggregateSigs(sigs);
    }
    catch (std::string &e) {
        _log << e << endl;
    }

    _log << "\t\taggregate sig = " << agg_sig << endl;

    return agg_sig;
}
*/
// update signatures on a message by storing received sigs & replacing with new sigs if tx involves this node
std::array<secp256k1_64, 2> MeshNode::UpdateMultisigSignatures(const MeshMessage& inMessage, const bool isSigning)
{
    const MeshNode& theSigningNode = MeshNode::FromHGID(inMessage.mSender);

    _log << "\t\tNode " << GetHGID() << ", ";
    _log << "\t\tUpdateMultisigSignatures, " << inMessage << endl;

    std::vector<ImpliedTransaction> theImpliedTransactions = GetTransactions(inMessage);

    // find sender's pk, scan for tx's involving us, decide if we're 1st or 2nd signer, and sign
    // must store signatures relevant to us when relaying, and replace with our own on forward path 
    secp256k1_33 sender_pk = MeshNode::FromHGID(inMessage.mSender).GetMultisigPublicKey();
    bool isOtherSigner = !isSigning;
    bool isSkip = inMessage.mIncentive.mType < eReceipt1;
    bool isSenderSigned = false;
    ImpliedTransaction previous_tx;
    std::array<secp256k1_64, 2> sigs; // for sigs we are attaching to tx's
    for (auto tx = theImpliedTransactions.rbegin(); tx != theImpliedTransactions.rend(); tx++) {
        secp256k1_33 tx_signer_pk = tx->GetMultisigSigner();

        // filter to only relevant transactions
        isSkip &= (tx_signer_pk != sender_pk && !isSenderSigned);

        // everything not a receipt or close (strip & save signatures)
        if (!isSkip) {
            // check if we are the 1st or 2nd signer
            isOtherSigner |= (tx_signer_pk != theSigningNode.GetMultisigPublicKey() || tx->GetType() == eRefund);

            if (isOtherSigner) {
                // save existing sigs on tx (to update channel)
                _log << "\t\t\t---------- " << endl;
                multisigstore[tx_signer_pk].push_front(inMessage.mIncentive.mSignature);
                multisigstore[tx_signer_pk].push_front(inMessage.mIncentive.mSecondSignature);
                previous_tx = *tx;
            }
            else {
                // add our sigs to the tx (refund and settlement, still need to modify tx)
                sigs[0] = SignMultisigTransaction(*tx);
                sigs[1] = SignMultisigTransaction(*tx);
            }
        }

        // after sender has signed their transactions, skip any later transactions signed by other nodes
        isSenderSigned |= (tx_signer_pk == theSigningNode.GetMultisigPublicKey());

        _log << "\t\tSigner: " << MeshNode::FromMultisigPublicKey(tx_signer_pk).GetHGID() << " Type: " << tx->GetType() << (isSkip ? "- " : (isOtherSigner ? "* " : " "));
        _log << "\t\t tx: [";
        for (int v: tx->GetHash()) { _log << std::setfill('0') << setw(2) << std::hex << v; }
        _log << "\t\t] ";
        _log << endl;
    }

    // add aggregation info for destination's signature for payload message
    if (inMessage.mIncentive.mType >= eReceipt2 || (inMessage.mIncentive.mType == eReceipt1 && !isSigning)) {
        const MeshNode& destination = MeshNode::FromHGID(inMessage.mDestination);
        secp256k1_33 pk = destination.GetMultisigPublicKey();

        HGID direction_hgid = inMessage.mDestination;
        if (GetHGID() == direction_hgid) {
            // sending receipt back to source
            direction_hgid = inMessage.mSource;
        }
        int hops;
        HGID next_hop_hgid = GetNextHop(GetHGID(), direction_hgid, hops);

        const PeerChannel& theChannel = GetChannel( next_hop_hgid, GetHGID());

        secp256k1_32 hash = theChannel.mPayloadHash;
        if (inMessage.mIncentive.mWitness) {
            hash = theChannel.mWitnessHash;
        }

        _log << endl << "\t\tSigner: " << inMessage.mDestination << " Type: sign_payload* ("<< std::hex << theChannel.mProposingPeer << ", " << theChannel.mFundingPeer << ") ";
        _log << "hash: [";
        for (int v: hash) { _log << std::hex << v; }
        _log << "] ";
        _log << endl;
    }

    _log << endl;

    return sigs;
}

// secp256k1 tag
// compute implied transaction from message and peer channel
std::vector<ImpliedTransaction> MeshNode::GetTransactions(const MeshMessage& inMessage)
{
    std::vector<ImpliedTransaction> theTransactions;
    const L49Header& incentive = inMessage.mIncentive;
    uint8_t prepaid_tokens = incentive.mPrepaidTokens;
    const MeshNode& destination = MeshNode::FromHGID(inMessage.mDestination);
    vector<uint8_t> message_hash(hashsize, 0);
    const MeshNode& source = MeshNode::FromHGID(inMessage.mSource);
    const MeshNode& sender = MeshNode::FromHGID(inMessage.mSender);

    HGID first_relay_hgid = inMessage.mDestination;
    //int outHops{};
    //if (incentive.mRelayHops != 0) {    // path too long for first relay to be direct sender or receiver
    //    first_relay_hgid = GetNextHop(inMessage.mSource, inMessage.mSender, outHops);
    //}
    // seems like we'd need to store the first relay in addition to sender/receiver & source/destination to remove relayPath
    if (incentive.mRelayHops != 0) {    // path too long for first relay to be direct sender or receiver
        first_relay_hgid = incentive.mRelayPath.front();
    }
    if (inMessage.mSender == inMessage.mSource) {
        first_relay_hgid = inMessage.mReceiver;
    }
    else if (inMessage.mReceiver == inMessage.mSource) {
        first_relay_hgid = inMessage.mSender;
    }
    const MeshNode& first_relay = MeshNode::FromHGID(first_relay_hgid);

    GetSHA256(&message_hash[0], inMessage.mPayloadData.data(), inMessage.mPayloadData.size());

    // TODO: find a unique unspent output for this channel instead of using a default UTXO based on senders public key
    ImpliedTransaction issued_value_tx = ImpliedTransaction::MultisigIssue(source.GetMultisigPublicKey(), COMMITTED_TOKENS);
    ImpliedTransaction setup_tx = ImpliedTransaction::MultisigSetup(issued_value_tx, source.GetMultisigPublicKey(), first_relay.GetMultisigPublicKey(), COMMITTED_TOKENS);

    _log << "\t\t\tGetTransactions, Type: " << incentive.mType << endl;

    if (incentive.mType >= eSetup1) {
        // create refund tx, first relay signs and source signs only if first negotiate fails
        theTransactions.push_back( ImpliedTransaction::MultisigRefund(setup_tx, source.GetMultisigPublicKey(), first_relay.GetMultisigPublicKey(), first_relay.GetMultisigPublicKey(), COMMITTED_TOKENS) );
    }
    if (incentive.mType >= eSetup2) {
        // TODO: sender only adds second signature for refund if first negotiate transaction fails
        // theTransactions.push_back( ImpliedTransaction::Refund(setup_tx, source.GetPublicKey(), first_relay.GetPublicKey(), source.GetPublicKey(), COMMITTED_TOKENS) );
        
        // create funding tx
        theTransactions.push_back( setup_tx );
    }
    if (incentive.mType >= eNegotiate1) {       
        // append current node to path
        vector<HGID> path = incentive.mRelayPath;
        //uint8_t impliedHops{};

        if (incentive.mType < eNegotiate2 ) {
            //impliedHops++; would increment relayHops but we're not modifying the message
            path.push_back(inMessage.mReceiver);
        }

        ImpliedTransaction last_update_tx = setup_tx;
        HGID hgid1 = inMessage.mSource;
        for (auto hgid2 = path.begin(); hgid2 != path.end(); ++hgid2) {
            const MeshNode& sender = MeshNode::FromHGID(hgid1);
            const MeshNode& receiver = MeshNode::FromHGID(*hgid2);

            // determine current balance between nodes
            const PeerChannel &theChannel = sender.GetChannel(sender.GetHGID(), receiver.GetHGID());
            
            // create update and settle tx, sender signs and then receiver signs
            theTransactions.push_back( ImpliedTransaction::MultisigUpdateAndSettle(last_update_tx, sender.GetMultisigPublicKey(), receiver.GetMultisigPublicKey(), sender.GetMultisigPublicKey(),
                theChannel.mUnspentTokens - prepaid_tokens, theChannel.mSpentTokens + prepaid_tokens, destination.GetMultisigPublicKey(), message_hash));

            // TODO: receiver should only adds the second signature to the state update if they want to checkpoint the update on the blockchain 
            theTransactions.push_back( ImpliedTransaction::MultisigUpdateAndSettle(last_update_tx, sender.GetMultisigPublicKey(), receiver.GetMultisigPublicKey(), receiver.GetMultisigPublicKey(),
                theChannel.mUnspentTokens - prepaid_tokens, theChannel.mSpentTokens + prepaid_tokens, destination.GetMultisigPublicKey(), message_hash));
            
            _log << endl << "\t\t\t public key: ";
            for (int byte: sender.GetMultisigPublicKey()) { _log << std::setfill('0') << std::setw(2) << std::hex << byte; };
            _log << ", tokens: " << (int) prepaid_tokens << ", sender:" << sender.GetHGID() << ", receiver:" << receiver.GetHGID() << " tx: [";
            for (int v: theTransactions.back().GetHash()) { _log << std::setfill('0') << setw(2) << std::hex << v; }
            _log << "] " << endl;

            hgid1 = *hgid2;
            prepaid_tokens--;
            last_update_tx = theTransactions.back();
        }
    }
    if (incentive.mType >= eNegotiate2) {

        // last incentive is from penultimate relay to destination; need to detect which side of the route we're on
        HGID penultimate_node = incentive.mRelayPath.empty() ? inMessage.mSource : incentive.mRelayPath.back();
        const MeshNode& sender = MeshNode::FromHGID(penultimate_node);
        const MeshNode& receiver = MeshNode::FromHGID(inMessage.mDestination);

        // determine current balance between nodes
        const PeerChannel &theChannel = sender.GetChannel(receiver.GetHGID(), sender.GetHGID());

        // create update and settle tx for delivery to destination node, sender signs and then destination signs
        theTransactions.push_back( ImpliedTransaction::MultisigUpdateAndSettle(theTransactions.back(), sender.GetMultisigPublicKey(), receiver.GetMultisigPublicKey(), sender.GetMultisigPublicKey(),
            theChannel.mUnspentTokens - prepaid_tokens, theChannel.mSpentTokens + prepaid_tokens, destination.GetMultisigPublicKey(), message_hash));
        theTransactions.push_back( ImpliedTransaction::MultisigUpdateAndSettle(theTransactions.back(), sender.GetMultisigPublicKey(), receiver.GetMultisigPublicKey(), destination.GetMultisigPublicKey(),
            theChannel.mUnspentTokens - prepaid_tokens, theChannel.mSpentTokens + prepaid_tokens, destination.GetMultisigPublicKey(), message_hash));
    }
    
    return theTransactions;
}

void MeshNode::UpdateIncentiveHeader(MeshMessage& ioMessage)
{
    _log << "\tNode " << GetHGID() << ", ";
    _log << "\tUpdateIncentiveHeader " << ioMessage << endl;

    L49Header &incentive = ioMessage.mIncentive;
    const PeerChannel &theChannel = GetChannel(ioMessage.mReceiver, GetHGID());

    incentive.mType = theChannel.mState;

    // no need to update the incentive header when relaying delivery receipt
    assert(incentive.mType != eReceipt2);

    if (incentive.mSignature.empty() || incentive.mSecondSignature.empty()) {
        throw std::invalid_argument("invalid MeshMessage: L49Header is missing both signatures");
    }    
    
    if ((theChannel.mState == eNegotiate1 || theChannel.mState == eNegotiate2) && ioMessage.mSender != ioMessage.mSource) 
    {
        // add relay node to path
        incentive.mRelayHops++;
        incentive.mRelayPath.push_back(ioMessage.mSender);
    }

    // relay should store the relevant signatures, and add theirs on the path back to the sender
    std::vector<ImpliedTransaction> theImpliedTransactions = GetTransactions(ioMessage);
    std::array<secp256k1_64, 2> new_sigs = UpdateMultisigSignatures(ioMessage, true);
    
    if (!VerifyMessage(ioMessage)) {
        return;
    }     

    std::copy(new_sigs[0].begin(), new_sigs[0].end(), incentive.mSignature.begin());
    std::copy(new_sigs[1].begin(), new_sigs[1].end(), incentive.mSecondSignature.begin());
}
// use as wrapper for SignMultisig; log, calc. hash, and pass on
secp256k1_64 MeshNode::SignMultisigTransaction(const ImpliedTransaction& inTransaction)
{
    std::vector<uint8_t> msg = inTransaction.Serialize();
    secp256k1_32 msg32 = inTransaction.GetHash();
    _log << "\tNode " << GetHGID() << ", ";
    _log << "\tSignTransaction, Tx Type: " << inTransaction.GetType() << " tx data: [";
    for (int v: msg) { _log << std::setfill('0') << setw(2) << std::hex << v; }
    _log << "]" << endl;
    _log << " tx hash: [";
    for (int v: msg32) { _log << std::setfill('0') << setw(2) << std::hex << v; }
    _log << "]" << endl;
    _log << "\t\tTx Signer PK: ";
    for (int v: inTransaction.GetMultisigSigner()) { _log << std::setfill('0') << setw(2) << std::hex << v; }
    _log << endl;

    return SignMultisig(msg32);
}

secp256k1_64 MeshNode::SignMultisigMessage(const std::vector<uint8_t>& inPayload)
{
    _log << "\tNode " << GetHGID() << ", SignMessage: size = " << std::dec << inPayload.size() << " [";
    for (int v: inPayload) {
        if ( std::isprint(v)) {
            _log << (char) v;
        }
        else {
            _log << std::hex << v;
        }
    }
    _log << "]" << endl;

    secp256k1_32 msg32;
    GetSHA256(msg32.data(), inPayload.data(), inPayload.size());

    _log << "\tSigner: " << GetHGID() << " Type: sign_payload hash: [";
    for (int v: msg32) { _log << std::hex << v; }
    _log << "] ";
    _log << endl;

    return SignMultisig(msg32);
}

secp256k1_64 MeshNode::SignMultisig(const secp256k1_32 msg32)
{
    secp256k1_ecdsa_signature newsigraw;
    secp256k1_64 newsigcompact;
    if (!secp256k1_ecdsa_sign(context_multisig, &newsigraw, msg32.data(), btc_sk.data(), NULL, NULL)) {
        throw std::invalid_argument("Error signing with multisig key");
    }
    if (!secp256k1_ecdsa_signature_serialize_compact(context_multisig, newsigcompact.data(), &newsigraw)) {
        throw std::invalid_argument("Error serializing sig");
    }
    return newsigcompact;
}

secp256k1_64 MeshNode::TestSignMultisigMessage(const std::vector<uint8_t>& inPayload)
{
    return SignMultisigMessage(inPayload);
}

// transmit message
void MeshNode::SendTransmission(const MeshMessage& inMessage)
{
    _log << "Node " << GetHGID() << ", ";
    _log << "<< SendTransmission: " << inMessage << endl << endl;
    double tx_distance = Distance(mCurrentPos, MeshNode::FromHGID(inMessage.mReceiver).mCurrentPos);
    _log << "\tDistance = " << tx_distance << " meters" << endl;
    
    if (!VerifyMessage(inMessage)) {
        return;
    }
    MeshNode& receiver = MeshNode::FromHGID(inMessage.mReceiver);
    receiver.ReceiveTransmission(inMessage);
}

// receive message
void MeshNode::ReceiveTransmission(const MeshMessage &inMessage)
{
    _log << "Node " << GetHGID() << ", ";
    _log << ">> ReceiveTransmission." << endl;
    MeshMessage theMessage = inMessage;

    // check that aggregate signature is valid
    if (!VerifyMessage(inMessage)) {
        return;
    }

    // handle setup1 (fund channel)
    if (theMessage.mIncentive.mType == eSetup1) {
        FundChannel(theMessage);
        return;
    }

    // handle receipt (relay receipt)
    if (theMessage.mIncentive.mType == eReceipt1 || theMessage.mIncentive.mType == eReceipt2) {
        RelayDeliveryReceipt(theMessage);
        return;
    }

    // handle negotiate 1 (relay message)
    if (theMessage.mDestination != GetHGID()) {
        RelayMessage(theMessage);
        return;
    }
    else {
        ReceiveMessage(theMessage);
        return;
    }

    // unhandled message
    assert(false);
}

// check that aggregate signature is valid
bool MeshNode::VerifyMessage(const MeshMessage& inMessage)
{
    _log << "\tNode " << GetHGID() << ", ";
    _log << "\tVerifyMessage: " << inMessage << endl;

    // for checking, just check correctness of L49 Headerand the sender's(?) sig
    // Adapt GetMultisigSignature from GetAggregate and then use secp to verify the sig(s)
    std::array<secp256k1_64, 2> new_sigs = UpdateMultisigSignatures(inMessage, false);
    // DEBUG
    // Need keys of signers here, which is not always the same as the current node
    //if (!new_sigs[0].VerifyMultisig() || !new_sigs[1].VerifyMultisig()) {
    //    _log << "\tVerify Failed!" << endl;
    //    assert(0);
    //    return false;
    //} 
    return true;
}

// Check a secp256k1 signature
bool MeshNode::VerifyMultisig(const secp256k1_33 pubkey, const secp256k1_64 sig, const secp256k1_32 msg32)
{
    secp256k1_pubkey native_pk;
    secp256k1_ecdsa_signature native_sig;
    if (!secp256k1_ec_pubkey_parse(context_multisig, &native_pk, pubkey.data(), pubkeysize)) {
        throw std::invalid_argument("Error parsing multisig pubkey");
    }
    if (pubkeysize != serial_pubkeysize) {
        throw std::invalid_argument("Length error parsing multisig pubkey");
    }
    if (!secp256k1_ecdsa_signature_parse_compact(context_multisig, &native_sig, sig.data())){
        throw std::invalid_argument("Error parsing sig");
    }
    if (!secp256k1_ecdsa_verify(context_multisig, &native_sig, msg32.data(), &native_pk)) {
        throw std::invalid_argument("Error verifying sig");
    }
    return true;
}

bool MeshNode::TestVerifyMultisig(const secp256k1_33 pubkey, const secp256k1_64 sig, const secp256k1_32 msg32)
{
    return VerifyMultisig(pubkey, sig, msg32);
}

bool MeshNode::GetNearestGateway(HGID& outGateway)
{
    // node already a gateway?
    if (mIsGateway) {
        outGateway = GetHGID();
        return true;
    }

    double min_distance = std::numeric_limits<double>::max();
    bool route_found = false;
    for (auto& n : sNodes) {
        // search routes to  gateways
        if (!n.GetIsGateway()) {
            continue;
        }
        std::list<HGID> visited;
        double distance;
        MeshRoute route;
        HGID hgid = n.GetHGID();
        int depth = 5;
        bool found = FindRoute(hgid, depth, route, visited, distance);
        if (found && distance < min_distance) {
            outGateway = hgid;
            route_found = true;
            min_distance = distance;
        }
    }
    return route_found;
}

// set corresondent node
void MeshNode::SetCorrespondentNode(const HGID inHGID)
{
    mCorrespondent = inHGID;
}

HGID MeshNode::GetCorrespondentNode() const
{
    return mCorrespondent;
}

// is gateway node?
bool MeshNode::GetIsGateway() const
{
    return mIsGateway;
}

// compute serialization of the L49Header for Witness verification
std::vector<uint8_t> L49Header::Serialize() const
{
    uint32_t offset = 0;
    std::vector<uint8_t> buf(4 + mRelayPath.size()*sizeof(HGID) + 2*sigsize);
    buf[offset++] = mWitness ? 0x1 : 0x0;
    buf[offset++] = (uint8_t) mType;
    buf[offset++] = mPrepaidTokens;
    uint8_t relay_path_size = mRelayHops;
    assert(relay_path_size < 10);
    buf[offset++] = relay_path_size;
    std::copy((uint8_t*) &mRelayPath[0], (uint8_t*) (&mRelayPath[0]) + relay_path_size*sizeof(HGID), &buf[offset]);
    offset += mRelayPath.size()*sizeof(HGID);
    std::copy(&mSignature[0], &mSignature[0] + sigsize, &buf[offset]);
    offset += sigsize;
    std::copy(&mSecondSignature[0], &mSecondSignature[0] + sigsize, &buf[offset]);
    offset += sigsize;
    assert(offset == buf.size());

    return buf;
}

// compute serialization of the Mesh Message for Witness verification
std::vector<uint8_t> MeshMessage::Serialize() const
{
    std::vector<uint8_t> incent_buf = mIncentive.Serialize();
    std::vector<uint8_t> buf(4 * sizeof(HGID) + 1 + incent_buf.size() + 2 + mPayloadData.size());

    uint32_t offset = 0;
    std::copy((char*) &mSender, (char*)(&mSender) + sizeof(mSender), &buf[offset]);
    offset += sizeof(mSender);
    std::copy((char*) &mReceiver, (char*)(&mReceiver) + sizeof(mReceiver), &buf[offset]);
    offset += sizeof(mReceiver);
    std::copy((char*) &mSource, (char*)(&mSource) + sizeof(mSource), &buf[offset]);
    offset += sizeof(mSource);
    std::copy((char*) &mDestination, (char*)(&mDestination) + sizeof(mDestination), &buf[offset]);
    offset += sizeof(mDestination);
    uint8_t incent_size = incent_buf.size();
    buf[offset++] = incent_size;
    std::copy(incent_buf.begin(), incent_buf.end(), &buf[offset]);
    offset += incent_size;
    assert(incent_size <= 255);
    uint16_t payload_size = mPayloadData.size();
    assert(payload_size <= std::numeric_limits<uint16_t>::max());
    std::copy((char*) &payload_size, (char*)(&payload_size) + sizeof(payload_size), &buf[offset]);
    offset += sizeof(payload_size);
    std::copy(mPayloadData.begin(), mPayloadData.end(), &buf[offset]);
    offset += payload_size;
    assert(offset == buf.size());

    return buf;
}

// reconstruct MeshMessage from serialized data
void MeshMessage::FromBytes(const std::vector<uint8_t>& inData) 
{
    uint32_t offset = 0;
    mSender = *reinterpret_cast<const HGID*>(&inData[offset]);
    offset += sizeof(mSender);
    mReceiver = *reinterpret_cast<const HGID*>(&inData[offset]);
    offset += sizeof(mReceiver);
    mSource = *reinterpret_cast<const HGID*>(&inData[offset]);
    offset += sizeof(mSource);
    mDestination = *reinterpret_cast<const HGID*>(&inData[offset]);
    offset += sizeof(mDestination);
    uint8_t incent_size = inData[offset++];
    std::vector<uint8_t> incent_buf(incent_size);
    std::copy(&inData[offset], &inData[offset] + incent_size, &incent_buf[0]);
    mIncentive.FromBytes(incent_buf);
    offset += incent_size;
    uint16_t payload_size;
    payload_size = *reinterpret_cast<const uint16_t*>(&inData[offset]);
    offset += sizeof(payload_size);
    mPayloadData.resize(payload_size);
    std::copy(&inData[offset], &inData[offset] + payload_size, &mPayloadData[0]);
    offset += payload_size;
    assert(offset == inData.size());
}

// reconstruct L49Header from serialized data
void L49Header::FromBytes(const std::vector<uint8_t>& inData) 
{
    uint32_t offset = 0;
    mWitness = (inData[offset++] == 0x1 ? true : false);
    mType = (EChannelState) inData[offset++];
    mPrepaidTokens = inData[offset++];
    uint8_t relay_path_size = inData[offset++];
    mRelayPath.resize(relay_path_size);
    if (relay_path_size > 0) {
        std::copy(&inData[offset], &inData[offset] + relay_path_size * sizeof(HGID), reinterpret_cast<uint8_t*>(&mRelayPath[0]));
        offset += relay_path_size*sizeof(HGID);
    }
    std::copy(&inData[offset], &inData[offset] + sigsize, &mSignature[0]);
    offset += sigsize;
    std::copy(&inData[offset], &inData[offset] + sigsize, &mSecondSignature[0]);
    offset += sigsize;
    assert(offset == inData.size());
}

void MeshNode::ChannelsBalances(int& outInChannels, int& outInBalance, int& outOutChannels, int& outOutChannelBalance) const
{
    outInChannels = outInBalance = outOutChannels = outOutChannelBalance = 0;
    for (const auto& ch : mPeerChannels) {
        if (ch.second.mFundingPeer == GetHGID()) {
            if (ch.second.mSpentTokens > 0) {
                outOutChannels++;
                outOutChannelBalance += ch.second.mSpentTokens;
            }
        }
        else {
            if (ch.second.mSpentTokens > 0) {
                outInChannels++;
                outInBalance += ch.second.mSpentTokens;
            }
        }
    }
}

void MeshNode::PrintTopology() 
{
    for (auto& node1 : sNodes) {
        _topology << sCurrentTime << ", ";
        _topology << std::hex << std::setw(4) << std::setfill('0') << node1.GetHGID() << ", ";
        _topology << std::hex << std::setw(4) << std::setfill('0') << node1.mCorrespondent << ", ";
        _topology << std::dec << (int) Distance(node1.mCurrentPos, MeshNode::FromHGID(node1.mCorrespondent).mCurrentPos) << ", ";
        _topology << std::dec << (int) node1.mCurrentPos.first << ", ";
        _topology << std::dec << (int) node1.mCurrentPos.second << ", ";
        _topology << (node1.mPausedUntil > sCurrentTime ? "true" : "false") << ", ";
        HGID next_hop;
        int hops;
        if (GetNextHop(node1.GetHGID(), node1.GetCorrespondentNode(),next_hop, hops)) {
            _topology << std::hex << std::setw(4) << std::setfill('0') << next_hop << ", ";
        }
        else {
            _topology << "<none> " << ", ";
        }
        int in_channels, received_tokens, out_channels, spent_tokens;
        node1.ChannelsBalances(in_channels, received_tokens, out_channels, spent_tokens);
        _topology << std::dec << in_channels << ", " << std::dec << out_channels << ", ";
        _topology << std::dec << received_tokens << ", " << std::dec << spent_tokens << ", ";
        _topology << std::dec << received_tokens - spent_tokens << endl;        
    }
    
    /*
    for (auto& node1 : sNodes) {
        for (auto& node2 : sNodes) {
            if (node1.GetHGID() != node2.GetHGID()) {
                MeshRoute route;
                std::list<HGID> visited;
                double distance;
                bool found = node1.FindRoute(node2.GetHGID(), route, visited, distance);
                if (found) {
                    _log << std::hex << std::setw(4) << node1.GetHGID() << "->" << std::hex << std::setw(4) << node2.GetHGID() << ", distance = " << (float) distance << ", hops = " << std::dec << route.size() << ", route = ";
                    for (auto hgid : route) {
                        _log << std::hex << std::setw(4) << hgid << ", ";
                    }
                    _log << endl;
                }
            }
        }
    }
    */
}

// update position of all nodes, update routes and send messages
void MeshNode::UpdateSimulation()
{
    // update position of all nodes
    for (auto& n : sNodes) {
        // paused at waypoint
        if (n.mPausedUntil > sCurrentTime) {
            continue;
        }
        // if at waypoint, pick new waypoint and pause
        double distance = Distance(n.mCurrentPos, n.mWaypoint);
        if (distance <= sMoveRate) {
            std::uniform_int_distribution<int> pos(-sMaxSize/2, sMaxSize/2);
            n.mWaypoint.first = pos(rng);
            n.mWaypoint.second = pos(rng);
            n.mPausedUntil = sCurrentTime + sPauseTime;
            distance = Distance(n.mCurrentPos, n.mWaypoint);
        }
        double dX = ((n.mWaypoint.first - n.mCurrentPos.first)/distance) * sMoveRate; // in meters per minute
        double dY = ((n.mWaypoint.second - n.mCurrentPos.second)/distance) * sMoveRate;

        // update position (wrap around instead of moving out of area)
        n.mCurrentPos.first = -sMaxSize/2 + ((int)(sMaxSize/2 + n.mCurrentPos.first + dX) % (int) sMaxSize);
        n.mCurrentPos.second = -sMaxSize/2 + ((int)(sMaxSize/2 + n.mCurrentPos.second + dY) % (int) sMaxSize);
    }

    // update links to nearby nodes
    ClearRoutes();
    for (auto& n1 : sNodes) {
        for (MeshNode& n2 : sNodes) {

            // skip nodes out of range and skip self
            double distance = Distance(n1.mCurrentPos, n2.mCurrentPos);
            if (distance > sRadioRange || n1.GetHGID() == n2.GetHGID()) {
                continue;
            }

            // propose channel to any nearby node
            if (!MeshNode::HasNeighbor(n1.GetHGID(),n2.GetHGID())) {
                MeshRoute route = {n1.GetHGID(), n2.GetHGID()};
                n1.AddRoute(route);
            }
        }
    }

    // log current relative positions of nodes
    PrintTopology();

    // send message to correspondent each round
    int originating = 0;
    int max_originating = sOriginatingPercent * sNodes.size();
    for (auto& n : sNodes) {
        if (originating++ < max_originating && n.GetHGID() != n.mCorrespondent) {
            // send a message and receive delivery receipt
            std::vector<uint8_t> payload(sPayloadSize, 'A');
            if (n.OriginateMessage(n.mCorrespondent, payload)) {
                cout << std::hex << endl << "Node " << std::setw(4) << std::setfill('0') << n.GetHGID() << ": send a message to Node " << std::setw(4) << std::setfill('0') << n.mCorrespondent << endl;
            }
            else {
                cout << std::hex << endl << "Node " << std::setw(4) << std::setfill('0') << n.GetHGID() << ": no route to Node " << std::setw(4) << std::setfill('0') << n.mCorrespondent << endl;
            }
        }
    }

    sCurrentTime++;
}


} // namespace lot49