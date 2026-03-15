# QUIC Investigation

## Submission Instructions

Place this completed file in the `05-ReliableUDP` folder of your GitHub Classroom repository. The TA will look for it at:

```
05-ReliableUDP/quic-investigation.md
```

Do not move it to a subfolder or rename it.

---

## Overview and Instructions

Use an AI model of your choice (Claude, ChatGPT, Gemini, etc.) to investigate the QUIC protocol concepts described below. You were introduced to QUIC at a high level in class — this investigation asks you to go deeper on your own, then connect what you learn back to the du-proto implementation you just built.

**The goal is not to summarize QUIC.** The goal is to use AI as an investigation tool to deepen your understanding of transport protocol design, and to critically evaluate your own implementation in light of what you discover.

Provide your answers directly in this file, replacing each `_Your answer here._` placeholder with your response. Each answer should be 2–4 paragraphs. You are expected to reference specific parts of your du-proto implementation — function names, data structures, and design decisions — in your responses. Vague answers that do not connect back to the code will not receive full credit.

---

## Question 1 — QUIC Streams vs. du-proto's Single Channel

Use AI to investigate how QUIC implements streams and what problem they are designed to solve. Pay particular attention to the concept of *head-of-line blocking* and how QUIC streams address it.

Then reflect on du-proto, which supports a single send/receive channel between a client and server.

**1a.** If you wanted to transfer two files simultaneously using du-proto, what would break or become complicated?

du-proto doesn't have a concept of multiple streams or channels as there is a single send/receive path between client and server. If you tried to transfer two files simultaneously, the two transfers would interfere with each other because both would be calling dpsend and dprecv on the same socket. A fragment from file a file could arrive when the receiver is expecting a fragment from another file, causing the wrong data to be written to the wrong file without a way to tell them apart. If you tried to bypass this by using two separate connections on different ports, du-proto doesn't have a multiplexing mechanism in the PDU. There is no stream ID or channel identifier in dp_pdu as the struct only carries proto_ver, mtype, seqnum, dgram_sz, and err_num. Without a way to mark each datagram with which transfer it belongs to, the receiver can't to route incoming data to the correct file.

**1b.** Head-of-line blocking is a well-known limitation of TCP. Does du-proto suffer from the same issue, a different issue, or is the concept not applicable given du-proto's design? Explain your reasoning with reference to how `dpsend()` and `dprecv()` are structured.

du-proto suffers from head-of-line blocking in the same way TCP does. In TCP, head-of-line blocking happens because the kernel holds back data until a missing segment is retransmitted. In du-proto, it happens because dprecv calls dprecvraw which blocks on recvfrom with MSG_WAITALL, waiting forever for the next fragment. If a fragment is lost or delayed, the entire transfer stalls and nothing else can make progress on that socket.

Additionally, dpsend waits for an ACK after every fragment before sending the next one. This is visible in dpsendfragment as it calls dprecvraw and blocks until it gets a FRAGMENTACK. This means du-proto is strictly stop and wait. Not only does a lost fragment stall the transfer, but the protocol doesn't have any timeout or retry logic, so a lost fragment would stall it permanently. QUIC solves this by giving each stream its own independent flow control so a stall on one stream never affects another.

**1c.** QUIC streams are multiplexed over a single UDP connection. What would you need to add to the `dp_pdu` structure in `du-proto.h` to begin supporting multiple streams? You do not need to implement this — describe the design change and explain why it would be necessary.

Adding int stream_id would let each datagram be tagged with what logical stream it belongs to. The receiver can then route each datagram to the correct reassembly buffer instead of than writing everything into one global _dpBuffer.
This would not be fine by itselfs. The seqnum field would also need to become per stream rather than per connection, because two streams sending simultaneously would have independent sequence spaces. The dp_connection struct would need per stream state tracking for the current sequence number and reassembly buffer for each active stream. Without the stream_id field as a starting point, none of that is possible and the receiver has no way to know which stream an incoming datagram belongs to.

---

## Question 2 — QUIC Connection IDs vs. du-proto's Socket-Based Identity

Use AI to investigate what a QUIC connection ID is, why it was introduced, and what real-world problem it solves. A useful starting point: think about what happens to an active connection when a mobile device switches from WiFi to LTE.

Then reflect on du-proto, which identifies a connection implicitly through UDP socket state — the IP address and port of the client and server are the effective identity of the session.

**2a.** What would happen to an active du-proto file transfer if the client's IP address changed mid-transfer? Walk through specifically what would break at the socket and protocol level.

du-proto identifies a connection through the UDP socket's source and destination address. On the server side, dprecvraw calls recvfrom which fills dp->outSockAddr.addr with the sender's address on every call. dpsendraw sends ACKs back to the address that is stored in dp->outSockAddr. There isn't a handshake negotiated identifier. The connection is the address. If the client's IP changed mid transfer, the server would keep sending ACKs to the old address. Meanwhile the client would be calling dpsendfragment which blocks on dprecvraw waiting for a FRAGMENTACK. The transfer would hang forever because du-proto has no timeout or retry logic and recvfrom blocks forever with MSG_WAITALL. The only way to recover would be to restart the entire transfer . Another problem is that dprecvraw always overwrites dp->outSockAddr with the source address of every packet. So if the client did reconnect from a new address, the server would silently update its destination without any validation, which could be exploited by an attacker to hijack the session.

**2b.** QUIC connection IDs decouple the connection identity from the network path. Is there anything in the current du-proto design that could serve as a starting point for a connection ID concept, or would it need to be built entirely from scratch? Reference specific code or data structures in your answer.

