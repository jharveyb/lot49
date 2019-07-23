#include "bls.hpp"
#include "MeshNode.hpp"
#include "Ledger.hpp"
#include "ImpliedTransaction.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <cassert>
extern "C" {
#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_musig.h>
}
#include <array>
#include <string>

using namespace std;
using namespace lot49;

// TODO: use  48 byte signatures, not 96 byte: https://twitter.com/sorgente711/status/1025451092610433024

bool testBLS();
bool testMuSig(bool);
// Needed to use std::array over C-style arrays
const size_t num_signers = 3;
const size_t seckeysize = 32; 
const size_t noncesize = 32; 
const size_t hashsize = 32; 
const size_t pubkeysize = 33; // always serialize keys in compressed form
typedef std::basic_string<unsigned char> ustring;
void test1();
void test2();
void test3();

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
    cout << "testBLS():  " << (testBLS() ? "success!" : "failed!") << endl;

    cout << "testMuSig():  " << (testMuSig(true) ? "success!" : "failed!") << endl;

    // test with pre-defined paths
    test1();

    // test randomly moving nodes
    //test2();

    // test aggregation of specific keys and messages
    //test3();
};

// test aggregation of specific keys and messages
void test3()
{  
    int s1 = 7;
    int s2 = 1;


    std::vector<uint8_t> seed1(32, s1);
    std::vector<uint8_t> seed2(32, s2);

    bls::PrivateKey sk1 = bls::PrivateKey::FromSeed(seed1.data(), seed1.size());
    cout << "sk" << s1 << " = " << std::hex;
    for (auto byte : seed1 ) { cout << std::setw(2) << std::setfill('0') << static_cast<int>(byte);} 
    cout << endl;
    bls::PublicKey pk1 = sk1.GetPublicKey();
    cout << "pk" << s1 << " = " << std::hex << pk1 << endl;

    bls::PrivateKey sk2 = bls::PrivateKey::FromSeed(seed2.data(), seed2.size());
    cout << "sk" << s2 << " = " << std::hex;
    for (auto byte : seed2 ) { cout << std::setw(2) << std::setfill('0') << static_cast<int>(byte);} 
    cout << endl;
    bls::PublicKey pk2 = sk2.GetPublicKey();
    cout << "pk" << s2 << " = " << std::hex << pk2 << endl;

    std::vector<uint8_t> refund_tx = lot49::HexToBytes("0304c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc704c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000d000000007000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    std::vector<uint8_t> setup_tx = lot49::HexToBytes("0204c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc7d000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    std::vector<uint8_t> update1_tx = lot49::HexToBytes("0404c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc704c9dac5dd96896276e531c2e86c8fa704c4a2996c23b146fcb48d08d6d3ae48728b29d9fdead8b3bab150cb7d58fbcd857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc7cd00030007018f62165173f9087a003c04de96bd3ef09cd84b7b7c53c4f77aa8870594dea08d41ab8fb1db7e48bdc4bae1dd6b1412700000000000000000000000000000000000000000000000000000000000000000");
    std::vector<uint8_t> update2_tx = lot49::HexToBytes("04857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc78f62165173f9087a003c04de96bd3ef09cd84b7b7c53c4f77aa8870594dea08d41ab8fb1db7e48bdc4bae1dd6b141270857958fe97f040075ef324bfd46a0d2031ea3c28fe8bda1ea2542cf4ffff3b2ca23ea8d216cf33881789fcd0ff9f7bc78f62165173f9087a003c04de96bd3ef09cd84b7b7c53c4f77aa8870594dea08d41ab8fb1db7e48bdc4bae1dd6b141270ce00020007028f62165173f9087a003c04de96bd3ef09cd84b7b7c53c4f77aa8870594dea08d41ab8fb1db7e48bdc4bae1dd6b1412700000000000000000000000000000000000000000000000000000000000000000");

    // SUCCEED
    std::vector<uint8_t> sig_refund_test = lot49::HexToBytes("038da3bdcdf4838137f88268e3bb532f873671d53ac19807e0806888e082df1e9f2d6ab6d632c94993a7db257c21a98509ceef38d5f195037a9fa2444979c6c03654d9f4a309dc159aa2a7c73f7f2be1f984c29755e45f2b288d32f08542b25b");

    // SUCCEED
    std::vector<uint8_t> sig_update1_test = lot49::HexToBytes("15ad5f8a75b0c3c18a6efbb298a6db788bc6049a2eab7d01454d03f50383f648935f045e3749da5296052ac43ff430aa05d66916f70a23eff3a5de65663e34f4e94cc42ae81909868b5d240ebbb01ac78ec2f2a068a586ab88a3139bd1d2f78b");

    // FAIL?
    std::vector<uint8_t> sig_update2_test = lot49::HexToBytes("99b850d5d360b0a6c61fa7ea909750546bf53b3acc78549d8804e709c9dd77ecdf23320c7f65de52d5e9303d0fd06a7602d90a3d19d45bfb6b59d9824db30e183bf2a797d2a1bee21c9fc80d53ca46d562a6ed281f33113f6fd8466ab5bf5b32");

    std::vector<uint8_t> refund_hash(bls::BLS::MESSAGE_HASH_LEN);
    bls::Util::Hash256(refund_hash.data(), reinterpret_cast<const uint8_t*>(refund_tx.data()), refund_tx.size());
    std::vector<uint8_t> setup_hash(bls::BLS::MESSAGE_HASH_LEN);
    bls::Util::Hash256(setup_hash.data(), reinterpret_cast<const uint8_t*>(setup_tx.data()), setup_tx.size());
    std::vector<uint8_t> update1_hash(bls::BLS::MESSAGE_HASH_LEN);
    bls::Util::Hash256(update1_hash.data(), reinterpret_cast<const uint8_t*>(update1_tx.data()), update1_tx.size());
    std::vector<uint8_t> update2_hash(bls::BLS::MESSAGE_HASH_LEN);
    bls::Util::Hash256(update2_hash.data(), reinterpret_cast<const uint8_t*>(update2_tx.data()), update2_tx.size());

    // Create signatures
    bls::Signature sig_refund_sk1 = sk1.Sign(refund_tx.data(), refund_tx.size());
    bls::Signature sig_setup_sk2 = sk2.Sign(setup_tx.data(), setup_tx.size());
    bls::Signature sig_update1_sk2 = sk2.Sign(update1_tx.data(), update1_tx.size());
    bls::Signature sig_update1_sk1 = sk1.Sign(update1_tx.data(), update1_tx.size());
    bls::Signature sig_update2_sk1 = sk1.Sign(update2_tx.data(), update2_tx.size());

    cout << "sig_refund = " << std::hex << sig_refund_sk1 << endl << endl;
    assert(sig_refund_sk1 == bls::Signature::FromBytes(sig_refund_test.data()));

    // Create aggregate signature for update1 test
    vector<bls::Signature> update1_sigs = {sig_refund_sk1, sig_setup_sk2, sig_update1_sk2};
    {
        cout << "update 1 test" << endl;
        bls::Signature agg_sig = bls::Signature::AggregateSigs(update1_sigs);
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        bool ok = agg_sig.Verify();
        cout << "agg_sig.Verify() = " << (ok ? "true" : "false") << endl;
        assert(agg_sig == bls::Signature::FromBytes(sig_update1_test.data()));
        update1_sigs.clear();
        update1_sigs.push_back(agg_sig);
        cout << endl;
    }

    // Create aggregate signature for update1 test and added update2 signatures
    {
        cout << "update 1 + update 2 test" << endl;
        bls::AggregationInfo a1 = bls::AggregationInfo::FromMsgHash(pk1, refund_hash.data());
        bls::AggregationInfo a2 = bls::AggregationInfo::FromMsgHash(pk2, setup_hash.data());
        bls::AggregationInfo a3 = bls::AggregationInfo::FromMsgHash(pk2, update1_hash.data());
        vector<bls::AggregationInfo> infos = {a1, a2, a3};
        bls::AggregationInfo agg_info = bls::AggregationInfo::MergeInfos(infos);
        bls::Signature agg_sig = bls::Signature::FromBytes(update1_sigs.back().Serialize().data());
        agg_sig.SetAggregationInfo(agg_info);
        
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        bool ok = agg_sig.Verify();
        cout << "agg_sig.Verify() = " << (ok ? "true" : "false") << endl;
        assert(agg_sig == bls::Signature::FromBytes(sig_update1_test.data()));
        cout << endl;

        std::vector<bls::Signature> sigs = {agg_sig, sig_update1_sk1, sig_update2_sk1};
        agg_sig = bls::Signature::AggregateSigs(sigs);
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        ok = agg_sig.Verify();
        cout << "agg_sig.Verify() = " << (ok ? "true" : "false") << endl;
        //assert(agg_sig == bls::Signature::FromBytes(sig_update2_test.data()));
        cout << endl;
    }

    // Create aggreage signature for update2 test
    vector<bls::Signature> update2_sigs;
    {
        cout << "update 2 test" << endl;
        std::vector<bls::Signature> sigs = {sig_refund_sk1, sig_setup_sk2, sig_update1_sk2};
        bls::Signature agg_sig = bls::Signature::AggregateSigs(sigs);
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        cout << "agg_sig.Verify() = " << (agg_sig.Verify() ? "true" : "false") << endl;

        sigs = {agg_sig, sig_update1_sk1, sig_update2_sk1};
        agg_sig = bls::Signature::AggregateSigs(sigs);
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        cout << "agg_sig.Verify() = " << (agg_sig.Verify() ? "true" : "false") << endl;
        assert(agg_sig == bls::Signature::FromBytes(sig_update2_test.data()));
        update2_sigs.push_back(agg_sig);
        cout << endl;
    }

    // use aggregation information with deserialize sig for update 2
    {
        bls::AggregationInfo a1 = bls::AggregationInfo::FromMsgHash(pk1, refund_hash.data());
        bls::AggregationInfo a2 = bls::AggregationInfo::FromMsgHash(pk2, setup_hash.data());
        bls::AggregationInfo a3 = bls::AggregationInfo::FromMsgHash(pk2, update1_hash.data());
        bls::AggregationInfo a4 = bls::AggregationInfo::FromMsgHash(pk1, update1_hash.data());
        bls::AggregationInfo a5 = bls::AggregationInfo::FromMsgHash(pk1, update2_hash.data());
        vector<bls::AggregationInfo> infos = {a1};
        bls::AggregationInfo agg_info = bls::AggregationInfo::MergeInfos(infos);
        infos = {agg_info, a2, a3};
        agg_info = bls::AggregationInfo::MergeInfos(infos);
        infos = {agg_info, a4, a5};
        agg_info = bls::AggregationInfo::MergeInfos(infos);

        // reconstruct signature from serialized data
        cout << "update 2 test from serialized aggregate signature" << endl;
        vector<bls::Signature> sigs = {sig_refund_sk1, sig_setup_sk2, sig_update1_sk2};
        bls::Signature agg_sig = bls::Signature::AggregateSigs(sigs);
        sigs = {agg_sig, sig_update1_sk1, sig_update2_sk1};
        agg_sig = bls::Signature::AggregateSigs(sigs);
        
        // strip aggregation data
        agg_sig = bls::Signature::FromBytes(agg_sig.Serialize().data());

        // add reconstructed aggregation data
        agg_sig.SetAggregationInfo(agg_info);
        cout << "agg_sig = " << std::hex << agg_sig << endl;
        cout << "agg_sig.Verify() = " << (agg_sig.Verify() ? "true" : "false") << endl;
        assert(agg_sig == bls::Signature::FromBytes(sig_update2_test.data()));
    }
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

        for (int i = 0; i < MAX_NODES; i++) {
            bls::PublicKey pk = MeshNode::FromIndex(i).GetPublicKey();
            Ledger::sInstance.Issue(pk, COMMITTED_TOKENS, MAX_NODES);
        }

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

bool testBLS()
{
    //
    // Creating keys and signatures
    //

    // Example seed, used to generate private key. Always use
    // a secure RNG with sufficient entropy to generate a seed.
    uint8_t seed[] = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192,
                      19, 18, 12, 89, 6, 220, 18, 102, 58, 209,
                      82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};

    bls::PrivateKey sk = bls::PrivateKey::FromSeed(seed, sizeof(seed));
    bls::PublicKey pk = sk.GetPublicKey();

    //
    // Serializing keys and signatures to bytes
    //

    uint8_t skBytes[bls::PrivateKey::PRIVATE_KEY_SIZE]; // 32 byte array
    uint8_t pkBytes[bls::PublicKey::PUBLIC_KEY_SIZE];   // 96 byte array
    uint8_t sigBytes[bls::Signature::SIGNATURE_SIZE];   // 48 byte array

    pk.Serialize(pkBytes); // 96 bytes

    // Takes array of 96 bytes
    pk = bls::PublicKey::FromBytes(pkBytes);

    sk.Serialize(skBytes); // 32 bytes

    uint8_t msg[] = {100, 2, 254, 88, 90, 45, 23};
    bls::Signature sig = sk.Sign(msg, sizeof(msg));

    sig.Serialize(sigBytes); // 48 bytes

    //
    // Loading keys and signatures from bytes
    //

    // Takes array of 32 bytes
    sk = bls::PrivateKey::FromBytes(skBytes);

    // Takes array of 96 bytes
    pk = bls::PublicKey::FromBytes(pkBytes);

    // Takes array of 48 bytes
    sig = bls::Signature::FromBytes(sigBytes);

    //
    // Verifying signatures
    //

    // Add information required for verification, to sig object
    sig.SetAggregationInfo(bls::AggregationInfo::FromMsg(pk, msg, sizeof(msg)));

    bool ok = sig.Verify();

    //
    // Aggregate signatures for a single message
    //

    {
        // Generate some more private keys
        seed[0] = 1;
        bls::PrivateKey sk1 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
        seed[0] = 2;
        bls::PrivateKey sk2 = bls::PrivateKey::FromSeed(seed, sizeof(seed));

        // Generate first sig
        bls::PublicKey pk1 = sk1.GetPublicKey();
        bls::Signature sig1 = sk1.Sign(msg, sizeof(msg));

        // Generate second sig
        bls::PublicKey pk2 = sk2.GetPublicKey();
        bls::Signature sig2 = sk2.Sign(msg, sizeof(msg));

        // Aggregate signatures together
        vector<bls::Signature> sigs = {sig1, sig2};
        bls::Signature aggSig = bls::Signature::AggregateSigs(sigs);

        // For same message, public keys can be aggregated into one.
        // The signature can be verified the same as a single signature,
        // using this public key.
        vector<bls::PublicKey> pubKeys = {pk1, pk2};

        bls::PublicKey aggPubKey = bls::PublicKey::Aggregate(pubKeys);
    }

    //
    // Aggregate signatures for different messages
    //

    // Generate one more key and message
    seed[0] = 1;
    bls::PrivateKey sk1 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
    bls::PublicKey pk1 = sk1.GetPublicKey();

    seed[0] = 2;
    bls::PrivateKey sk2 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
    bls::PublicKey pk2 = sk2.GetPublicKey();

    seed[0] = 3;
    bls::PrivateKey sk3 = bls::PrivateKey::FromSeed(seed, sizeof(seed));
    bls::PublicKey pk3 = sk3.GetPublicKey();
    uint8_t msg2[] = {100, 2, 254, 88, 90, 45, 23};

    // Generate the signatures, assuming we have 3 private keys
    bls::Signature sig1 = sk1.Sign(msg, sizeof(msg));
    bls::Signature sig2 = sk2.Sign(msg, sizeof(msg));
    bls::Signature sig3 = sk3.Sign(msg2, sizeof(msg2));

    // They can be noninteractively combined by anyone
    // Aggregation below can also be done by the verifier, to
    // make batch verification more efficient
    vector<bls::Signature> sigsL = {sig1, sig2};
    bls::Signature aggSigL = bls::Signature::AggregateSigs(sigsL);

    // Arbitrary trees of aggregates
    vector<bls::Signature> sigsFinal = {aggSigL, sig3};
    bls::Signature aggSigFinal = bls::Signature::AggregateSigs(sigsFinal);

    // Serialize the final signature
    aggSigFinal.Serialize(sigBytes);

    //
    // Verify aggregate signature for different messages
    //

    // Deserialize aggregate signature
    aggSigFinal = bls::Signature::FromBytes(sigBytes);

    // Create aggregation information (or deserialize it)
    bls::AggregationInfo a1 = bls::AggregationInfo::FromMsg(pk1, msg, sizeof(msg));
    bls::AggregationInfo a2 = bls::AggregationInfo::FromMsg(pk2, msg, sizeof(msg));
    bls::AggregationInfo a3 = bls::AggregationInfo::FromMsg(pk3, msg2, sizeof(msg2));
    vector<bls::AggregationInfo> infos = {a1, a2};
    bls::AggregationInfo a1a2 = bls::AggregationInfo::MergeInfos(infos);
    vector<bls::AggregationInfo> infos2 = {a1a2, a3};
    bls::AggregationInfo aFinal = bls::AggregationInfo::MergeInfos(infos2);

    // Verify final signature using the aggregation info
    aggSigFinal.SetAggregationInfo(aFinal);
    ok = aggSigFinal.Verify();

    // If you previously verified a signature, you can also divide
    // the aggregate signature by the signature you already verified.
    ok = aggSigL.Verify();
    vector<bls::Signature> cache = {aggSigL};
    aggSigFinal = aggSigFinal.DivideBy(cache);

    // Final verification is now more efficient
    ok = aggSigFinal.Verify();

    return ok;
}

/* Create a key with a seed; if seed is nullptr will generate a random key. */
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
int sign(const secp256k1_context* ctx, std::array<std::array<uint8_t, seckeysize>, num_signers> seckeys, const secp256k1_pubkey* pubkeys, const std::basic_string<unsigned char> msg, secp256k1_schnorrsig *sig) {
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

    std::array<uint8_t, 32> seed;
    if (useseed) {
        seed = {0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6, 
            220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22};
    } else {
        seed.fill(255);
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
    for (uint8_t value : seed) {
        if (value != 255) {
            nullseed = false;
            break;
        }
    }

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