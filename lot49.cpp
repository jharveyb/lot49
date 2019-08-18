#include "MeshNode.hpp"
#include "ImpliedTransaction.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
#include <array>
#include <string>
#include <sstream>

using namespace std;
using namespace lot49;

// TODO: use  48 byte signatures, not 96 byte: https://twitter.com/sorgente711/status/1025451092610433024

bool testMuSig(bool);
void test1();
void test2();
bool testsecp();

void TestRoute(HGID inSender, HGID inReceiver, std::string& inMessage)
{
    // send a message and receive delivery receipt
    cout << std::hex << endl << "Node " << std::setw(4) << std::setfill('0') << inSender << ": send a message to Node " << std::setw(4) << inReceiver << endl; 
    std::vector<uint8_t> data(inMessage.begin(), inMessage.end());
    assert(data.size() == inMessage.size());
    MeshNode::FromHGID(inSender).OriginateMessage(inReceiver, data);
}

int main(int argc, char* argv[]) 
{
    cout << "testMuSig():  " << (testMuSig(true) ? "success!" : "failed!") << endl;

    // test with pre-defined paths
    test1();

    // test randomly moving nodes
    //test2();

    // test libsecp integration
    cout << "test4(), libsecp:  " << (testsecp() ? "success!" : "failed!") << endl;
};

// test integration of libsecp for nodes
bool testsecp()
{
    const size_t NODECOUNT = 2;
    // seeds for secp keypairs
    secp256k1_32 seed = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6, 
                    220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};

    // save keys to compare deterministic vs random; static message for signing
    std::array<secp256k1_33, NODECOUNT> pk_cache{};
    std::array<secp256k1_64, NODECOUNT> sig_cache{};
    const char raw_test_message[seckeysize] = "this_could_be_the_hash_of_a_msg";
    std::vector<uint8_t> vec_test_message{};
    secp256k1_32 array_test_message{};
    memcpy(array_test_message.data(), &raw_test_message[0], seckeysize);
    vec_test_message.resize(seckeysize);
    memcpy(vec_test_message.data(), &raw_test_message[0], seckeysize);

    // Known hash when include the terminating null byte (normal usage)
    string knownhash = "7667b1c4e47d8cb2e9c5b6d5ac39168a3eedfd958bf394375ed26af478b884";
    // Use known good hash from GtkHash (no terminating null byte) - convert to byte array for comparison, testing SHA256
    string gtkknownhash = "e0fb2bdfd9c74844a977407025d310a691aeb39c80600e73051db0eb00ee9acf";
    std::vector<uint8_t> convert_knownhash = HexToBytes(gtkknownhash);
    secp256k1_32 rawgtkknownhash{};
    std::copy(convert_knownhash.begin(), convert_knownhash.end(), rawgtkknownhash.begin());

    cout << endl;
    MeshNode::CreateNodes(NODECOUNT);
    // test keygen and node lookup based on pubkeys
    for (int i = 0; i < NODECOUNT; i++) {
        cout << "HGID - " << MeshNode::FromIndex(i).GetHGID() << endl;
        cout << "Setting seed for multisig keypair..." << endl;
        MeshNode::FromIndex(i).SetMultisigSeed(seed);
        if (seed != MeshNode::FromIndex(i).GetSeed()) {
            cout << "Seed not set or returned correctly!" << endl;
            return false;
        }
        seed[0]++;
        cout << "Keygen with deterministic seeds..." << endl;
        MeshNode::FromIndex(i).NewMultisigPublicKey(false);
        pk_cache[i] = MeshNode::FromIndex(i).GetMultisigPublicKey();

        // check that pubkey-based lookup works
        HGID prev_node;
        secp256k1_33 pk_prev_node;
        if ((i % 2) == 1) {
            prev_node = MeshNode::FromIndex(i-1).GetHGID();
            pk_prev_node = MeshNode::FromIndex(i-1).GetMultisigPublicKey();
            if (prev_node != MeshNode::FromMultisigPublicKey(pk_prev_node).GetHGID()) {
                cout << "Lookup via deterministic pubkey failed!" << endl;
                return false;
            }
        }

        // check keygen with randomness
        cout << "Keygen with random seeds..." << endl;
        MeshNode::FromIndex(i).NewMultisigPublicKey(true);
        MeshNode::FromIndex(i).GetMultisigPublicKey();
        // ideally check that "enough" bits are flipped
        if (pk_cache[i] == MeshNode::FromIndex(i).GetMultisigPublicKey()) {
            cout << "Random keygen same as deterministic!" << endl;
            return false;
        }
        pk_cache[i] = MeshNode::FromIndex(i).GetMultisigPublicKey();
        MeshNode::FromIndex(i).NewMultisigPublicKey(true);
        // compare against the first random key
        if (pk_cache[i] == MeshNode::FromIndex(i).GetMultisigPublicKey()) {
            cout << "Random keygen failed to be random!" << endl;
            return false;
        }
        pk_cache[i] = MeshNode::FromIndex(i).GetMultisigPublicKey();
    }

    // test signing with message wrapper & verifying
    secp256k1_32 signinghash;
    GetSHA256(signinghash.data(), array_test_message.data(), array_test_message.size());
    for (int i = 0; i < NODECOUNT; i++) {
        sig_cache[i] = MeshNode::FromIndex(i).TestSignMultisigMessage(vec_test_message);
        if (!MeshNode::FromIndex(i).TestVerifyMultisig(pk_cache[i], sig_cache[i], signinghash)) {
            cout << "Verifying own signature failed!" << endl;
            return false;
        }
        // tamper with signature; reset later for the next node
        if (sig_cache[i][0] == UINT8_MAX) {
            sig_cache[i][0] = 0;
        } else {
            sig_cache[i][0] = UINT8_MAX;
        }

        // tampering detected via either verification failure (exception) or mismatched pubkeys (can't always check this way in practice)
        try {
            MeshNode::FromIndex(i).TestVerifyMultisig(pk_cache[i], sig_cache[i], signinghash);
        }
        catch (exception& e) {
            cout << "Detected modified signature!" << endl;
        }
        cout << "Successful verification of our own signature!" << endl;
        sig_cache[i] = MeshNode::FromIndex(i).TestSignMultisigMessage(vec_test_message);

        // check signature of the previous node
        if ((i % 2) == 1) {
            if (sig_cache[i-1] == sig_cache[i]) {
                cout << "Same signatures for different pubkeys!" << endl;
                return false;
            }
            if (!MeshNode::FromMultisigPublicKey(pk_cache[i-1]).TestVerifyMultisig(pk_cache[i-1], sig_cache[i-1], signinghash)) {
                cout << "Verifying on signature of another node failed!" << endl;
                return false;
            }
            cout << "Successful verification of another node's signature!" << endl;
        }
    }

    // test standalone functions
    const ustring msg = {reinterpret_cast<const unsigned char*>(raw_test_message)};
    secp256k1_32 hashout;
    GetSHA256(hashout.data(), msg.data(), msg.size());
    cout << "SHA256 of \"this_could_be_the_hash_of_a_msg\": ";
    for ( uint8_t byte : hashout ) { cout << std::setw(2) << std::setfill('0') << static_cast<int>(byte); }
    cout << endl;
    if (hashout != rawgtkknownhash) {
        cout << "Hash does not match result from GtkHash!" << endl;
        return false;
    }
    return true;
}