The closest existing field is seqnum in dp_pdu. Both sides track a sequence number in dp->seqNum on the dp_connection struct, and is exchanged during the handshake in dpconnect and dplisten. However, seqnum is designed to track data ordering, not connection identity. It increments with every byte sent and wouldn't survive a network path change.
There is nothing in dp_pdu or dp_connection that functions as a stable, path independent identifier. A real connection ID would need to be built from scratch. The smallest change would be to add a conn_id field to dp_pdu, which is a random value generated during dpconnect and echoed in every subsequent PDU. The server would look up connections by conn_id rather than by socket address. This is how QUIC handles connection migration.

**2c.** Connection IDs also have a security motivation. Use AI to explore this briefly — what type of attack do connection IDs help mitigate, and does du-proto have any exposure to a similar threat?

QUIC connection IDs help mitigate off path injection attacks, specifically connection reset attacks. Without a connection ID, an attacker can observe the 4-tuple and can forge packets that appear to come from one of the endpoints. In TCP this is used to send forged RST packets that tear down a connection. In UDP based protocols the same attacker could inject forged datagrams into the stream.
du-proto has significant exposure to this threat. Because it runs over UDP with no encryption or authentication, any attacker on the network path can forge a DP_MT_CLOSE PDU with a matching source address and port, which would cause dprecvdgram to send a CLOSEACK and call dpclose, ending the transfer. An attacker could also forge FRAGMENTACK packets to trick the sender into skipping chunks, corrupting the file. QUIC connection IDs prevent this by making the connection identifier unpredictable as an attacker cannot forge a valid packet without knowing the connection ID. du-proto has no equivalent protection since all its PDU fields are either predictable or observable in plaintext.

---

## Question 3 — Protocol Design Tradeoffs

This question asks you to step back from specific features and think about design philosophy.

du-proto was intentionally kept simple. QUIC is intentionally comprehensive. Use AI to help you think through the following.

**3a.** QUIC's connection establishment includes built-in TLS 1.3 and is designed to complete in 1-RTT, or even 0-RTT for repeat connections. Your du-proto completes connection setup in a single `dpconnect()`/`dplisten()` exchange with no security layer. What has du-proto traded away to achieve this simplicity, and in what real-world deployment scenarios would those tradeoffs be unacceptable?

du-proto's dpconnect and dplisten exchange a single DP_MT_CONNECT/DP_MT_CNTACK PDU pair without authentication, encryption, or identity verification. This completes in one round trip and requires no cryptographic computation. This makes it extremely simple to implement. However it trades away three things that matter in real deployments, which are confidentiality, integrity, and authentication.
Without confidentiality, every byte exchanged, including filename and all file content sent through dpsend, is visible in plaintext to any observer on the network path. Without integrity, an attacker can modify file data in transit and neither dprecv or start_server can detect the corruption. The md5 check happens after the transfer completes. Without authentication, there isn't a way for the server to verify that the client is who it claims to be, and no way for the client to verify it connected to the right server.
These tradeoffs would be rejected in any real world deployment. Transferring files over a public network would expose that data to passive eavesdropping. A bank transferring financial data over du-proto would have no protection. Even on local networks, a someone could run tcpdump and read every file transferred. QUIC's mandatory TLS 1.3 exists because of experience with unencrypted protocols showed these risks.

**3b.** Based on your investigation, identify one additional QUIC feature — beyond streams and connection IDs — that you think would be most valuable to add to du-proto if you were to extend it. Justify your choice in terms of specific limitations you observed in du-proto while completing this assignment.

The most valuablebest feature to add would be a timeout for retransmission. Something i noticed with du-proto was the stop and wait ACK mechanism in dpsendfragment and dpsenddgram. Both functions call dprecvraw and block indefinitely. There is no timeout, retry counter, or recovery path. If an ACK is lost the entire transfer hangs forever.
QUIC handles this with a retransmission system based on measured round trip time as it tracks how long ACKs typically take and retransmits any unacknowledged packet after a timeout. Even a basic version of this would improve du-proto's reliability. The change would involve setting a socket timeout using setsockopt with SO_RCVTIMEO on dp->udp_sock, and putting the dprecvraw call in dpsendfragment and dpsenddgram with retry logic that resends the datagram. A retry counter would prevent infinite retries.

## AI Conversation Log

Briefly describe how you used AI during this investigation. This does not need to be a full transcript — a honest, reflective summary is what matters. Address the following:

- What were your initial prompts, and did they produce useful results right away or did you need to refine your approach?
- Was there a moment where the AI gave you an answer that seemed incomplete, inconsistent, or that you had to verify? Describe it.
- What was the most useful follow-up question you asked, and why did it help?

I started by asking basic questions about QUIC streams and connection IDs. They were fine for a start, but I needed to be more specific. I sent it my code and started to ask questions that specifically addressed it. This ended up giving me better answers. There was a time AI said QUIC's 0 RTT reconnection as basically free with no tradeoffs, which confused me as to why it wasn't used more. I asked for what it meant by "basicaly" and it talked about a lack of security. This did not feel like something with "basically no trade offs". The most useful follow-up question I asked was about what specific fields would need to change in dp_pdu to support connection IDs and stream multiplexing. This was helpful because it provided me with a struct definition.

## A Note on How to Use AI Effectively for This

There is a wrong way and a right way to approach this. The wrong way is to ask *"summarize QUIC streams for me"* and paste the result. The right way is to use AI as a knowledgeable conversation partner — start broad, then go narrow, and push back when something is unclear.

A prompt that will serve you well: *"I just built a stop-and-wait protocol over UDP with a simple PDU structure. Explain QUIC streams to me starting from that context."* You will get dramatically better results than a generic question, and you will actually learn something.
