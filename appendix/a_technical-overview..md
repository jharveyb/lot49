# Appendix A.Technical Overview

### Lot49 vs. Lightning Network

To reduce the amount of data exchanged between nodes for each payment we use a mechanism that requires fewer rounds of communication. To reduce the amount of data that must be settled on the blockchain through internet gateways we use a signature aggregation scheme. Specifically, this project proposes using the [eltoo](https://blockstream.com/eltoo.pdf) payment channel update mechanism and signature aggregation using the [MuSig](https://eprint.iacr.org/2018/068) scheme as described below. Using these techniques can reduce the amount of signature data exchanged between nodes by approximately 50%. With these changes we believe incentive overhead can be reduced to near 100 bytes per transmission on average.

Other ways Lot49 deviates from the existing Bitcoin Lightning Network are described below:

#### Data Delivery

The Lot49 protocol is primarily designed to use micropayments to incentivize successful delivery of data in a peer-to-peer communication network. Where possible we minimize the incentive protocol overhead required for micropayments to increase the bandwidth available for data delivery. Payments to relay nodes are small and fixed, or are based on the amount of data transferred, not the amount of value transferred. This proposal should be compatible with the [Sphinx keysend](https://github.com/lightningnetwork/lnd/pull/2455) system if that proposal is extended to support sending arbitrary data over the Lightning Network.

#### Network Topology

The Lot49 protocol assumes a mesh radio network where nodes can only communicate directly with other nodes within radio range. A micropayment is committed by a node to incentivize a nearby relay or gateway node to deliver their data. A node only directly incentivizes nodes they can directly communicate with. No such constraint exists in the Lightning Network because you can route a payment though any internet connected node.

#### Bitcoin Network Access

Nodes do not always have direct access to the internet to confirm bitcoin transactions on the Bitcoin network. Any communication with the Bitcoin network is via a gateway node with a direct connection to the internet. Sending transaction information to the Bitcoin network using relay and gateway nodes may require including incentive payments just like any other data originated from an off-grid node.

We assume the most bandwidth efficient method for off-grid nodes to confirm and commit transactions is to communicate with an internet connected node they control or trust. Other light client techniques are possible but not contemplated by this proposal.

#### Mesh Routing

Data is source-routed using a low-overhead technique specific to the mesh radio network being used. We assume nodes start with knowing a route, if available, to the node they wish to communicate with. The use of incentives to exchange accurate routing information or cryptography to validate route information is not contemplated in this proposal. We also do not assume using onion routing to keep route information private.

#### Inferred Transactions

To reduce the payment overhead communicated between nodes we assume only the minimal amount of data needed to update their payment state is ever communicated. This is similar to the way the Lightning Network peer-to-peer protocol currently works, as described in [Bolt \#2](https://github.com/lightningnetwork/lightning-rfc/blob/master/02-peer-protocol.md). Our proposal assumes even more standardization of default values to further reduce the amount of data transmitted between nodes. We also assume using hierarchical deterministic wallets, or similar techniques, to pre-share public key information between nodes. Because signature information represents the large set of data that can not be inferred and must be transmitted, we attempt to reduce the total amount of signature data that must be broadcast over the network.

#### Payment Network

Each message sent via a relay or gateway node includes a message header which commits to pay for proof the message was delivered. Enough value is committed to pay every relay node and the message receiver a previously agreed amount. Each message must also include the preimage that unlocks the HTLC incentive payments. The preimage should be encrypted so that only the message receiver can decrypt it, and only if they receive the full message.

#### eltoo

We propose using the eltoo channel state update mechanism because it allows simplex payments between nodes to update their payment state. After a channel has been set up, only the node that is forwarding a payment must transmit their signatures to the payment receiver. The payment sender does not need the payment receiver’s signatures because they will always have their own signatures for the last payment they received, or for the opening refund transaction. This reduces the amount of data that must be communicated to update a channel by more than half. With the current Poon-Drjya update scheme both sides need to transmit two signatures and revocation information from the last state.

#### MuSig

When two nodes commit to an eltoo trigger, refund or closing transaction, they would normally need to communicate two 64 byte signatures to the Bitcoin blockchain to spend from the 2-of-2 multisig transaction. The MuSig signature aggregation scheme can reduce this to a single multisignature for both signers. In the general MuSig signing session case, each signer transmits 96 bytes of information in three communication rounds to create a single 64 byte multisignature. However, for the special case of a two-party session, only one node needs to send a nonce commitment before the other party sends their nonce, thus saving 32 bytes. This means that two nodes exchange only 32 bytes more data to create an aggregate signature than if they exchanged two full 64 byte signatures. Each relay hop that the transaction must be sent to reach the internet requires half the bandwidth when sending a single multisignature compared to sending two 64 byte signatures.

### Open Questions

#### Public Keys

This public key information must be exchanged between payment channel partner that cosign transactions. The eltoo protocol also requires using new public keys for every new settlement transaction. The most space efficient way to exchange public keys is with some pre-shared table that maps the extended public key of a node to a shorter node ID. The nonce or index to specify the exact path of each public key would also need to be sent for each transaction. We have not specified how this public key information will be communicated and used to recreate complete transactions that can be settled on the Bitcoin network.

#### Bitcoin Script

We have not yet specified how the Bitcoin scripts that would be used differ from the ones described in the eltoo paper and lightning network BOLT specification. We also need to specify how to reduce the amount of data needed to reconstruct these scripts from the data communicated between nodes.

#### Peer-to-Peer Messages

We still need to specify the exact messages and message passing sequence used to set up, update, and close payment channels between nodes.

#### Off-chain Optimizations

Techniques that minimize the amount of data that must be communicated via internet gateways and settled on the Bitcoin network deserve further exploration. Ideally we could setup, close and monitor channels with minimal or no communication through internet gateways. It should also be possible to amortise many payments and channel operations into a small number of infrequent on-chain transactions. Some potential ways to do this include:

#### Channel Factories

Instead of always anchoring payment channels with a new on-chain set up transaction, the eltoo update mechanism enables the use of [channel factories](https://www.tik.ee.ethz.ch/file/a20a865ce40d40c8f942cf206a7cba96/Scalable_Funding_Of_Blockchain_Micropayment_Networks%20%281%29.pdf) to negotiate opening and closing channels with a federation of available off-grid nodes. Channel factories can also reduce the time required to create new channels between off-grid nodes.

#### Light Client Support

Broadcast block headers and compact block filter digests using the scheme of BIP-158 in order to keep nodes on a mesh subnet in sync with the global blockchain with minimal access to the distributed ledger. This method would reduce the need for trusted witness nodes.

