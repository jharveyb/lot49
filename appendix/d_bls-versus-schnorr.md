# Appendix D. BLS versus Schnorr

|  | BLS | Schnorr | ECDSA |
| :--- | :--- | :--- | :--- |
| Signature Size \(EC curve\) | 48 bytes \(BLS12-381\) | 64 bytes \(secp256k1\) | Up to 72 bytes \(secp256k1\) |
| Signature Aggregation | Non-interactive, general aggregation | 3 rounds \(MuSig\), multi-signature aggregation | No |
| Cryptographic Assumptions | CDH in a gap Diffie-Hellman group | Same as ECDSA, simpler to implement | Computational Diffie-Hellman \(CDH\) |
| Bitcoin Support | Unlikely | Proposed Softfork | Standard |

### Signature Size

BLS signatures use pairing friendly elliptic curves and currently the most popular choice is the BLS12-381 curve \[39\] which requires 48 bytes to represent a signature. Schnorr signatures proposed for Bitcoin use the same secp256k1 curve as ECDSA signatures currently used in Bitcoin. Because Schnorr signatures are not malleable, they can be represented as 64 bytes, whereas ECDSA signatures are variable sized up to 72 bytes.

### Signature Aggregation

BLS signatures can be used for general non-interactive signature aggregation. This means that signatures on different messages can be aggregated into a single signature without additional communication rounds. The MuSig multi-signature scheme proposed for Bitcoin uses Schnorr signatures to aggregate multiple signatures on the same message. MuSig also requires three rounds of communication between signers to prevent certain attacks.

A MuSig like scheme could be used to sign the trigger and closing transactions between pairs of nodes. This would require exchanging 96 bytes instead of 64 bytes per signature, but could be improved if the commitment round is eliminated using zero knowledge proofs \[18\]; see [Appendix E](https://global-mesh-labs.gitbook.io/lot49/appendix/e_transmission-overhead) for a discussion the transmission overhead using this approach. Pairs of nodes can also eliminate one exchange of commitment information.

### Cryptographic Assumptions

BLS signatures require additional cryptographic assumptions compared to Schnorr signatures. Schnorr signature implementations are also generally simpler than for ECDSA signature implementations and have similar cryptographic assumptions \[40\].

### Bitcoin Support

Because of the difference in cryptographic assumptions, it seems likely that Schnorr signatures and a multi-signature scheme like MuSig will be adopted by the Bitcoin community before BLS signatures and general message aggregation. From a purely technical perspective BLS may be superior for the Lot49 use case, but that is less relevant if BLS does not have the support of a large community of cryptocurrency users. Currently the oldest and largest cryptocurrency community uses the Bitcoin protocol so this is an important factor in our choice of signature scheme.

