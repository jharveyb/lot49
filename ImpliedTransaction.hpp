
#include "bls.hpp"
extern "C" {
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <secp256k1_schnorrsig.h>
#include <secp256k1_musig.h>
#include <hash.h>
#include <util.h>
#include <hash_impl.h>
}

#pragma once

namespace lot49
{
// Hashed GID
typedef uint16_t HGID;

// add types related to secp256k1; always use compressed keys
// pending change to only use even-signed keys would remove need for 33-byte type

// Needed to use std::array over C-style arrays
const size_t num_signers = 2;
const size_t seckeysize = 32; 
const size_t noncesize = 32; 
const size_t hashsize = 32; 
const size_t pubkeysize = 33; // always serialize keys in compressed form
const size_t sigsize = 64;
typedef std::basic_string<unsigned char> ustring;
typedef std::array<uint8_t, seckeysize> secp256k1_32;
typedef std::array<uint8_t, pubkeysize> secp256k1_33;
typedef std::array<uint8_t, sigsize> secp256k1_64;

void ReadCSPRNG(const std::string &source, char* outbuf, uint8_t readsize);

enum ETransactionType {
    eIssue,
    eTransfer,
    eSetup,
    eRefund,
    eUpdateAndSettle,
    eClose
};

class ImpliedTransaction {
    public:

    // create implied transactions
    static ImpliedTransaction Issue(const bls::PublicKey& inReceiver, const uint16_t inFundingAmount);
    static ImpliedTransaction MultisigIssue(const secp256k1_33& inReceiver, const uint16_t inFundingAmount);
    static ImpliedTransaction Issue(const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, const uint16_t inFundingAmount);
    static ImpliedTransaction MultisigIssue(const secp256k1_33& inSender, const secp256k1_33& inReceiver, const uint16_t inFundingAmount);
    static ImpliedTransaction Transfer(const ImpliedTransaction& inInput, const bls::PublicKey& inSource, const bls::PublicKey& inReceiver, 
        const uint16_t inFundingAmount);
    static ImpliedTransaction MultisigTransfer(const ImpliedTransaction& inInput, const secp256k1_33& inSource, const secp256k1_33& inReceiver,
        const uint16_t inFundingAmount);
    static ImpliedTransaction Setup(const ImpliedTransaction& inInput, const bls::PublicKey& inSource, const bls::PublicKey& inReceiver, 
        const uint16_t inFundingAmount);
    static ImpliedTransaction MultisigSetup(const ImpliedTransaction& inInput, const secp256k1_33& inSource, const secp256k1_33& inReceiver,
        const uint16_t inFundingAmount);
    static ImpliedTransaction Refund(const ImpliedTransaction& inInput, const bls::PublicKey& inSource, const bls::PublicKey& inReceiver, 
        const bls::PublicKey& inSigner, const uint16_t inRefundAmount);
    static ImpliedTransaction MultisigRefund(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver,
        const secp256k1_33& inSigner, const uint16_t inRefundAmount);
    static ImpliedTransaction UpdateAndSettle(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, 
        const bls::PublicKey& inSigner, const uint16_t inSenderAmount, const uint16_t inReceiverAmount, const bls::PublicKey& inDestination, 
        const std::vector<uint8_t>& inMessageHash);
    static ImpliedTransaction MultisigUpdateAndSettle(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver,
        const secp256k1_33& inSigner, const uint16_t inSenderAmount, const uint16_t inReceiverAmount, const secp256k1_33& inDestination,
        const std::vector<uint8_t>& inMessageHash);
    static ImpliedTransaction Close(const ImpliedTransaction& inInput, const bls::PublicKey& inSender, const bls::PublicKey& inReceiver, 
        const bls::PublicKey& inSigner, const uint16_t inSenderAmount, const uint16_t inReceiverAmount);
    static ImpliedTransaction MultisigClose(const ImpliedTransaction& inInput, const secp256k1_33& inSender, const secp256k1_33& inReceiver,
        const secp256k1_33& inSigner, const uint16_t inSenderAmount, const uint16_t inReceiverAmount);

    // default ctor
    ImpliedTransaction();

    // operators to sort/map transactions
    bool operator==(const ImpliedTransaction& rval) const;
    bool operator<(const ImpliedTransaction& rval) const;

    // get short transaction ID
    uint32_t GetId() const;

    // compute hash of this transaction
    std::vector<uint8_t> GetHash() const;

    // get short transaction ID of input transaction
    uint32_t GetInputId() const;

    // get hash of input transaction
    std::vector<uint8_t> GetInputHash() const;

    // compute serialization of the transaction
    std::vector<uint8_t> Serialize() const;

    ETransactionType GetType() const { return mType; }

    // public key of the transaction signer
    const bls::PublicKey GetSigner() const;

    const secp256k1_33 GetMultisigSigner() const;

    // aggregate public key of signer with public key of other signer
    bool AddSigner(const bls::PublicKey& inSigner);

    bool AddMultisigSigner(const secp256k1_33& inSigner);

    // return true if transaction output must be signed by public keys of two owners
    bool IsMultisig() const;

    // get public key of output owner 0 or 1
    bls::PublicKey GetOutputOwner(const int index) const;

    secp256k1_33 GetMultisigOutputOwner(const int index) const;

    // get total output amount
    uint16_t GetOutputAmount() const;

    // get output value for owner 0 or 1
    uint16_t GetOutputAmount(const int index) const;

    // get public key of input owner 0 or 1
    bls::PublicKey GetInputOwner(const int index) const;

    secp256k1_33 GetMultisigInputOwner(const int index) const;

    // get public key aggregated from all transaction output owner public keys
    bls::PublicKey GetAggregateOutputOwner() const;

    secp256k1_33 GetMusigOwner() const;

    // Taken from BLS library
    uint32_t FourBytesToInt(const uint8_t* bytes) const;

    private:

    ETransactionType mType;
    // TODO: input transaction hash should correctly point to specific tx or be empty if this tx is signed with NOINPUT signature flag
    std::vector<uint8_t> mInputTxHash; 
    // TODO: public keys should be generated from an extended public key + nonce
    secp256k1_33 mInputOwner1;
    secp256k1_33 mInputOwner2;
    secp256k1_33 mOutputOwner1;
    secp256k1_33 mOutputOwner2;
    uint16_t mOutputAmount1;
    uint16_t mOutputAmount2;
    uint8_t mTimeDelay;
    uint8_t mChannelState;
    secp256k1_33 mMessageSigner; // public key of destination node that must sign the message hash
    secp256k1_32 mMessageHash;

    // public key of the transaction signer 
    // NOTE: also part of the serialization and transaction hash to avoid two signers signing identical transactions
    secp256k1_33 mTransactionSigner;
    secp256k1_33 mTransactionSecondSigner;
};
}