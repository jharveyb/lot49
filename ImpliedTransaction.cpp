#include "bls.hpp"
#include "MeshNode.hpp"
#include "ImpliedTransaction.hpp"
#include <random>
#include <cassert>
#include <algorithm>

static std::default_random_engine rng(std::random_device{}());
static std::uniform_int_distribution<uint8_t> dist(0, 255); //(min, max)

namespace lot49
{

//
// create implied transactions
//

void ReadCSPRNG(const std::string &source, char* outbuf, uint8_t readsize)
{
    std::ifstream urandom;
    urandom.open(source, std::ios::in | std::ios::binary);
    if (urandom.is_open()) {
        urandom.read(outbuf, readsize);
        urandom.close();
    } else {
        urandom.close();
        throw std::invalid_argument("Error with /dev/urandom");
    }
}
/*
void SetPublicKey(std::vector<uint8_t>& outPubkey, const bls::PublicKey& inSource)
{
    inSource.Serialize(&outPubkey[0]);
}
*/
void SetMultisigPublicKey(secp256k1_33& outPubkey, const secp256k1_33& inSource)
{
    std::copy(inSource.begin(), inSource.end(), outPubkey.begin());
}
/*
ImpliedTransaction ImpliedTransaction::Issue(const bls::PublicKey& inReceiver, const uint16_t inFundingAmount)
{
    // cout << "Make Issue Tx" << endl;
    // issue 1:1 stored value UTXO from no previous UTXO, equivalent to mining reward (ie. no input tx)
    ImpliedTransaction tx;
    tx.mType = eIssue;

    std::vector<uint8_t> seed(bls::PrivateKey::PRIVATE_KEY_SIZE);
    std::generate_n(seed.begin(), bls::PrivateKey::PRIVATE_KEY_SIZE, [&] { return dist(rng); });
    bls::PrivateKey sk = bls::PrivateKey::FromSeed(seed.data(), seed.size());

    SetPublicKey(tx.mInputOwner1, sk.GetPublicKey());
    SetPublicKey(tx.mOutputOwner1, inReceiver);
    tx.mOutputAmount1 = inFundingAmount;
    SetPublicKey(tx.mTransactionSigner, inReceiver);
    return tx;    
}
*/
ImpliedTransaction ImpliedTransaction::MultisigIssue(const secp256k1_33& inReceiver, const uint16_t inFundingAmount)
{
    // cout << "Make Issue Tx" << endl;
    // issue 1:1 stored value UTXO from no previous UTXO, equivalent to mining reward (ie. no input tx)
    ImpliedTransaction tx;
    tx.mType = eIssue;

    // Issuing coins with a random keypair (outside of MeshNode)
    secp256k1_context* context_temporary = secp256k1_context_create(SECP256K1_CONTEXT_SIGN || SECP256K1_CONTEXT_VERIFY);
    secp256k1_32 seckey;
    secp256k1_pubkey pubkey;
    secp256k1_33 serial_pk;
    size_t serialsize = pubkeysize;
    ReadCSPRNG("/dev/urandom", reinterpret_cast<char*>(seckey.data()), seckeysize); 
    if (!secp256k1_ec_seckey_verify(context_temporary, seckey.data())) {
        throw std::invalid_argument("Invalid multisig private key");
    }
    if (!secp256k1_ec_pubkey_create(context_temporary, &pubkey, seckey.data())) {
        throw std::invalid_argument("Error deriving pubkey");
    }
    secp256k1_ec_pubkey_serialize(context_temporary, serial_pk.data(), &serialsize, &pubkey, SECP256K1_EC_COMPRESSED);
    if (serialsize != pubkeysize) {
        throw std::invalid_argument("Error serializing pubkey");
    }

    SetMultisigPublicKey(tx.mInputOwner1, serial_pk);
    SetMultisigPublicKey(tx.mOutputOwner1, inReceiver);
    tx.mOutputAmount1 = inFundingAmount;
    SetMultisigPublicKey(tx.mTransactionSigner, inReceiver);
    secp256k1_context_destroy(context_temporary);
    return tx;    
}
/*
ImpliedTransaction ImpliedTransaction::Transfer(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const uint16_t inFundingAmount)
{
    //cout << "Make Transfer Tx" << endl;
    // transfer value to 1:1 UTXO from previous 1:1 UTXO
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eTransfer;
    SetPublicKey(tx.mInputOwner1, inSender);
    ClearVector(tx.mInputOwner2);
    SetPublicKey(tx.mOutputOwner1, inReceiver);
    ClearVector(tx.mOutputOwner2);
    tx.mOutputAmount1 = inFundingAmount;
    tx.mOutputAmount2 = 0;
    tx.mTimeDelay = 0;
    tx.mChannelState = 0;
    ClearVector(tx.mMessageSigner);
    ClearVector(tx.mMessageHash);
    SetPublicKey(tx.mTransactionSigner, inSender);
    return tx;
}
*/
ImpliedTransaction ImpliedTransaction::MultisigTransfer(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver, const uint16_t inFundingAmount)
{
    //cout << "Make Transfer Tx" << endl;
    // transfer value to 1:1 UTXO from previous 1:1 UTXO
    // constructor intializes with 0-filled vectors of proper size -> don't need to clear again
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eTransfer;
    SetMultisigPublicKey(tx.mInputOwner1, inSender);
    SetMultisigPublicKey(tx.mOutputOwner1, inReceiver);
    tx.mOutputAmount1 = inFundingAmount;
    // implicit but makes this more readable
    tx.mOutputAmount2 = 0;
    tx.mTimeDelay = 0;
    tx.mChannelState = 0;
    SetMultisigPublicKey(tx.mTransactionSigner, inSender);
    return tx;
}
/*
ImpliedTransaction ImpliedTransaction::Setup(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const uint16_t inFundingAmount)
{
    //cout << "Make Setup Tx" << endl;
// fund 2:2 UTXO from previous 1:1 UTXO
    ImpliedTransaction tx(inInput);
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eSetup;
    SetPublicKey(tx.mInputOwner1, inSender);
    ClearVector(tx.mInputOwner2);
    SetPublicKey(tx.mOutputOwner1, inSender);
    SetPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inFundingAmount;
    tx.mOutputAmount2 = 0;
    tx.mTimeDelay = 0;
    tx.mChannelState = 0;
    ClearVector(tx.mMessageSigner);
    ClearVector(tx.mMessageHash);
    SetPublicKey(tx.mTransactionSigner, inSender);
    return tx;
}
*/

ImpliedTransaction ImpliedTransaction::MultisigSetup(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver, const uint16_t inFundingAmount)
{
    //cout << "Make Setup Tx" << endl;
    // fund 2:2 UTXO from previous 1:1 UTXO
    ImpliedTransaction tx(inInput);
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eSetup;
    SetMultisigPublicKey(tx.mInputOwner1, inSender);
    SetMultisigPublicKey(tx.mOutputOwner1, inSender);
    SetMultisigPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inFundingAmount;
    SetMultisigPublicKey(tx.mTransactionSigner, inSender);
    return tx;
}
/*
ImpliedTransaction ImpliedTransaction::Refund(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const bls::PublicKey& inSigner, const uint16_t inRefundAmount)
{
    //cout << "Make Refund Tx" << endl;
    // refund to 1:1 UTXO from previous 2:2 UTXO after delay
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eRefund;
    SetPublicKey(tx.mInputOwner1, inSender);
    SetPublicKey(tx.mInputOwner2, inReceiver);
    SetPublicKey(tx.mOutputOwner1, inSender);
    ClearVector(tx.mOutputOwner2);
    tx.mOutputAmount1 = inRefundAmount;
    tx.mOutputAmount2 = 0;
    tx.mTimeDelay = 7;
    tx.mChannelState = 0;
    ClearVector(tx.mMessageSigner);
    ClearVector(tx.mMessageHash);
    SetPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
*/
ImpliedTransaction ImpliedTransaction::MultisigRefund(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver, const secp256k1_33& inSigner, const uint16_t inRefundAmount)
{
    //cout << "Make Refund Tx" << endl;
    // refund to 1:1 UTXO from previous 2:2 UTXO after delay
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eRefund;
    SetMultisigPublicKey(tx.mInputOwner1, inSender);
    SetMultisigPublicKey(tx.mInputOwner2, inReceiver);
    SetMultisigPublicKey(tx.mOutputOwner1, inSender);
    tx.mOutputAmount1 = inRefundAmount;
    tx.mTimeDelay = 7;
    SetMultisigPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
/*
ImpliedTransaction ImpliedTransaction::UpdateAndSettle(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const bls::PublicKey& inSigner, 
    const uint16_t inSenderAmount, const uint16_t inReceiverAmount, const bls::PublicKey& inDestination, const std::vector<uint8_t>& inMessageHash)
{
    //cout << "Make UpdateAndSettle Tx" << endl;
    // update to new 2:2 UTXO or settle to two 1:1 UTXOs after delay from previous 2:2 UTXO
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eUpdateAndSettle;
    SetPublicKey(tx.mInputOwner1, inSender);
    SetPublicKey(tx.mInputOwner2, inReceiver);
    SetPublicKey(tx.mOutputOwner1, inSender);
    SetPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inSenderAmount;
    tx.mOutputAmount2 = inReceiverAmount;
    tx.mTimeDelay = 7;
    tx.mChannelState = inInput.mChannelState + 1;
    SetPublicKey(tx.mMessageSigner, inDestination);
    ClearVector(tx.mMessageHash);
    SetPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
*/
ImpliedTransaction ImpliedTransaction::MultisigUpdateAndSettle(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver, const secp256k1_33& inSigner, 
    const uint16_t inSenderAmount, const uint16_t inReceiverAmount, const secp256k1_33& inDestination, const std::vector<uint8_t>& inMessageHash)
{
    //cout << "Make UpdateAndSettle Tx" << endl;
    // update to new 2:2 UTXO or settle to two 1:1 UTXOs after delay from previous 2:2 UTXO
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eUpdateAndSettle;
    SetMultisigPublicKey(tx.mInputOwner1, inSender);
    SetMultisigPublicKey(tx.mInputOwner2, inReceiver);
    SetMultisigPublicKey(tx.mOutputOwner1, inSender);
    SetMultisigPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inSenderAmount;
    tx.mOutputAmount2 = inReceiverAmount;
    tx.mTimeDelay = 7;
    tx.mChannelState = inInput.mChannelState + 1;
    SetMultisigPublicKey(tx.mMessageSigner, inDestination);
    SetMultisigPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
/*
ImpliedTransaction ImpliedTransaction::Close(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const bls::PublicKey& inSigner, 
    const uint16_t inSenderAmount, const uint16_t inReceiverAmount)
{
    //cout << "Make Close Tx" << endl;
    // refund Refund 2:2 UTXO
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eClose;
    SetPublicKey(tx.mInputOwner1, inSender);
    SetPublicKey(tx.mInputOwner2, inReceiver);
    SetPublicKey(tx.mOutputOwner1, inSender);
    SetPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inSenderAmount;
    tx.mOutputAmount2 = inReceiverAmount;
    tx.mTimeDelay = 0;
    tx.mChannelState = inInput.mChannelState + 1;
    ClearVector(tx.mMessageSigner);
    ClearVector(tx.mMessageHash);
    SetPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
*/
ImpliedTransaction ImpliedTransaction::MultisigClose(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver, const secp256k1_33& inSigner, 
    const uint16_t inSenderAmount, const uint16_t inReceiverAmount)
{
    //cout << "Make Close Tx" << endl;
    // refund Refund 2:2 UTXO
    ImpliedTransaction tx;
    tx.mInputTxHash = inInput.GetHash();
    tx.mType = eClose;
    SetMultisigPublicKey(tx.mInputOwner1, inSender);
    SetMultisigPublicKey(tx.mInputOwner2, inReceiver);
    SetMultisigPublicKey(tx.mOutputOwner1, inSender);
    SetMultisigPublicKey(tx.mOutputOwner2, inReceiver);
    tx.mOutputAmount1 = inSenderAmount;
    tx.mOutputAmount2 = inReceiverAmount;
    tx.mChannelState = inInput.mChannelState + 1;
    SetMultisigPublicKey(tx.mTransactionSigner, inSigner);
    return tx;
}
// default ctor
ImpliedTransaction::ImpliedTransaction()
{
    mInputTxHash.resize(hashsize, 0);
    mType = eSetup;
    /*
    mInputOwner1.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    mInputOwner2.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    mOutputOwner1.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    mOutputOwner2.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    */
    mInputOwner1.fill(0);
    mInputOwner2.fill(0);
    mOutputOwner1.fill(0);
    mOutputOwner1.fill(0);
    mOutputAmount1 = 0;
    mOutputAmount2 = 0;
    mTimeDelay = 0;
    mChannelState = 0;
    /*
    mMessageSigner.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    mMessageHash.resize(hashsize, 0);
    mTransactionSigner.resize(bls::PublicKey::PUBLIC_KEY_SIZE, 0);
    */
    mMessageSigner.fill(0);
    mMessageHash.fill(0);
    // not part of serialization or transaction hash
    mTransactionSigner.fill(0);
    mTransactionSecondSigner.fill(0);
}

bool ImpliedTransaction::operator==(const ImpliedTransaction& rval) const
{
    return GetId() == rval.GetId();
}

bool ImpliedTransaction::operator<(const ImpliedTransaction& rval) const
{
    return GetId() < rval.GetId();
}

// get short transaction ID
uint32_t ImpliedTransaction::GetId() const
{
    std::vector<uint8_t> txhash = GetHash();
    uint32_t txid = FourBytesToInt(txhash.data());
    return txid;    
}

// compute hash of this transaction
std::vector<uint8_t> ImpliedTransaction::GetHash() const
{
    const std::vector<uint8_t> msg = Serialize();
    std::vector<uint8_t> message_hash(hashsize);
    GetSHA256(message_hash.data(), msg.data(), msg.size());
    return message_hash;
}

// get short transaction ID of input transaction
uint32_t ImpliedTransaction::GetInputId() const
{
    std::vector<uint8_t> txhash = GetInputHash();
    uint32_t txid = FourBytesToInt(txhash.data());
    return txid;
}

// get hash of input transaction
std::vector<uint8_t> ImpliedTransaction::GetInputHash() const
{
    return mInputTxHash;
}

// compute serialization of the transaction
std::vector<uint8_t> ImpliedTransaction::Serialize() const
{
    // mType is a byte + 4 pubkeys + 2 amounts (2 bytes) + 2 byte-sized fields + pubkey + hash
    //std::vector<uint8_t> msg(bls::PublicKey::PUBLIC_KEY_SIZE*5 + hashsize + 7);
    std::vector<uint8_t> msg(pubkeysize*5 + hashsize + 7);
    auto msg_ptr = msg.begin();
    //msg_ptr = std::copy(mInputTxHash.begin(), mInputTxHash.end(), msg_ptr);
    *msg_ptr++ = static_cast<uint8_t>(mType);
    msg_ptr = std::copy(mInputOwner1.begin(), mInputOwner1.end(), msg_ptr);
    msg_ptr = std::copy(mInputOwner2.begin(), mInputOwner2.end(), msg_ptr);
    msg_ptr = std::copy(mOutputOwner1.begin(), mOutputOwner1.end(), msg_ptr);
    msg_ptr = std::copy(mOutputOwner2.begin(), mOutputOwner2.end(), msg_ptr);
    *msg_ptr = mOutputAmount1; msg_ptr+= sizeof(mOutputAmount1);
    *msg_ptr = mOutputAmount2; msg_ptr+= sizeof(mOutputAmount2);
    *msg_ptr++ = mTimeDelay;
    *msg_ptr++ = mChannelState;
    msg_ptr = std::copy(mMessageSigner.begin(), mMessageSigner.end(), msg_ptr);
    msg_ptr = std::copy(mMessageHash.begin(), mMessageHash.end(), msg_ptr);
    // left out of serialization and transaction hash
    //std::copy(mTransactionSigner.begin(), mTransactionSigner.end(), msg_ptr);
    return msg;
}

// public key of the transaction signer
/*
const bls::PublicKey ImpliedTransaction::GetSigner() const
{
    return bls::PublicKey::FromBytes(mTransactionSigner.data());
}
*/
const secp256k1_33 ImpliedTransaction::GetMultisigSigner() const
{
    return mTransactionSigner;
}

// aggregate public key of signer with public key of other signer
/*
bool ImpliedTransaction::AddSigner(const bls::PublicKey& inSigner) 
{
    // record aggregate public key for two transaction signers; aggregate public keys in order (eg. first, second)
    const bls::PublicKey signer = GetSigner();
    bls::PublicKey input_owner0 = GetInputOwner(0);
    bls::PublicKey input_owner1 = GetInputOwner(1);
    bls::PublicKey current_signer = GetSigner();
    bool isValidSigner = current_signer == input_owner0 && inSigner == input_owner1;
    isValidSigner |= inSigner == input_owner0 && current_signer == input_owner1;
    
    // replace single public key of transaction signer with aggregate public key of both required transaction signers
    if (isValidSigner) {
        std::vector<bls::PublicKey> signers = {input_owner0, input_owner1};
        bls::PublicKey::Aggregate(signers).Serialize(mTransactionSigner.data());
    }

    assert(isValidSigner);
    return isValidSigner;
}
*/
bool ImpliedTransaction::AddMultisigSigner(const secp256k1_33& inSigner) 
{
    // record aggregate public key for two transaction signers; aggregate public keys in order (eg. first, second)
    const secp256k1_33 signer = GetMultisigSigner();
    secp256k1_33 input_owner0 = GetMultisigInputOwner(0);
    secp256k1_33 input_owner1 = GetMultisigInputOwner(1);
    secp256k1_33 current_signer = GetMultisigSigner();
    bool isValidSigner = (current_signer == input_owner0 && inSigner == input_owner1);
    isValidSigner |= (inSigner == input_owner0 && current_signer == input_owner1);
    
    // add second signer to the transaction
    if (isValidSigner) {
        std::copy(inSigner.begin(), inSigner.end(), mTransactionSecondSigner.begin());
    }

    assert(isValidSigner);
    return isValidSigner;
}
// return true if transaction output must be signed by public keys of two owners
bool ImpliedTransaction::IsMultisig() const
{
    if (GetType() == eIssue || GetType() == eTransfer || GetType() == eRefund) {
        return false;
    }
    return true;  
}

// get public key of output owner 0 or 1
/*
bls::PublicKey ImpliedTransaction::GetOutputOwner(const int index) const
{
    assert(index == 0 || index == 1);
    if (index == 0) {
        return bls::PublicKey::FromBytes(mOutputOwner1.data());
    }
    return bls::PublicKey::FromBytes(mOutputOwner2.data());
}
*/
secp256k1_33 ImpliedTransaction::GetMultisigOutputOwner(const int index) const
{
    assert(index == 0 || index == 1);
    if (index == 0) {
        return mOutputOwner1;
    }
    return mOutputOwner2;
}

// get total output amount for a given signing owner
uint16_t ImpliedTransaction::GetOutputAmount() const 
{
    // to spend the entire value, the new output must be signed by both owners
    return GetOutputAmount(0) + GetOutputAmount(1);
}

// get output value for owner 0 or 1
uint16_t ImpliedTransaction::GetOutputAmount(const int index) const
{
    assert(index == 0 || index == 1);
    if (index == 0) {
        return mOutputAmount1;
    }
    return mOutputAmount2;
}

// get public key of input owner 0 or 1
/*
bls::PublicKey ImpliedTransaction::GetInputOwner(const int index) const
{
    assert(index == 0 || index == 1);
    if (index == 0) {
        return bls::PublicKey::FromBytes(mInputOwner1.data());
    }
    return bls::PublicKey::FromBytes(mInputOwner2.data());    
}
*/
secp256k1_33 ImpliedTransaction::GetMultisigInputOwner(const int index) const
{
    assert(index == 0 || index == 1);
    if (index == 0) {
        return mInputOwner1;
    }
    return mInputOwner2;
}

// secp256k1 tag - should be used for MuSig now?
// get aggregated public key from transaction output owners
/*
bls::PublicKey ImpliedTransaction::GetAggregateOutputOwner() const
{
    if (IsMultisig()) {
        std::vector<bls::PublicKey> owners;
        bls::PublicKey pk1 = bls::PublicKey::FromBytes(mOutputOwner1.data());
        owners.push_back(pk1);
        bls::PublicKey pk2 = bls::PublicKey::FromBytes(mOutputOwner2.data());
        owners.push_back(pk2);
        return bls::PublicKey::Aggregate(owners);
    }
    return bls::PublicKey::FromBytes(mOutputOwner1.data());
}
*/

// Taken from BLS library
uint32_t ImpliedTransaction::FourBytesToInt(const uint8_t* bytes) const
{
    uint32_t sum = 0;
    for (size_t i = 0; i < 4; i++) {
        uint32_t addend = bytes[i] << (8 * (3 - i));
        sum += addend;
    }
    return sum;
}

}; // namespace lot49