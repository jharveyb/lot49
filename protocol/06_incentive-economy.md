# 6. Incentive Economy

Any node can earn a reward for relaying data for others and by being at the right place at the right time. The position of a mesh node relative to other mesh nodes determines whether their channel balance is likely to increase, decrease or stay even. Being an internet gateway, or near one, is also likely to increase the value earned due to increased traffic and the ability to confirm newly created payment channels.

### 6.1 Relay Pricing

A node that originates a message must determine how much to prepay the relay nodes that deliver their data. Each relay node can set the price they expect in order to retransmit a packet of data. Gateway and witness nodes may also set prices for their services.

Relays will charge a _price_ to relay that adequately compensate them for their _cost_ to transmit a message plus some _fee_. For mobile nodes their cost derives from the opportunity cost of using battery power to transmit for other nodes instead of reserving it for their own use. Gateway and witness nodes can include a larger fee based on the demand for their services. Relay nodes that bridge subnets could also add a higher fee than those where alternative routes exist.

Nodes should also adjust their relay price to account for the PDR \(packet delivery ratio\) of the network. The lower the PDR the more packets a relay will have to transmit before one is delivered. Relays are not compensated for data that is not ultimately delivered. The price a relay node _i_ should charge for a single relay transmission is then:

$$
price_{relay_{i}} = {(cost_{relay} + fee)  \times {1 \over PDR}}
$$

Each _relay_ node advertises their transmission price plus the minimum price advertised by all nodes _n_ in the set of neighbor nodes _N_ that advertise a route to the destination node.

$$
price_{relay_{i} \to dest} = price_{relay_{i}} + \forall{n} \in N, min(price_{relay_{n} \to dest})
$$

Neighboring nodes that include too high a fee or require more intermediate nodes to reach a destination will advertise a higher relay price and not be selected as the next hop to deliver a packet.

### 6.2 Issuance

Nodes must obtain bitcoin that can be spent to incentivize other nodes to transmit their data. Bitcoin can then be redeemed by users that provide relay services to the network in excess of their use of the network.

For this analysis we will consider the primary use to be sending short messages \(eg. SMS, Whatsapp, etc\). More data intensive applications over higher bandwidth links would require more initial value to be stored per device.

We can assume each device using the protocol sends 50 messages per day, each message requires no more than 10 hops to be delivered and relay nodes expect to earn one satoshi per hop. Using these assumptions we can predict a device uses roughly 500 satoshi per day.

We need to also factor in that there is a delay between when value is earned and when transactions are settled and the bitcoin becomes re-spendable. This delay depends on how often offline nodes are expected to access the internet. If we assume users have internet connectivity at least every seven days, then each device needs a minimum of 3500 satoshi to incentivize every message they send.

A device that only spends tokens, but never relays for others, will run out of bitcoin after about a week. However, if a node relays more messages then it sends, it will end the week with a net gain in bitcoin. Whether a node gains or loses value depends on the behavior of the user \(eg. a node must be kept powered on to relay\) but also on the location of the device and the topology of the network.

### 6.3 Central Nodes

Nodes that are situated in a central location within a well connected mesh network will easily earn value for relaying but tend to not receive more messages to relay then they are capable of relaying. Central nodes are unlikely to have a backlog of messages because they must share the local radio channel with other nearby nodes. They will earn incentives steadily but also spend them regularly in equal proportion for a net even balance.

### 6.4 Bridge Nodes

Nodes located in a sparsely meshed position that bridges two more connected meshed areas are more likely to receive a backlog of messages to relay, and will relay more for others than for themselves. These nodes will be able to prioritize messages with incentive value attached and are more likely to be net earners of incentive value.

### 6.5 Gateway Nodes

Nodes that provide a gateway to the global internet are also likely to be net earners of incentive value. Nodes that often send messages via gateway nodes will likely have their incentive balances decrease over time. These nodes will likely be a bottleneck and prioritize messages with incentive tokens.

### 6.6 Edge Nodes

Nodes that are not a bridge between subnets, but are located at the edges of a mesh network will have fewer opportunities to relay for other nodes. These nodes will likely have their incentive balances decrease over time unless they become a gateway or bridge node. These nodes represent “last-mile” nodes and any relay nodes that serve them will be net earners of tokens.

### 6.7 Confirmed Channels

A relay node should expect to receive more messages and earn more value from a route that includes only confirmed payment channels. This gives nodes some incentive to proactively confirm transaction chains that include funding transactions for new payment channels. All nodes, including the message receiver, are more likely to receive incentive value from channels with confirmed funding transactions.

### 6.8 Free Relay

Nodes need not always spend or require incentive to relay data. A node may send a message without adding an incentive header and only route through nearby nodes that likewise do not advertise a relay fee. These messages will be delivered as long as a path of free relay nodes is found. Sending to a node within direct radio range also does not require incentives.

For example, when nodes only send short distances and have many nearby nodes available to relay. In these cases the incentive protocol adds overhead but may not improve message delivery. Relaying for free can also help nodes build their routing tables and become discoverable by other nodes in the network.

For incentive payments to become useful, nodes using the incentive protocol must create a network with better message delivery than would emerge with only free relay behavior. This is more likely when sending data across many hops and when bridging isolated networks. Incentives also improve local delivery for networks that suffer from the “free-rider” problem where many nodes only send data but do not relay it.

### 6.9 Resale

A robust market already exists for converting bitcoin to local fiat currency. The price nodes charge to relay data will ultimately float based on the exchange between bitcoin and fiat currencies.

### 6.10 Coverage

It is not necessary to separately incentivize nodes to provide coverage in a particular area. Incentives for message delivery will encourage nodes to operate where there is demand. It is also easy to spoof the proofs of location that any proof of coverage system would rely on.