// test randomly moving nodes
void test2()
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    const size_t MAX_NODES = 30;
    const size_t MAX_TIME = 480; // 8 hrs of simulation
    
     std::vector<int> sides =  {2928}; // {5477, 4472, 3873, 3464, 3162, 2928};
     for (auto side : sides) {
        std::ostringstream oss;
        oss << "results/";
        oss << std::put_time(&tm, "%y%m%d_%H%M%S");        
        oss << "_side-" << side;
        oss << "_nodes-" << MAX_NODES << "_";
        MeshNode::sParametersString = oss.str();
        MeshNode::sMaxSize = side;
        MeshNode::CreateNodes(MAX_NODES);

        for (int i = 0; i < MAX_TIME; i++) {
            MeshNode::UpdateSimulation();
        }
        CloseLogs();
    }
}

// test with pre-defined paths
void test1()
{
    const size_t MAX_NODES = 6; 
    MeshNode::CreateNodes(MAX_NODES);

    // create linear route: A <-> B <-> C <-> D1
    MeshRoute route1;
    for (int i = 0; i < MAX_NODES-2; i++) {
        HGID hgid = MeshNode::FromIndex(i).GetHGID();
        route1.push_back(hgid);
    }
    auto iter = route1.begin();
    HGID A = *iter++;
    HGID B = *iter++;
    HGID C = *iter++; 
    HGID D1 = route1.back();
    MeshRoute route2 = route1;
    route2.pop_back();
    HGID D2 = MeshNode::FromIndex(MAX_NODES-2).GetHGID();
    route2.push_back(D2);

    // add route to gateway
    HGID gateway = MeshNode::FromIndex(MAX_NODES-1).GetHGID();
    route1.push_back(gateway);
    route2.push_back(gateway);
    MeshNode::AddGateway(gateway);

    // add route from first to D1 node A->B->C->D1
    MeshNode::AddRoute(route1);

    // add route from first to D2 node A->B-C->D2
    MeshNode::AddRoute(route2);

    cout << "----------------------------------------------" << endl << endl;

    // send a message from route 0 [A to C], and receive delivery receipt
    std::string payload = "TEST TEST TEST";
    TestRoute(A, B, payload);
    TestRoute(A, C, payload);

    cout << "----------------------------------------------" << endl << endl;

    // send a message from route 1 [A to D1], and receive delivery receipt
    payload = "Mr. Watson - come here - I want to see you.";
    TestRoute(A, D1, payload);
    cout << "----------------------------------------------" << endl << endl;


    // send a message from route 2 [A to D2], and receive delivery receipt
    payload = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks.";
    TestRoute(A, D2, payload);

    cout << "----------------------------------------------" << endl << endl;

    // send a message from reverse of route 1 [D2 to A], and receive delivery receipt
    payload = "The computer can be used as a tool to liberate and protect people, rather than to control them.";
    TestRoute(D2, A, payload);
}

