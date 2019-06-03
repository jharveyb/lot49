# 8. Future Work

Our proposal describes a design sufficient to incentivize nodes in a mobile mesh network, but there are other areas worth exploring over time that would increase the reliability and speed of the network, as well as minimize the need for internet connectivity.

* **Incentivized Flood.** Including incentive payments during “Flood” mode message relay could extend the protocol to encourage route discovery. In this mode nodes could receive the same messages from different routes and would relay the one that arrived along the shortest path \(or that pays the most tokens\).
* **Channel Factories.** Groups of nodes that are well connected on a mesh subnet, but are not well connected to the internet, could fund channel factories [\[35\]]() to facilitate dynamically creating pairwise payment channels with each other while offgrid.
* **Local Rebalance.** A protocol to detect and rebalance payment channels among nearby nodes. This could also be done with a routing protocol metric that tries to maintain balanced payment channels with nearby nodes.
* **Block digests.** Broadcast compact block filter digests using the scheme of BIP-158 [\[37\]]() in order to keep nodes on a mesh subnet in sync with the global blockchain with minimal access to the distributed ledger. This method would reduce the need for trusted witness nodes.
* **Geographic Sharding.** Allow mesh nodes to lock payments to specific geographic areas. This would allow for constructing and broadcasting smaller block filter digests specific for different geographic regions. Because nodes know a priori their location, they can reject funding transactions that are not locked to their current geographic location.
* **BLS Signatures.** Instead of using Schnorr based multisignatures, BLS-based signature aggregation could potentially be used to settle multiple transactions with a single short signature.
* **Offline Channel Funding.** Fund new offline channels using Lightning payments from an online Lightning node using previously shared invoices with long timeouts and known preimages. [\[44\]]()

