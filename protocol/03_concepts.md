---
description: >-
  Below we describe the foundational concepts used by our protocol. Those
  already familiar with the current Bitcoin Lightning Network can also skip to
  Appendix A for a technical overview of the changes
---

# 3. Concepts

### 3.1 Aggregate Signatures

The Bitcoin network uses the Elliptic Curve Digital Signature Algorithm \(ECDSA\) to sign transactions that transfer bitcoin from one address to another. Transactions are verified by checking associated ECDSA signatures. Even if other details of a transaction can be inferred or reconstructed, signatures must be supplied by the entity that authorizes a transaction. Because we want to optimize our protocol for low-bandwidth communication channels, we focus on how to reduce the size of the signatures that authorize transactions.

In particular, we would like a way to reduce the size of signatures for transactions with multiple signers. Multisignatures \[18\] offers a solution for combining the signatures from two or more signers on a single message into a single signature. This technique can dramatically reduces the amount of data needed to communicate a multi-signature transaction.

Unfortunately, no efficient scheme exists to aggregate ECDSA signatures so proposals for signature aggregation of Bitcoin transactions require a new signature algorithm. Schnorr signatures support has been proposed for Bitcoin [\[17\]]() and would, among other things, provide support for the MuSig [\[18\]]() multisignature schemes. One problem with using MuSig to reduce communication overhead is that it requires multiple rounds of interaction between signers. A suggestion for non-interactive half-aggregation [\[19\]]() of Schnorr signatures has also been made, but would create signatures that grow in size with each signature added.

The BLS [\[20\]]()[\[21\]]() signature scheme has also been suggested as a solution for creating multi-signatures for Bitcoin transactions [\[22\]](). BLS signatures can also be used for general signature aggregation [\[23\]](). Unfortunately, Bitcoin is unlikely to adopt BLS signatures due to their more complicated security assumptions and higher computational requirements for verification. Although BLS signatures require more computation to verify signatures, computation is relatively less power- and time-consuming than transmitting data for mobile devices [\[24\]](). For low-bandwidth mobile mesh networks the increased verification time of pairing based signature schemes like BLS is justified by the reduction in data transmission achieved using signature aggregation.

We discuss the trade-offs of using Schnorr multisignatures and BLS signature aggregation in [Appendix D](). Our conclusion is that Schnorr multisignatures have the best technical tradeoffs for this protocol.

### 3.2 Payment Channels

Payment channels [\[10\]]()[\[11\]]() are used to make local bidirectional payments between nodes. They reduce the transactions that must be settled on the distributed ledger to only setup and settlement transactions. Update transactions exchanged directly between nodes to modify the current split of value in a shared multisignature address do not need to be immediately communicated to an internet connected Bitcoin node.

Validating many small transfers between nodes using the Bitcoin blockchain directly would require internet access for each payment. With payment channels, value is locked in a shared transaction output which both nodes must authorize any spending from. Nodes directly negotiate how tokens stored in the shared transaction’s outputs should be divided. Nodes only need to communicate the final balance split with a final settlement transaction sent to the blockchain once they are done exchanging payments.

