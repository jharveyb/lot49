---
description: A lightweight protocol to incentivize mobile peer-to-peer communication
---

# Lot49

Richard Myers, goTenna Inc.

[rich@gotenna.com](mailto:rich@gotenna.com)

June 11, 2019

DRAFT version 0.8.5

**Abstract:** Mesh networks offer a decentralized alternative to centralized carriers and ISPs for mobile communication. To optimize successful message delivery, and improve network coverage and reliability, mesh nodes need a system to incentivize the relay behavior of peer network nodes. We propose a trust-minimized protocol for message senders to exchange incentive value with mesh nodes that relay their messages. Our approach is to create a payment channel network \(PCN\) based on the Bitcoin Lightning Network \[[12](protocol/references.md#12-poon-j-dryja-t-2015-the-bitcoin-lightning-network-url-https-lightning-network-lightning-network-paper-pdf-accessed-29-sep-2018)\] but adapted for very low-bandwidth ad hoc mobile networks and therefore also applicable to less-constrained mesh topologies. To reduce incentive protocol overhead we propose using signature aggregation, simplex payment channel updates and payment channels formed between mesh nodes within direct communication range. Nodes primarily exchange payments to incentivize the delivery of their data without internet connectivity. Only when nodes less frequently establish, checkpoint or close payment channels must they relay payment data to a mesh connected internet gateway. This proposal requires that both the Schnorr signature \[[17](protocol/references.md#17-wuille-p-bip-schnorr-mediawiki-url-https-github-com-sipa-bips-blob-bip-schnorr-bip-schnorr-mediawiki-accessed-29-sep-2018)\] and SIGHASH\_NOINPUT signature hash flag \[[43](protocol/references.md#43-decker-c-bip-0118-mediawiki-url-https-github-com-bitcoin-bips-blob-master-bip-0118-mediawiki-accessed-16-may-2019)\] protocol updates have been adopted by the Bitcoin community.

