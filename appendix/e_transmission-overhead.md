# Appendix E. Transmission Overhead

In this appendix we analyze the transmission overhead of three smart contract systems for paying incentives to mobile mesh nodes for successful message delivery. This analysis focuses on the communication overhead to update channel states and close channels. We assume the channel setup overhead is roughly similar for all three. We also assume the same simplifying optimizations have been made to all three systems to minimize communication overhead. For each system we analyze the total communication overhead needed to update the payment channels of an example three hop message delivery. We also examine the amount of data that must be communicated to the internet to close all three channels.

The three smart contract systems are based on the following protocols:

* Poon-Dryja state revocation \(with ECDSA signatures\)
* eltoo state revocation \(with Schnorr MuSig multi-signatures\)
* eltoo state revocation \(with BLS non-interactive signature aggregation\)

The Poon-Dryja state revocation protocol requires multiple communication phases for both the sender and receiver of a payment to exchange commitments to the new state and revoke the previous state. The eltoo state revocation protocol uses transaction replacement and only requires the payment sender to commit to a new channel state. In the eltoo/Schnorr variant all nodes involved in the message delivery only sign two transactions to update their channels along the route. In the eltoo/BLS variant each node signs a transaction to update their channel with the next node in the route.

### Schnorr and MuSig Multisignatures

The MuSig\[18\] signing scheme requires three rounds of communication for a set of nodes to collaboratively generate a multi-signature for a transaction. For each multi-signature, a node must exchange with each other signing node a 32 byte nonce commitment, a 32 byte nonce and a 32 byte partial signature. A valid multi-signature is composed of a 32 byte combined nonce and 32 byte combined partial signature.

This means that two nodes transmit 96 bytes to each other, and then either node can transmit a single 64 bytes of signature information to the blockchain. In total they transmit 256 bytes for a single signing session.

Without multi-signatures, each node needs to transmit their complete 64 byte signature to the other node, and two 64 byte signatures to the blockchain for a total of 256 bytes.

The total is the same, but the ratio of local transmission versus final signature size is different.

#### Example

Assume four nodes create setup transactions for three channels \(A &lt;&gt; B &lt;&gt; C &lt;&gt; D\) with their neighbors. Each channel setup transaction requires two signatures. Also assume transactions must be relayed over 3 hops to be settled via an internet gateway node.

#### One Multisignature per channel

If we create one multi-signature with two signers per channel, then we must transmit 2 \* 96 bytes \(nonce commitment, nonce, partial signature\) per channel to create one 64 byte multi-signature. The total transmitted to sign setup transactions for each of the three channels is 3 \* 2 \* 96 = 576 bytes. To transmit the multi-signature for a single channel three hops to a gateway requires transmitting 64 \* 3 hops = 192 bytes, or an additional 576 bytes for three channels. The total transmitted is thus 576 + 576 = 1152 bytes.

#### No Multisignature

Without multi-signatures, each pair of nodes would exchange a 64 byte signature with their channel partner, or 64 \* 2 \* 3 channels = 384 bytes of information. Each of the three channels will also need to send 64 \* 2 = 128 bytes of signature information to the blockchain over three hops for a total of 128 \* 3 \* 3 hops = 1152 bytes of transmission. The total data transmitted would be 384 + 1152 = 1536 bytes.

#### BLS Signatures

Each BLS signature is 48 bytes and can be non-interactively aggregated with other {message, signature} pairs. Signing three channels would require transmitting 48 \* 2 \* 3 = 288 bytes of signature data to create an aggregate signature of 48 bytes for each channel. The total data transmitted would be 288 + 48 \* 3 \* 3 hops = 720 bytes.

There are also additional optimizations that can be made that would reduce this number further such as only transmitting a single signature to sign all three setup transactions.

#### Conclusion

This analysis only focuses on signature data, which is the largest but not the only information that must be transmitted between nodes and between nodes and the internet via gateways.

While BLS signatures are by far the most space saving option, we can achieve a substantial improvement over current ECDSA multisig signing using MuSig and Schnorr signatures. Not only would this technique reduce overall signature transmission overhead by 50%, but it would reduce the amount of signature data that must be transmitted via relays to a gateway by 50%.