/*
Create a key with a seed; if seed is nullptr will generate a random key.
More thorough in MeshNode::GetMultisigPublicKey
*/
int create_key(const secp256k1_context* ctx, const std::array<uint8_t, 32> &seed, const bool nullseed, std::array<uint8_t, 32> &seckey, secp256k1_pubkey* pubkey)
{
    if (nullseed == false) {
        memcpy(seckey.data(), seed.data(), 32);
    } else {
        ifstream urandom("/dev/urandom", ios::in|ios::binary);
        if (urandom) {
            urandom.read(reinterpret_cast<char*>(seckey.data()), 32);
            urandom.close();
        } else {
            urandom.close();
            return 0;
        }
    }
    if (!secp256k1_ec_seckey_verify(ctx, seckey.data())) {
        return 0;
    } else {
        return secp256k1_ec_pubkey_create(ctx, pubkey, seckey.data());
    }
}

/* Sign a message hash with the given key pairs and store the result in sig */
int sign(const secp256k1_context* ctx, std::array<std::array<uint8_t, seckeysize>, num_signers> seckeys, const secp256k1_pubkey* pubkeys, const ustring msg, secp256k1_schnorrsig *sig) {
    secp256k1_musig_session musig_session[num_signers];
    std::array<std::array<uint8_t, noncesize>, num_signers> nonce_commitment;
    std::array<uint8_t*, num_signers> nonce_commitment_ptr;
    secp256k1_musig_session_signer_data signer_data[num_signers][num_signers];
    secp256k1_pubkey nonce[num_signers];
    int i, j;
    secp256k1_musig_partial_signature partial_sig[num_signers];
    secp256k1_pubkey musig_pk;

    for (i = 0; i < num_signers; i++) {
        std::array<uint8_t, noncesize> session_id32;
        std::array<uint8_t, hashsize> pk_hash;
        secp256k1_pubkey combined_pk;

        /* Create combined pubkey and initialize signer data */
        if (!secp256k1_musig_pubkey_combine(ctx, nullptr, &combined_pk, pk_hash.data(), pubkeys, num_signers)) {
            return 0;
        }

        /* Save combined key for veriying group signature */
        memcpy(&musig_pk, &combined_pk, 64);

        /* Create random session ID. It is absolutely necessary that the session ID
         * is unique for every call of secp256k1_musig_session_initialize. Otherwise
         * it's trivial for an attacker to extract the secret key! */
        ifstream urandom("/dev/urandom", ios::in|ios::binary);
        if (urandom) {
            urandom.read(reinterpret_cast<char*>(session_id32.data()), 32);
            urandom.close();
        } else {
            urandom.close();
            return 0;
        }

        /* Initialize session */
        if (!secp256k1_musig_session_initialize(ctx, &musig_session[i], signer_data[i], nonce_commitment[i].data(), session_id32.data(), msg.data(), &combined_pk, pk_hash.data(), num_signers, i, seckeys[i].data())) {
            return 0;
        }
        nonce_commitment_ptr[i] = nonce_commitment[i].data();
    }
        
    /* Communication round 1: Exchange nonce commitments */
    for (i = 0; i < num_signers; i++) {
        /* Set nonce commitments in the signer data and get the own public nonce */
        if (!secp256k1_musig_session_get_public_nonce(ctx, &musig_session[i], signer_data[i], &nonce[i], nonce_commitment_ptr.data(), num_signers)) {
            return 0;
        }
    }

    /* Communication round 2: Exchange nonces */
    for (i = 0; i < num_signers; i++) {
        for (j = 0; j < num_signers; j++) {
            if (!secp256k1_musig_set_nonce(ctx, &signer_data[i][j], &nonce[j])) {
                /* Signer j's nonce does not match the nonce commitment. In this case
                 * abort the protocol. If you make another attempt at finishing the
                 * protocol, create a new session (with a fresh session ID!). */
                return 0;
            }
        }
        if (!secp256k1_musig_session_combine_nonces(ctx, &musig_session[i], signer_data[i], num_signers, nullptr, nullptr)) {
            return 0;
        }
    }

    /* Each signer produces partial signatures */
    for (i = 0; i < num_signers; i++) {
        if (!secp256k1_musig_partial_sign(ctx, &musig_session[i], &partial_sig[i])) {
            return 0;
        }
    }

    /* Communication round 3: Exchange partial signatures */
    if (!secp256k1_musig_partial_sig_combine(ctx, &musig_session[0], sig, partial_sig, num_signers)) {
        return 0;
    }

    /* Produce & check group signature before the partials */
    if (!secp256k1_schnorrsig_verify(ctx, sig, msg.data(), &musig_pk)) {
        for (i = 0; i < num_signers; i++) {
            for (j = 0; j < num_signers; j++) {
                /* To check whether signing was successful, it suffices to either verify
                * the the combined signature with the combined public key using
                * secp256k1_schnorrsig_verify, or verify all partial signatures of all
                * signers individually. Verifying the combined signature is cheaper but
                * verifying the individual partial signatures has the advantage that it
                * can be used to determine which of the partial signatures are invalid
                * (if any), i.e., which of the partial signatures cause the combined
                * signature to be invalid and thus the protocol run to fail. It's also
                * fine to first verify the combined sig, and only verify the individual
                * sigs if it does not work.
                */
                if (!secp256k1_musig_partial_sig_verify(ctx, &musig_session[i], &signer_data[i][j], &partial_sig[j], &pubkeys[j])) {
                    return 0;
                }
            }
        }
    } else {
        return 1;
    }
    return 0;
}

