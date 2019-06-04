# 5. Safety

Mesh nodes by default relay data without payment. Our incentive protocol augments the default relay heuristics to encourage better delivery for senders that spend value to incentivize relays. Misbehaving relay nodes should not be able to gain unearned value, but we can not always prevent limited free data delivery for data senders. We would like the network to err on the side of data being delivered.

With this framework in mind, we examine below some problems and general protections against degenerate behavior by nodes.

### 5.1 Offline Nodes

The normal security assumption for PCNs includes the possibility that nodes could be offline and not monitoring the blockchain. During this offline period, a malicious counter party could try to settle an out-of-date payment channel transaction. The offline node must react to this attempt and publish a more current state to prevent the malicious counterparty from rolling back payments.

Because mesh nodes will be offline most of the time we must consider this situation carefully. Our protocol must adapt the techniques used by other PCN protocols to the constraints of a mesh network.

#### Long Timeouts

The primary security for offline nodes comes from long delays for transactions that unilaterally close a channel. This gives the other party time to notice and submit a more current update. Our system will have to use timeouts adapted to the characteristics of the off-grid community. If users typically have internet access at home or via gateways once a day, then timeouts can be set shorter than if access is once a week. One consequence of this is that the less frequently nodes have internet access, the more incentive they have to cooperatively close channels to prevent locking up their liquidity.

#### Watchtowers

The Lightning Network white paper contemplates a private and trust minimized third party also called a “watchtower”\[12\]\[30\], who could be incentivized to always be online to watch for channels settled with out of date update transactions. It is more difficult to outsource channel monitoring for an offline mesh network node because it requires sending encrypted transactions to the watchtower. This may not be possible in a bandwidth efficient way over a mesh network. Instead it is more likely that witness nodes could be incentivized to monitor and notify offline nodes when channels are settled as an additional service to offline nodes. Watchtowers could also be opportunistically sent the latest fully signed update transaction held by a node. These checkpoints can be sent when near a gateway and could only be used by the watchtower when an invalid update appears on the blockchain.

### 5.2 Other Risks

#### Invalid Transactions

Channel updates from newly created payment channels should be considered potentially invalid until confirmed on the distributed ledger. If a channel funding transaction is included, but not ultimately confirmed, the transaction will become invalid. This means a relay node could owe their downstream node value when the channel funding transaction from their upstream node fails to validate.

For nodes that reciprocally relay messages for each other in roughly equal amounts, it is less critical how long it takes to confirm channel funding transactions on-chain because their channel balances will mostly net out close to zero.

Each node along a route can heuristically decide whether or not to relay a message when the transaction chain includes an unconfirmed transactions that funds a new channel. Priority would be given to messages that are not at risk from an unconfirmed channel funding transaction.

#### Unsigned for Delivery

The data receiver must reveal a preimage derived from the data for the incentive payment associated with that data to be valid. There is no way for the last relay node to know if their next transmission, which includes the data payload, will be overheard by the message receiver. A malicious message receiver could receive a message and choose not to acknowledge receipt.

Message receivers also receive incentive payments when they sign to prove they received the message. This is the primary way we encourage message receivers to follow the incentive protocol. However, when a message sender and receiver are colluding, or controlled by the same entity, this payment may not be relevant. For example, if the sender and receiver are the same entity then the sender can avoid the cost to send a message if the receiver always fails to sign and prove they received it. This situation can only be solved by nodes noticing the pattern and blacklisting responsible nodes.

#### Sybil Relays

Message destination nodes should receive whatever value was committed by the sender, but not used by relay nodes. This discourages relays from claiming more fees than they advertised. A message destination node that does not collect any value when they receive a message may not reveal the preimage that proves they received it. Also, the more value received by the destination node the more likely they are to forward the transaction to an internet gateway to be confirmed on the blockchain which reveals the preimage to other relays nodes that depend on that preimage.  If a relay wishes to earn more value they can publish a higher relay cost so nodes can incorporate this information in their routing metrics.

#### False Witness Nodes

A witness node can falsely confirm for an offline node that a transaction is valid. This could result in a relay node accepting payment from a new payment channel that in fact was not funded. This can be discouraged by using multiple witness nodes and by rechecking transactions when a node has direct internet connectivity. Witness nodes will earn tokens for confirming transactions and so have an incentive to perform their function honestly. Because anyone can run a witness node, those with the best uptime and longest track record should be favored.

#### Transaction Fees

The fees to settle transactions on the blockchain may exceed the value transferred as a micropayment to deliver a message. This could lead to payment transactions that are uneconomical to spend in the case of an uncooperative payment channel close.

### 5.3 Solutions

The risks described above would all require nodes to run a malicious version of the node client software. It is important that these attacks can at best result in free message delivery, but not result in unearned fungible value. This means only nodes using the system for message delivery have a reason to attack it. Solutions that isolate and reduce delivery success for malicious nodes should be sufficient to prevent attacks. Only an attack that could not be detected would be worth implementing.

#### Local Blacklists

If a node does not properly follow the incentive protocol, then any nearby nodes will overhear it at the RF radio layer. Locally misbehaving nodes can be safely ignored. For example, a destination node that does not sign to prove they received a message is either offline, out of range or misbehaving. To rule out channel errors, nodes should not be ignored until enough evidence of misbehavior has been observed. Whatever the reason, local nodes can safely stop routing messages to them. The incentive protocol extends trust to nodes beyond the range a message sender can directly monitor by allowing nodes to monitor transactions and identify routes more or less likely to result in confirmed message delivery.

Message receivers could still collude with senders to periodically change their identity, move and/or wait for new peers that have not witnessed their misbehavior when sending or receiving messages. However this would require establishing new payment channels and propagating new routing information to relays each time a node changes its identity.

#### Relay Heuristics

Ultimately, what is most important for a relay node that earns incentives for message delivery is how likely it is that a given message they relay will result in a successful and acknowledged message delivery. Delivery failure can occur for many reasons. The destination node could be offline, isolated or malicious. Downstream relays may likewise have communication failures that result in a message not being delivered. The only reliable data a node has is their own gathered statistics about success and failure for the messages they have sent or relayed. Heuristics based on this information are the best way for nodes to prioritize which messages they will relay.

A local heuristic that optimizes successful message delivery is also good for the network at-large. Flooding the network with messages that are unlikely to be received decreases the capacity of the network for everyone. Each node must balance their personal cost to transmit messages against the likelihood of successful delivery. A node must also participate in the overall relay protocol to be discoverable and learn new routes.

A node unconstrained by power or opportunities to transmit can always relay every message it receives. A power-constrained node or one located in a critical location may be unable to relay every message it receives. This should lead successful nodes to adopt a strategy that prioritizes relaying incentivized messages that are most likely to be delivered.

#### Rational Micropayments

To solve the problem of settling micropayments that are not economically viable due to transaction fees various solutions have been proposed \[41\]. For our system we believe probabilistic micropayments would be a viable option if they are supported by the Bitcoin script language.

It is also possible to amortize payments over multiple messages if the payment for a single message is not economical. In this case relay nodes take a larger risk that they will not receive payment until multiple messages have been delivered. However, this would be no worse than accepting micropayments that they are not able to spend.

Another approach currently implemented in the Lightning Network is to ‘trim outputs’ \[42\] that are below a predefined value. The value from trimmed outputs are committed to the transaction fee instead of creating an unspendable output. Trimmed outputs can later be credited to economically rational outputs when a payment channel is settled.

