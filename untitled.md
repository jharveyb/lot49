# 1. Introduction

It is estimated that people globally send over 80 billion mobile messages a day [\[1\]](). Mobile messaging has for many years been among the most popular uses of mobile phones worldwide [\[]()[2]()[\]](). However, consumers must rely on a few large corporations to provide mobile communication services. Many centralized mobile carriers and Internet service providers \(ISPs\) are also regional or national monopolies. Because of this centralization these services can be censored and surveilled, especially across political boundaries. Physically centralized infrastructure is also prone to catastrophic failure during natural or manmade disasters and often fails at serving last-mile communities. These problems exist despite the fact that most people possess mobile phones that can communicate directly with each other and bypass centralized infrastructure.

Mobile ad-hoc networks \(MANETs\) offer a promising solution to the problems of centralized carriers and ISPs. With the advent of smartphones, people now possess the technology required to power mobile mesh networks and communicate with each other in an entirely peer-to-peer manner. In a mesh network, data is transmitted directly to the devices of nearby people who relay and route the data until it reaches its destination. But to have the same utility as centralized networks, a mesh network must provide similar coverage and reliability.

Mobile mesh networks need a decentralized way to ensure relay and gateway nodes operate when and where they are needed to optimize coverage and reliability. We propose a protocol that allows data senders to pay relay nodes involved in data delivery with exchangeable, monetizable tokens. Nodes that receive tokens can use them to incentivize delivery of their own data or sell excess tokens to others. This allows people to earn value when they add value to the network.

### 1.1 Related Work

Early efforts to organize mobile mesh nodes in a decentralized way focused on embedding secure hardware modules [\[3\]]() in devices and game theoretic solutions [\[4\]]() to encourage nodes to cooperate. These approaches were primarily focused on preventing routing misbehavior [\[5\]]() and not on creating incentives to increase the coverage and reliability of the mesh network.

Some later proposals [\[6\]]() introduced payments to incentivize relay behavior but required a trusted third party \(TTP\) to keep a ledger of payment balances. The advent of Bitcoin [\[7\]](), a permissionless decentralized ledger system, made it possible to eliminate the TTP [\[8\]](), but not to handle high transaction volumes on-chain due to scaling issues [\[9\]](). The invention of payment channels [\[10\]]()[\[11\]]() and recent payment-channel networks \(PCN\) like the Lightning Network [\[12\]]() now allows for efficient settlement of high volume micro-payments without a TTP and using untrusted relay nodes. Recent incentivized mesh projects [\[13\]]()[\[14\]]() use payment channels to pay nearby nodes for providing internet access and bandwidth. These projects incentivize a certain quality of service \(QoS\) to transfer a quantity of data rather than charging for each data packet delivered.

For example, Althea Mesh[\[13\]]() assumes fixed directional WiFi links between nodes. High-bandwidth links mean protocol overhead is negligible and nodes are assumed to have indirect access to the internet through gateways to set up and settle relatively long lived payment channels. These conditions are appropriate for fixed wireless community networks \(WCN\)[\[15\]]() designed to replace wired internet connections, but do not apply for MANETs.

Mobile mesh devices rely on battery power and omni-directional transmissions that trade-off greater range for lower bandwidth. Network topologies in an ad hoc network will not be long lived because nodes move. Mesh subnets can become isolated from internet gateways so internet connectivity may be intermittent. These constraints require overhead be ruthlessly minimized because every transmission requires power and uses radio spectrum shared with nearby nodes. Payment channels must also be usable even when internet access is not immediately available for set up and settlement.

|  | WCN | MANET |
| :--- | :--- | :--- |
| Internet | Connected | Intermittent |
| Bandwidth | High | Low to High |
| Topology | Fixed | Dynamic |
| Power | Wall | Battery |
| Transmission | Directed | Omni |
| Metered | Amount of data at QoS | Per-packet delivered |
| Replaces | Wired Internet/ISP | Mobile Carrier/ISP |

**Table 1**: Summary of differences between Wireless Community Networks \(WCN\) and Mobile Ad hoc Networks \(MANET\).