Importantly, Schnorr based MuSig multi-signatures are likely to be supported by Bitcoin in the foreseeable future, whereas BLS signatures are not currently supported by any widely used distributed ledgers.

### Simplifying Assumptions:

We assume all three systems use the following simplifications and optimizations to reduce the bandwidth required to update their channel state.

* Each node has a short 2-byte node ID.
* The extended public key for each node ID is pre-shared.
* New public keys are deterministically derived from the extended public key for each transaction.
* A route of relays nodes from the sender to a destination node are computed by the sender and communicated in clear text \(no onion routing\).
* Transactions, including script details and defaults, can be inferred from a 1 byte type ID.
* The destination node signs a hash of the message to confirm receipt and unlock channel updates between nodes.

For this analysis, the following data structures are used to compute the overhead of simplified versions of the protocol messages described in “[Bolt \#2](https://github.com/lightningnetwork/lightning-rfc/blob/master/02-peer-protocol.md) : Peer Protocol For Channel Management.” using the current Lightning Network’s Poon-Dryja update scheme, or a proposed Lot49 scheme based on the eltoo update scheme.

### LN Update Channel

The channel update phase \(described in the [Normal Operation](https://github.com/lightningnetwork/lightning-rfc/blob/master/02-peer-protocol.md#normal-operation) section of Bolt \#2\) requires the update proposer to send the update\_add\_htlc message, and then both the sender and receiver to commit to a new channel state and revoke the previous channel state.

#### update\_add\_htlc

|  | Original Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 128 \(update\_add\_htlc\) |
| channel\_id | 32 | - | inferred from destination node |
| id | 8 | 2 | ID to refer to this new HTLC |
| amount\_msat | 8 | 2 | smaller max payment range |
| payment\_hash | 32 | - | inferred from message data |
| cltv\_expiry | 4 | - | use standard value |
| onion\_routing\_packet | 1366 | 12 | up to 6 clear text hops, one 2 byte node ID per hop |
| Total |  | 17 |  |

#### 

#### commitment\_signed

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 132 \(commitment\_signed\) |
| channel\_id | 32 | - | inferred from destination node |
| signature | 64 | 64 | sign remote commitment |
| num\_htlcs | 2 | - | assume single htlc signature |
| htlc\_signature | num\_htlcs\*64 | 64 | commit to previous HTLC payments |
| Total |  | 129 |  |

#### revoke\_and\_ack

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 132 \(commitment\_signed\) |
| channel\_id | 32 | - | inferred from destination node |
| per\_commitment\_secret | 32 | 32 | revoke last commitment |
| next\_per\_commitment\_point | 33 | 33 | new commitment point |
| Total |  | 66 |  |

#### Example

To update three channels requires transmitting the update\_add\_htlc message once per channel and the commit\_signed and revoke\_and\_ack messages six times.

Unfortunately a Schnorr multi-signatures can not be used to reduce the overall amount of data transmitted between nodes to update a channel. Signatures are only included in the commitment\_signed messages and nodes only update their own signature; the signatures from both sender and receiver are not transmitted together during a channel update. Both parties signatures are only combined when a channel is closed cooperatively by signing a partially signed closing transaction or uncooperatively by signing an unrevoked HTLC transaction

<table>
  <thead>
    <tr>
      <th style="text-align:left">Message Type</th>
      <th style="text-align:left">Simplified Size (bytes)</th>
      <th style="text-align:left">Total data transmitted by both sender and receiver (bytes)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="text-align:left">
        <p>Update three channels</p>
        <p>ECDSA or Schnorr</p>
      </td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
    </tr>
    <tr>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
    </tr>
    <tr>
      <td style="text-align:left">update_add_htlc*</td>
      <td style="text-align:left">17</td>
      <td style="text-align:left">51</td>
    </tr>
    <tr>
      <td style="text-align:left">commitment_signed</td>
      <td style="text-align:left">129</td>
      <td style="text-align:left">774</td>
    </tr>
    <tr>
      <td style="text-align:left">revoke_and_ack</td>
      <td style="text-align:left">66</td>
      <td style="text-align:left">396</td>
    </tr>
    <tr>
      <td style="text-align:left">Total:</td>
      <td style="text-align:left"></td>
      <td style="text-align:left">1221</td>
    </tr>
  </tbody>
</table>\* transmitted three times by payment sender only

### LN Channel Setup

The setup phase for a new payment channel \(described in the [Channel Establishment](https://github.com/lightningnetwork/lightning-rfc/blob/master/02-peer-protocol.md#channel-establishment) section of Bolt \#2\) requires sending the following messages: the funding node sends ‘open\_channel’, the receiving node sends back ‘accept\_channel’, the funding node sends ‘funding\_created’, the receiving node sends back ‘funding\_signed’, the funding node sends ‘funding\_locked’ and finally the receiving node replies with its own ‘funding\_locked’ message.

For the simplified versions of these messages it is assumed that we can use defaults for many of the parameters exchanged to open a channel, and that information about the funding pubkey is transmitted, but the ‘basepoint’ information can be inferred. For example, using BIP-32 extended public keys, and agree \(either in the protocol spec or in the protocol messages\) on which BIP-32 child key derivation paths \(child paths\) to use.



#### open\_channel

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 32 \(open\_channel\) |
| chain\_hash | 32 |  | Use defaults |
| temporary\_channel\_id | 32 | 2 | inferred from destination node |
| funding\_satoshis | 8 | - | use defaults |
| push\_msat | 8 | - | use defaults |
| dust\_limit\_satoshis | 8 | - | use defaults |
| max\_htlc\_value\_in\_flight\_msat | 8 | - | use defaults |
| channel\_reserve\_satoshis | 8 | - | use defaults |
| htlc\_minimum\_msat | 8 | - | use defaults |
| feerate\_per\_kw | 4 | - | use defaults |
| to\_self\_delay | 2 | - | use defaults |
| max\_accepted\_htlcs | 2 | - | use defaults |
| funding\_pubkey | 33 | 33 | DER Encoded secp256k1, half of 2:2 multisig |
| revocation\_basepoint | 33 | - | inferred ? |
| payment\_basepoint | 33 | - | inferred ? |
| delayed\_payment\_basepoint | 33 | - | inferred ? |
| htlc\_basepoint | 33 | - | inferred ? |
| first\_per\_commitment\_point | 33 | - | inferred ? |
| channel\_flags | 1 | - | not used |
| shutdown\_len | 2 | - | not used |
| shutdown\_scriptpubkey | shutdown\_len | - | not used |
| Total |  | 36 |  |

#### accept\_channel

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 33 \(accept\_channel\) |
| temporary\_channel\_id | 32 | 2 | inferred from destination node |
| dust\_limit\_satoshis | 8 | - | use defaults |
| max\_htlc\_value\_in\_flight\_msat | 8 | - | use defaults |
| channel\_reserve\_satoshis | 8 | - | use defaults |
| htlc\_minimum\_msat | 8 | - | use defaults |
| minimum\_depth | 4 | - | use defaults |
| to\_self\_delay | 2 | - | use defaults |
| max\_accepted\_htlcs | 2 | - | use defaults |
| funding\_pubkey | 33 | 33 | DER Encoded secp256k1, half of 2:2 multisig |
| revocation\_basepoint | 33 |  | inferred ? |
| payment\_basepoint | 33 |  | inferred ? |
| delayed\_payment\_basepoint | 33 |  | inferred ? |
| htlc\_basepoint | 33 |  | inferred ? |
| first\_per\_commitment\_point | 33 |  | inferred ? |
| shutdown\_len | 2 | - | not used |
| shutdown\_scriptpubkey | shutdown\_len | - | not used |
| Total |  | 36 |  |

#### funding\_created

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 34 \(funding\_created\) |
| temporary\_channel\_id | 32 | 2 | inferred from destination node |
| funding\_txid | 32 | - | inferred |
| funding\_output\_index | 2 | - | inferred |
| signature | 64 | 64 | 96 for MuSig interactive signing |
| Total |  | 67 |  |

#### funding\_signed

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 35 \(funding\_signed\) |
| channel\_id | 32 | 2 | inferred from destination node |
| signature | 64 | 64 | 96 for MuSig interactive signing |
| Total |  | 67 |  |

#### funding\_locked

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 36 \(funding\_locked\) |
| channel\_id | 32 | 2 | inferred from destination node |
| next\_per\_commitment\_point | 33 | - | inferred ? |
| Total |  | 3 |  |

### Summary for Setup

#### Example

To setup three channels requires transmitting the above messages once per channel and the ‘funding\_locked’ message twice per channel. Using Schnorr multi-signatures requires each signer send an addition 32 bytes as part of the interactive signing protocol.

<table>
  <thead>
    <tr>
      <th style="text-align:left">Message Type</th>
      <th style="text-align:left">Simplified Size (bytes)</th>
      <th style="text-align:left">Total data transmitted by both sender and receiver (bytes)</th>
      <th style="text-align:left"></th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="text-align:left">
        <p>Setup three channels</p>
        <p>ECDSA or Schnorr</p>
      </td>
      <td style="text-align:left">Setup three channels with Schnorr Multi-Signature</td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
    </tr>
    <tr>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
    </tr>
    <tr>
      <td style="text-align:left">open_channel</td>
      <td style="text-align:left">36</td>
      <td style="text-align:left">108</td>
      <td style="text-align:left">108</td>
    </tr>
    <tr>
      <td style="text-align:left">accept_channel</td>
      <td style="text-align:left">36</td>
      <td style="text-align:left">108</td>
      <td style="text-align:left">108</td>
    </tr>
    <tr>
      <td style="text-align:left">funding_created</td>
      <td style="text-align:left">67</td>
      <td style="text-align:left">201</td>
      <td style="text-align:left">297</td>
    </tr>
    <tr>
      <td style="text-align:left">funding_signed</td>
      <td style="text-align:left">67</td>
      <td style="text-align:left">201</td>
      <td style="text-align:left">297</td>
    </tr>
    <tr>
      <td style="text-align:left">funding_locked*2</td>
      <td style="text-align:left">6</td>
      <td style="text-align:left">18</td>
      <td style="text-align:left">18</td>
    </tr>
    <tr>
      <td style="text-align:left">Total:</td>
      <td style="text-align:left"></td>
      <td style="text-align:left">636</td>
      <td style="text-align:left">828</td>
    </tr>
  </tbody>
</table>### LN Channel Close

#### closing\_signed

|  | LN Size \(bytes\) | Simplified Size \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 39 \(closing\_signed\) |
| channel\_id | 32 | - | inferred from destination node |
| fee\_satoshis | 8 | 2 | smaller max amount |
| signature | 64 | 64 | 96 for MuSig interactive signing |
| Total |  | 67 |  |

### Summary for Close

Both parties exchange close messages to settle immediately with the current channel balances. Using Schnorr multi-signatures requires each signer send an addition 32 bytes as part of the interactive signing protocol.

<table>
  <thead>
    <tr>
      <th style="text-align:left">Message Type</th>
      <th style="text-align:left">Size Schnorr (bytes)</th>
      <th style="text-align:left">Total data transmitted by both sender and receiver (bytes)</th>
      <th style="text-align:left"></th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="text-align:left">
        <p>Close three channels</p>
        <p>ECDSA or Schnorr</p>
      </td>
      <td style="text-align:left">Close three channels with Schnorr Multi-Signature</td>
      <td style="text-align:left"></td>
      <td style="text-align:left"></td>
    </tr>
    <tr>
      <td style="text-align:left">closing_signed</td>
      <td style="text-align:left">67</td>
      <td style="text-align:left">402</td>
      <td style="text-align:left">594</td>
    </tr>
    <tr>
      <td style="text-align:left">Total:</td>
      <td style="text-align:left"></td>
      <td style="text-align:left">402</td>
      <td style="text-align:left">594</td>
    </tr>
  </tbody>
</table>### Lot49 Network

The Lot49 proposal uses the eltoo scheme to update channel states. There is no revocation phase in this update protocol which reduces the amount of data transmitted. It also means that nodes that are part of the same _transaction chain_ can cooperatively sign a single combined transaction to update their respective channel states. In our example the _transaction chain_ includes the transactions that update the three channel states between the message sender and destination node. If an out-of-date update is committed by one of the nodes, it will not result in a penalty and can be replaced by any of the nodes by committing a more recent update within the timelock period.

#### update \(update\_add\_htlc & commitment\_signed\)

|  | Size Schnorr \(bytes\) | Size BLS \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 2 \(Negotiate\_1\) |
| channel\_id | - | - | inferred from destination node |
| id | - | - | ID of the sender is included in the normal mesh routing header |
| prepaid\_tokens \(amount\_msat\) | 1 | 1 | sat amount committed by message sender |
| payment\_hash | - | - | inferred from message data |
| cltv\_expiry | - | - | use standard value |
| relay\_path \(onion\_routing\_packet\) | 12 | 12 | up to 6 clear text hops, one 2 byte node ID per hop; also used to reconstruct the complete transaction chain |
| agg\_signature | 64 | 48 | commit to new transaction chain |
| Total | 78 | 62 |  |

All of the nodes along the relay path collaboratively sign a single transaction \(with Schnorr\) or set of transactions \(BLS\). This involves each node modifying the _update_ message to aggregate their signature with the agg\_signature field data they received before retransmitting it to the next relay node. To update all three channels requires transmitting the _update_ message three times.

A Schnorr multisignature or BLS aggregate signature is used to reduce the total amount of data transmitted between nodes. If using Schnorr signatures, each node aggregates their signature to commit to spend from a single _setup_ transaction that must be signed by all of the involved relay nodes. If using BLS signatures, each node aggregates their signature to commit to spend from a _setup_ transaction established with the next relay node along the route.

For each update, nodes do not need to revoke the previous channel state. This reduces the amount of signature information that must be transmitted. If at any point a relay becomes unresponsive, a single transaction signed with the last complete multisignature \(Schnorr\) or aggregate signature \(BLS\) can be committed to settle all of the channels involved.

| Message Type | Size Schnorr \(bytes\) | Size BLS \(bytes\) | Total data transmitted by both sender and receiver \(bytes\) |  |
| :--- | :--- | :--- | :--- | :--- |
| Update three channels |  |  |  |  |
| Schnorr | BLS |  |  |  |
| update | 78 | 62 | 234 | 189 |
| Total: |  |  | 234 | 189 |

### Lot49 Channel Setup / Close

The setup phase for a new payment channel starts with a nearby node proposing a transaction to refund the setup transaction. The receiving node that is funding the channel can hold that transaction until they need it because they have the 2nd signature.

### Lot49 Channel Setup / Close

#### Setup / close \(closing\_signed\)

|  | Size Schnorr \(bytes\) | Size BLS \(bytes\) | Note |
| :--- | :--- | :--- | :--- |
| type | 1 | 1 | 1 \(Setup\_2\) or 7 \(Close\_1\) |
| channel\_id | - | - | inferred from destination node |
| id | - | - | ID of the sender is included in the normal mesh routing header |
| prepaid\_tokens \(amount\_msat\) | 1 | 1 | msat committed by message sender |
| payment\_hash | - | - | inferred from message data |
| cltv\_expiry | - | - | use standard value |
| relay\_path \(onion\_routing\_packet\) | 12 | 12 | up to 6 clear text hops, one 2 byte node ID per hop; also used to reconstruct the complete transaction chain |
| agg\_signature | 64 | 48 | commit to new transaction chain; 96 bytes for Schnorr if MuSig signing used |
| Total | 79 | 62 |  |

### Summary for Lot49 Setup and Close

| Message Type | Size Schnorr \(bytes\) | Size BLS \(bytes\) | Total data transmitted by both sender and receiver \(bytes\) |  |  |  |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Setup / Close three channels | Data transmitted to a gateway |  |  |  |  |  |
| Schnorr | BLS | Schnorr | BLS |  |  |  |
| Setup / Close | 79 | 62 | 474 | 372 | 234 | 62 |
| Total: |  |  | 474 | 372 | 234 | 62 |

### Conclusion

For an example three hop message delivery, the eltoo revocation system would require transmitting 1/5 as much data to update channels \(234 vs 1221 bytes\) compared to the current update mechanism used by the Lightning Network. BLS signature aggregation could reduce the amount of data that must be transmitted to an internet gateway to setup and close channels by 2/3 of that required when each channel is independently setup or closed \(62 vs 234 bytes\). The reduction in transmission overhead from using eltoo and signature aggregation increases linearly for longer relay paths.