bool testMuSig(bool useseed)
{
    // Example seed, used to generate private key. Always use
    // a secure RNG with sufficient entropy to generate a seed.

    secp256k1_32 seed = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6, 
                    220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};
    if (!useseed) {
        seed.fill(UINT8_MAX);
    }

    secp256k1_context* ctx;
    size_t serial_pubkeysize = pubkeysize;
    uint8_t i;
    std::array<std::array<uint8_t, seckeysize>, num_signers> seckeys;
    secp256k1_pubkey pubkeys[num_signers];
    std::array<std::array<uint8_t, pubkeysize>, num_signers> serialized_pubkeys;
    secp256k1_pubkey combined_pk;
    const ustring msg(reinterpret_cast<const unsigned char*>("this_could_be_the_hash_of_a_msg"));
    secp256k1_schnorrsig sig;

    bool nullseed = true;
    nullseed = !(std::any_of(seed.begin(), seed.end(), [](const uint8_t byte) {return byte != UINT8_MAX;}));

    /* Create a context for signing and verification */
    ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    cout << "Creating key pairs......" << endl;
    for (i = 0; i < num_signers; i++) {
        if (!create_key(ctx, seed, nullseed, seckeys[i], &pubkeys[i])) {
            cout << "keygen failed" << endl;
            return false;
        } else {
            cout << "sk" << i << ": " << std::hex;
            for ( uint8_t byte : seckeys[i] ) { cout << std::setw(2) << std::setfill('0') << static_cast<int>(byte); }
            cout << endl;
            secp256k1_ec_pubkey_serialize(ctx, serialized_pubkeys[i].data(), &serial_pubkeysize, &pubkeys[i], SECP256K1_EC_COMPRESSED);
            if (serial_pubkeysize != pubkeysize) {
                cout << "pubkey serialization failed" << endl;
                return false;
            }
            cout << "pk" << i << ": " << std::hex;
            for ( uint8_t byte : serialized_pubkeys[i] ) { cout << std::setw(2) << std::setfill('0') << static_cast<int>(byte); }
            cout << endl;
            seed[0]++;
        }
    }
    cout << "Combining pubkeys......" << endl;
    if (!secp256k1_musig_pubkey_combine(ctx, nullptr, &combined_pk, nullptr, pubkeys, num_signers)) {
        cout << "key combining failed" << endl;
        return false;
    }

    cout << "Signing and verifying......" << endl;
    if (!sign(ctx, seckeys, pubkeys, msg, &sig)) {
        cout << "signing or verifying failed" << endl;
        return false;
    }

    secp256k1_context_destroy(ctx);
    return true;
}