![Figure 4: At time t1 Node B proposes a payment channel by signing a Refund transaction back to Node A. At time t2 Node A opens the channel by signing a Setup transaction to fund the channel and both paths of an UpdateAndSettle transaction to offer a new state update to Node B. To be valid the transaction requires Node B&#x2019;s signature and a secret that proves message m was delivered. Node B can update the transaction state at time t3 by adding their own signature and the secret for message m that proves it was delivered. At time t4 Node A can sign a final Close transaction to make the transfer to Node B final without a delay if Node B presents the secret that proves delivery for a new message m.](../.gitbook/assets/fig4.svg)

Payment channel transactions use scripts that encode the rules for how to update and settle the value split between nodes. These rules include a timeout so that if one node stops responding, the other can finalize the most recently settled split. This timeout is tuned to prevent a malicious node from trying to finalize an out of date balance update. In our usage, settlement transactions also require proof that a message has been delivered. This makes channel updates valid only when an associated message delivery is successful.

![Figure 5: Pseudo code for UpdateAndSettle output script that must be satisfied to spend from a payment channel transaction \(eg. Tx0,AB\). The first path transfers tokens from the jointly controlled transaction 1 week after the last channel update was committed. The second path immediately transfers all of the value to a new Tx1,AB controlled by both Nodes A and B if the new transaction state is greater than the old state and if Node D has revealed the preimage associated with message m1. If the update transaction is signed and settled before the 1 week delay, then the update to the next state of the channel \(with a different value split\) makes the old settlement transaction invalid.](../.gitbook/assets/fig5.svg)

Our payment channels are based on Decker-Russell-Osuntokun eltoo channels [\[25\]]() and assume a conditional input script that either cooperatively transfers the tokens to an address with a newer state or settles the channel unilaterally after some delay \(Figure 5\).

For example, Node A commits to pay Node B to relay data m1 to message destination node D by signing a settlement transaction that unlocks the balance controlled by Tx0,AB with output script _UpdateAndSettle_ \(figure 6\). If Node B also signs the script and reveals the secret from Node D for the data m1 then Node B can settle the payment channel one week after the transaction that funded Tx0,AB is settled. Tokens controlled by the transaction Tx0,AB will be distributed based on the allocation \(eg. 7 to Node A and 13 to Node B\) Node A committed to pay to Node B if data m1 was delivered.

![Figure 6: The pseudo code Settle input script is only valid if it includes signatures from the settlement public keys used for state 0 by Nodes A and B and 1 week has passed since the last update transaction.](../.gitbook/assets/fig6.svg)

Even if Node A becomes unresponsive, as long as Node B has proof that Node A also signed the transaction and Node D’s signature for m1 proving it was delivered, then Node B can settle the payment channel after one week. However, if Node B wishes to commit to a new split of tokens with Node A, Node B can simply prove to Node A they have Node D’s signature for m1. Both nodes can then safely negotiate a new split that assumes the previous agreed settlement split was confirmed. Node A can now sign a new transaction with the same _UpdateAndSettle_ output script but with a newer state. Node A would sign a commit to the new split that pays Node B more tokens that the previous split conditioned on proof that some new data was delivered to a destination node.

![Figure 7: The pseudo code Update input script immediately transfers value from the previous state to a new one controlled by both Nodes A and B if it includes signatures from Nodes A and B, Node D&#x2019;s secret for message m1 and if the state of the new transaction is greater then the current one.](../.gitbook/assets/fig7.svg)

In this way, nodes can make arbitrarily many bi-directional transfers between themselves without needing internet access to connect to the distributed ledger. If either node becomes unresponsive, then both will have enough information to settle the last payment they received after some delay. If at any point both nodes wish to settle the current state of their balances, they can cooperatively sign a closing transaction that does not have a 1 week delay encumbrance. The closing transaction can also be negotiated with an aggregate signature to reduce the amount of data that needs to be settled online.

### 3.3 Payment Channel Network

Our design builds off the concept of a payment channel network \(PCN\) pioneered by the Bitcoin Lightning Network [\[12\]](). In this type of network, pairs of nodes connected by payment channels are organized into a connected graph that can safely route payments between nodes that do not have a direct payment channel established.

![Figure 8: Existing PCNs communicate via the internet and route payments via a logical network of payment channels.](../.gitbook/assets/fig8.svg)

We propose a system that routes both data and payments over the same point-to-point communication channels. Current PCNs form an overlay network that routes payments over logical connections between nodes, but communicate using a different set of internet connected data routers.

Our physical PCN creates payment channels directly between nodes within radio range based on their physical location. Both data and payments are routed over the same network topology.

![Figure 9: In a mesh based PCN nodes route payments and data together over the same physical links.](../.gitbook/assets/fig9.svg)

Existing online PCNs assume internet connectivity and direct access to the distributed ledger to confirm channel funding transactions when a new channel is set up.

In our physical PCN, transactions to set up and settle payment channels on the distributed ledger must be relayed through the mesh to an internet gateway.

### 3.4 Implied Transaction

Nodes transmit incentive headers to commit to payment channel transactions that transfer value from one node to another. The commitment to a transaction can be verified by checking the signatures created for these transactions. These transactions are not explicitly included in each incentive header. Instead nodes only transmit the information for each transaction that cannot be otherwise inferred from other information in the transmission.

For example, an incentive header might include a payment channel Update Tx transaction sent from Node A to Node B but does not need to include the payment addresses for Nodes A and B. These addresses can be deterministically computed by looking up a cached extended public key \(xpub\) [\[26\]]()[\[27\]]() for the nodes involved. For example, addresses can be deterministically generated by multiplying cached xpubs for Nodes A and B by a known nonce, such as the hash of the message. xpubs can be exchanged and cached when new node IDs are first encountered or synced when a node is online.

Nodes must also aggregate their signatures for an implied transaction to be valid. Any node that reconstructs an implied transaction can use the aggregate signature to confirm the correct implied payment addresses were included in the implied transaction.

An important implied value in our protocol is a secret value computed from the payload data. This value is committed to by each message sender along with their payment. The message sender generates a secret nonce for each message and encrypts it with the message payload. Only the destination node can decrypt the payload and recover the secret nonce.

Payment channel update transactions are not valid unless the data being sent is received by the destination node; this condition can only be satisfied after the destination node decrypts the message and returns the secret nonce.

Transactions are also predefined to eliminate values that can be standardized. For example, we can create a standard relay fee where each node pays one less token than it received. This information need not be transmitted to reconstruct the implied transactions. Knowing the starting relay payment amount and how many hops have occured is enough to reconstruct the payment amounts of every implied transaction. We can also allow nodes to set different values for the payment they will take. For example, a node that bridges two large subnets could charge more than a node within a large well connected network. Variable relay fees can be implemented by adding a few more bits of information to the hint for each hop.

Large values like extended public keys can be looked up in locally-stored caches from shorter node IDs that map to the larger public key value. If a particular value is not already locally cached, it can be transmitted first in long-form and from then on only the shorter hash ID need be sent.

![Figure 10: An implied transaction transmitted from Node A to Node B includes hints that imply a transaction where Nodes A will pay 3 tokens to Node B if Node D signs for receipt of data m1 by returning a value that matches the preimage condition Hash\(msecret,1\). The normal routing header should already include the IDs for Nodes A and D, so the only additional information that must be transmitted is the number of tokens Node A spends \(eg. 3\) and the signature signed by Node A \(eg. SigA\).](../.gitbook/assets/fig10.svg)

![Figure 11: Node B can reconstitute the full signed transaction from the hints and routing header sent by Node A and from their public key cache. The implied transaction updates the payment channel between Nodes A and B to a new state if Node B has the preimage encrypted for Node D with message m1.](../.gitbook/assets/fig11.svg)

### 3.6 Witness

When a new payment channel is set up, a funding transaction commits tokens to the channel. Until the funding transaction is validated and settled by the consensus mechanism of the blockchain, there is a risk the funding payment can be double-spent.

Nodes could keep an up-to-date view of settled transactions by operating as a full node that witness all new ledger updates as they are produced. However, over a low bandwidth mobile mesh network the overhead of broadcasting this information to all nodes is impractical. Instead, offline mesh nodes will delegate this task to online witness nodes they operate themselves or trust.

Online witness nodes should always return the secret value included with the  transaction that they receive from offline nodes to indicate they received the request. Witness nodes should also return a message to the requesting offline node that commits to whether or not the transaction is valid on the Bitcoin blockchain.

Interactions between offline mesh nodes and internet connected witness nodes are handled like a normal communication between nodes, but must include an internet gateway node. Validating a transaction requires making a payment to the relay nodes and gateway node that relays the information across the mesh network to the internet. Like other destination nodes, witness nodes will receive payment for confirming receipt of a valid payload, in this case the payload is a transaction that must also be confirmed by the Bitcoin network. A payment sent to a witness node could also require one of two valid preimages. This would enable the witness node to confirm the message was received and signal to the sender that the  transaction was successfully confirmed or not depending on which preimage they return.

### 3.7 Settlement

We propose using the Bitcoin blockchain as a permissionless distributed ledger to maintain an authoritative global record of incentive payment transactions. A distributed ledger ensures there is no central point of failure that could compromise the operation of the system. The alternative to this approach would be a centralized ledger run by a single or federated group of operators. A centralized ledger would be technically easier to scale by leveraging well-established database technology, but would not be as socially scalable [\[28\]]() or as resilient to downtime. Trusted third parties are single points of failure that introduce the same censorship and rent-seeking opportunities available to centralized ISPs and mobile carriers.

Bitcoin was the first successful permissionless decentralized ledger network and remains the most reliable example. It has proven its resilience with over 99.99% uptime since 2009 [\[29\]](). It’s ability to resist attacks in a highly adversarial environment has established the Bitcoin network as the world’s preeminent decentralized financial ledger.

We also plan to use Bitcoin in order to take advantage of the many scaling and security features that have been tested and refined for the Bitcoin protocol. However, to allow us to verify transactions over bandwidth constrained mesh networks composed of mostly offline nodes we need to a support for a signature aggregation scheme like MuSig. For this reason we do not propose to initially settle transactions on the existing Bitcoin ledger, but will instead use a new Bitcoin testnet that includes the modifications we need to test the Lot49 protocol. In particular, Schnorr signatures [\[17\]]() and the SIGHASH\_NOINPUT signature hash flag [\[43\]]().

During the initial testing phase of the network we anticipate running a single centralized instance of the ledger. During this phase token issuance and redemption will be pegged to some nominal unit of value. This approach removes speculative price volatility and ensures tokens will only be acquired to be used for data delivery. Tokens will be redeemed by users that have earned an excess by their efforts relaying data for others.

Once the necessary protocol changes have been adopted by the Bitcoin community on the main Bitcon network, we will transition to using it for settlement. We could also potentially use a federated sidechain [\[38\]]() to create a network with value pegged to the main Bitcoin blockchain if the protocol changes we expect to use are not adopted by the Bitcoin community.

