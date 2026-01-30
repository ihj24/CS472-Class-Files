# Part 1

| Problem (from 1974 paper OR modern problem) | Solution Proposed by Paper OR Why Not Addressed | How We See This Today |
|---------------------------------------------|------------------------------------------------|----------------------|
| Networks have different ways of addressing the reciever | Gateways are sort of like checkpoints that reformat data coming from one point to another. The way it is done reliably is that data is prefixed with an internetwork header that details the formatted data. | One place we actively see gateways in effect is a 502 bad gateway error|
| Networks have different maximum sizes for data | Gateways will break up the packet with fragmentation and the network will patch the packet back together | VPNs fragment packets due to added encryption |
| Why does youtube play videos that buffer and have low quality but play ads with perfect quality | This was not mentioned because youtube and internet ads did not exist at the time | Ads have higher delivery priority, and are fully downloaded before playing unlike videos |
| Local network addresses of TCP's can have different format and size | The paper introduces ports, which is a "designator of one such message stream associated with a process". This means a standard port address format is required | 80/TCP, 443/TCP |
| Transmitions are not 100% reliable | The paper proposes a timeout and positive acknowledgement mechanism that allows TCP's to recover from packet losses. If the host TCP does not recieve a confirmation, it will continuously retransmit the message for a while. If confirmation is still not recieved, the connection will time out. | My computer will struggle to connect to my Japanese friends minecraft server and I will get a time out error |
| Reconstruction of packets after fragmentation | Each packet will have a sequence number that will be unique to its destination port message stream. This increasing number will be used so the TCP can reorder the arriving packets | Every TCP segment has a sequence number |
| Unsending or editing messages | This was probably not mentioned either due to it being complicated or was not a primary concern | Nowadays, on iMessage, Instagram, and other messaging platforms, we have a breif period to "unsend" a message, or an indefinite period to edit a message |

# Part 2
### Scenario: Message Unsending (Row 8)
The modern scenario I am investigating is message unsending. The reason I chose this is that it is a relatively recent feature and I wonder why this was not included as a possibility in Cerf-Kahns' paper.

### Key Questions
1. how do messaging apps allow people to unsend messages
2. what happens if someone unsends while a back up is happening? what takes priority?
3. why may some devices miss the event in a group chat
4. are messages sent the same way packets are
5. make a diagram showing how one message becomes multiple packets, traverses the network, and gets reassembled
6. is sending the unsend event difficult to do, or something that was only able to be done recently

### Connection to paper
  Modern day message sending on platforms heavily relies on the concept of fragmentation proposed by Cerf-Kahn in 1974. A relatively recent development of messaging was the ability to unsend messages so that neither the sender nor recipent can see them. While basis of sending messages lies in thier work, technology and techniques had to evolve past that to allow for messages to be unrelated. The way messages are unsent is that a delete request for a chosen message ID is sent to the server storing the messages. From there, a delete event is broadcasted to all participants and the message is deleted or removed when the event is recieved. The issue that prevents the Cerf-Kahn packet system is that messages are not stored in a server, and are "shot and forgot".
  
### Surprised Finding
  Something that I found surprising was that messages were continuously stored and systems were not always synced when communicating. In retrospect, this should have been expected, but I am just so used to thinking about chat rooms or messaging apps that I did not think about it. I guess messages back then were closer to email. Was having a dedicated server for messaging at that time too difficult or was it too expensive? Perhaps it was just not needed.
  
### AI Conversation Documentation
how do messaging apps allow people to unsend messages

Messaging apps can let you “unsend” messages because messages aren’t just sent once and forgotten — they’re tracked, synced, and controlled by the service.

Here’s how it works in practice.

1. Messages have unique IDs (not just text)

When you send a message, the server assigns it:

A message ID

A conversation ID

Timestamps and sender info

This lets the system later say:

“Message X should no longer exist.”

2. Unsend = a delete instruction, not time travel

When you tap Unsend / Delete for everyone:

Your app sends a delete request referencing the message ID

The server:

Marks the message as deleted

Broadcasts a delete event to all participants

Other clients:

Receive the delete event

Remove or replace the message locally

What you see:

“This message was deleted”

3. End-to-end encryption still allows this

Even with E2EE (WhatsApp, Signal, iMessage):

The server can’t read message contents

But it can route control messages

The delete event contains:

Message ID

Conversation info

Sender authentication

Clients trust the sender to delete their own message.

4. Time limits exist for a reason

Apps usually limit unsend to:

Minutes or hours (WhatsApp: ~2 days)

Before read receipts or sync guarantees

Why?

Devices may be offline

Messages may be backed up

Screenshots are forever 😅

5. Why unsend isn’t perfect

Unsend fails if:

The recipient is offline for too long

The message was already backed up

The app version doesn’t support delete events

The user copied or forwarded it

Some apps handle this by:

Showing “deleted” instead of removing content

Only guaranteeing deletion on supported clients

6. Group chats make this harder

In groups:

Delete events must fan out to many devices

Some devices may miss the event

Server reconciliation is required

This is why:

Unsending may not work equally for everyone

Old devices sometimes keep the message

7. Why SMS can’t do this

SMS:

Has no message IDs shared across devices

No central server

No sync protocol

Once sent:
➡️ It’s gone forever.

8. One-sentence takeaway

“Unsend” works because modern messaging is a synchronized system, not a one-way delivery.

It’s a coordinated delete — not a rewind.

If you want, I can:

Compare WhatsApp vs Signal vs iMessage

Explain why screenshots defeat unsend

Show how delete events work technically

Explain what “delete for me” vs “delete for everyone” means

Just let me know.

what happens if someone unsends while a back up is happening? what takes priority?

Ah, this is where things get tricky, because you now have two competing systems: the backup and the unsend request. Let’s break it down carefully.

1. How backups work

Modern messaging apps (WhatsApp, Signal, iMessage) typically back up messages periodically, either:

Locally (device storage)

Cloud backup (Google Drive, iCloud)

Backups usually copy messages as they exist at the moment the backup starts, including:

Message ID

Timestamp

Content

2. How unsend works

When you unsend a message:

Your app sends a delete event to the server and/or recipients.

The server updates the message state as “deleted.”

Recipients’ apps receive the delete event and remove the message.

3. Timing matters

If backup starts before unsend:

The original message is copied to the backup.

Later, if you unsend, the backup won’t know about it, because it already captured the message.

If backup starts after unsend:

The message is marked deleted in the database.

Backup sees no message, or sees the deleted placeholder, and doesn’t include the original content.

4. What “takes priority”?

There isn’t a global priority; it depends on timing and implementation:

Case	Result
Backup starts before unsend completes	Original message may appear in backup. Unsending affects live chat but not the backup.
Backup starts after unsend	Backup sees message as deleted → original content is not saved.
Backup happens mid-unsend (race condition)	Could result in partial copy — some apps include content, some just placeholder.

So essentially:

Backups are “point-in-time snapshots.” Unsend only affects live chat, not past snapshots already made.

5. Practical consequences

WhatsApp warns:

“Messages you delete may still appear in backups.”

Signal:

No cloud backups by default → unsend is safer.

iMessage:

If messages are in iCloud backup, deleted messages may still exist there.

💡 Rule of thumb: unsend can’t retroactively remove already backed-up messages.

6. Why this happens

Backups are designed for reliability, not real-time sync.

Unsending is real-time deletion, but backups are eventually consistent snapshots.

The systems don’t coordinate fully — deletion can’t rewrite already committed backups.

✅ One-sentence takeaway

Unsend affects live chat; backups are immutable snapshots — if a backup occurs first, the original message may still exist there.

If you want, I can make a diagram showing the race between unsend and backup, so you can visualize exactly which takes effect when.

Do you want me to do that?

why may some devices miss the event in a group chat

Ah — this is a subtle but very real issue in group messaging systems. Devices can miss “unsend” events” for several reasons. Let’s go carefully.

1. Offline or disconnected devices

If a device is offline when the unsend event is sent, it can’t receive it immediately.

Some apps queue the event on the server for delivery when the device reconnects.

If the device never reconnects before the message expires (or before a retention policy deletes it), it may never see the delete event.

Example:

Alice sends a message in a WhatsApp group.

Bob goes offline immediately.

Alice unsends the message while Bob is offline.

When Bob reconnects after a week, the delete event may have already expired from the server queue.

2. Out-of-order or delayed delivery

Messaging apps often use event streams or message queues.

Network glitches, congestion, or packet loss can cause events to arrive out of order.

If the unsend event arrives before the original message is delivered, the client may fail to reconcile properly.

Result:

The device may display the original message, because it “missed” the delete instruction.

3. Device software version differences

Older clients may not understand the unsend/delete event type.

If a participant is using an older version of WhatsApp, Signal, or iMessage, the delete event is ignored.

Effect:

The message appears normally on their device, even though others see it deleted.

4. Backup restores

If a device restores from a backup taken before the unsend, the message may reappear.

Delete events are live-sync only; they usually don’t retroactively remove backed-up messages.

5. Group event fan-out failures

In large groups, the server sends unsend events to many devices simultaneously.

If some devices experience temporary network issues, their event delivery can fail.

Some apps retry delivery; others don’t (to save resources).

6. One-line mental model

Unsend is a “best-effort delete” — it works in real-time for online, compatible devices, but offline, outdated, or restored devices may miss it.

If you want, I can make a timeline diagram showing why some devices see the delete and some don’t, including backup and offline scenarios. It makes this issue much clearer visually.

Do you want me to do that?

are messages sent the same way packets are

Yes and no — messages ultimately travel as packets, but the way messaging apps handle them is very different from just “sending packets raw.” Let’s break it down carefully.

1. The network view: everything is packets

At the lowest level:

Every message (text, image, video) is broken into packets.

Packets carry protocol headers: Ethernet → IP → TCP/UDP → (maybe TLS) → payload.

Routers and gateways don’t know it’s a “message” — they just forward packets.

Example:

[Ethernet][IP][TCP][TLS][Message Chunk 1]
[Ethernet][IP][TCP][TLS][Message Chunk 2]
…


So yes, all messages are packets in transit.

2. Messaging apps: logical messages vs packets

Messaging apps treat messages as high-level objects, not raw bytes:

Each message has:

Message ID

Sender & recipient

Timestamp

Delivery/read status

Optional media attachments

Apps use protocols over TCP/UDP to transmit messages reliably.

Messages may be split across multiple packets (segmentation) and reassembled at the recipient.

3. Protocol layers matter
Layer	How messages travel
Application	Message object (text, image, video)
Transport	TCP/UDP segments, sequence numbers
Network	IP packets with routing info
Link	Ethernet/Wi-Fi frames

So a single “message” can be:

1 packet (small text)

Dozens/hundreds of packets (large video)

4. Reliability & delivery

TCP-based messaging apps (WhatsApp, Signal) rely on TCP:

Guarantees packets arrive in order

Retransmits lost packets

Reassembles message from multiple packets

UDP-based apps (e.g., some video calls) may:

Send packets out of order

Lose packets without retransmission

Use application-layer logic to reconstruct messages

5. Summary

Packets are the low-level transport unit.

Messages are the high-level logical unit that apps construct from packets.

Messages are split into packets for transit and reassembled at the recipient.

Apps add metadata, delivery status, encryption, and message IDs that packets themselves do not have.

One-line takeaway

Messages ride on packets, but messaging apps layer on logic, IDs, and metadata to make sure the “message” arrives reliably and in order.

If you want, I can make a diagram showing how one message becomes multiple packets, traverses the network, and gets reassembled — it really makes this concept crystal clear.

Do you want me to do that?

yes please

Perfect! Let’s walk through a visual explanation of how a single message becomes multiple packets, travels through the network, and is reassembled on the recipient side. I’ll describe it in steps and a diagram you can imagine or draw.

Step 1: Message creation (app layer)

You type “Hello” in a messaging app:

Message object:
- ID: 12345
- Sender: Alice
- Recipient: Bob
- Timestamp: 15-Jan-2026 10:00
- Content: "Hello"


This is the logical message, not yet packets.

Step 2: Message segmentation (transport layer)

App hands the message to TCP (or QUIC over UDP).

TCP splits the message into segments, each with a sequence number.

TCP Segment 1: Seq 1000 -> "Hel"
TCP Segment 2: Seq 1003 -> "lo"

Step 3: Packet encapsulation (network layer)

Each TCP segment is wrapped in an IP packet, which is wrapped in a link-layer frame:

[Ethernet Header]
   [IP Header: Src=AliceIP, Dst=BobIP]
      [TCP Header: SrcPort=50000, DstPort=443, Seq=1000]
         [Payload: "Hel"]


Second segment:

[Ethernet Header]
   [IP Header: Src=AliceIP, Dst=BobIP]
      [TCP Header: SrcPort=50000, DstPort=443, Seq=1003]
         [Payload: "lo"]

Step 4: Network traversal (routers, switches, gateways)

Packets travel independently through the network.

They may arrive out of order.

Each router only looks at the IP header to forward the packet.

Packet 2 --> Router A --> Router B --> Bob
Packet 1 --> Router C --> Router B --> Bob

Step 5: Reassembly at recipient (transport layer)

Bob’s device receives TCP segments:

Checks sequence numbers

Buffers out-of-order segments

Reassembles the message:

Seq 1000 + Seq 1003 → "Hello"

Step 6: App-level message reconstruction

Finally, the messaging app:

Reads the message ID

Checks delivery/unsend flags

Displays it in the chat:

Alice: Hello

Diagram (text version)
[APP] Alice types "Hello"
       |
       v
[TRANSPORT] TCP splits message → Seg1("Hel"), Seg2("lo")
       |
       v
[NETWORK] Packets created → IP + TCP headers
       |
       v
[ROUTERS / INTERNET] Packets travel independently
       |
       v
[TRANSPORT] TCP reassembles segments in order
       |
       v
[APP] Bob's device shows message "Hello"

Key points

Messages ≠ Packets — messages are logical, packets are physical transport.

Sequence numbers ensure ordering.

Headers (TCP, IP) are added by the OS/network stack.

Apps handle metadata (IDs, timestamps, unsend flags) separate from packets.

If you want, I can also make a more visual “ASCII diagram showing packets splitting and arriving out of order” — that really makes the flow crystal clear.

Do you want me to do that next?

is sending the unsend event difficult to do, or something that was only able to be done recently

Sending the unsend event is actually not inherently difficult from a technical standpoint, and it could have been done for a long time — it’s more about system design and app architecture than network capability. Let me explain carefully.

1. What an “unsend event” really is

When you tap Unsend / Delete for everyone, the app doesn’t reach back in time. It just:

Identifies the message ID of the message you want to delete.

Sends a special delete/unsend event to the server.

The server propagates the event to other devices in the conversation.

Clients receiving the event remove or replace the message.

Technically, it’s just another message in the system, with special instructions.

2. Why it wasn’t common historically

Several reasons:

A) Messaging systems were simpler

Early SMS, MSN Messenger, AIM, ICQ:

No persistent server storing all messages.

Messages were fire-and-forget — once delivered, the sender had no control.

Without a central authority to coordinate deletes, unsend wasn’t possible.

B) Clients weren’t synchronized

Before smartphones and modern cloud sync, devices weren’t always online simultaneously.

You couldn’t guarantee that every client would receive a delete event.

C) Encryption challenges

Apps that didn’t use end-to-end encryption had no secure way to authenticate unsend events.

Modern E2EE messaging needed careful design to let clients trust delete events without revealing message content to the server.

3. What changed to make unsend common

Modern messaging apps combine several capabilities:

Persistent servers

Keep a master copy of messages until delivered.

Reliable delivery protocols

TCP, QUIC, or their own retry mechanisms.

Real-time syncing

Devices almost always online and synchronized.

Message IDs & metadata

Every message is uniquely identifiable.

Authentication & trust

Delete events are signed or verified so clients accept them.

With these in place, an unsend event is technically trivial to implement.

4. Why it still isn’t perfect

Even today:

Offline devices can miss delete events.

Backups may retain original messages.

Older clients may ignore delete instructions.

So it’s easy to implement, but hard to make perfectly reliable in all situations.

✅ Summary

Sending an unsend event is not technically difficult — it’s a simple, authenticated message telling clients to delete a message. The reason it wasn’t done historically is that messaging systems weren’t centralized or synchronized enough, and encryption/verification requirements made it harder to trust such events.

If you want, I can make a timeline showing when unsend became feasible and why modern architectures allow it — it makes the evolution very clear.

Do you want me to do that?

# Part 3

  The first thing I was surprised about was how easy the paper was to follow. I am used to academic papers not being in layman's terms, but this was and whatever wasn't was thoroughly explained. I think that the most interesting solution was the gateway notion. The gate way notion is a great and very much needed checkpoint system that I'm surprised this was not the intitial idea when creating ARPANET. Since this paper made me think more about how message unsending was possible, the AI investigation was very useful for learning because the paper did not supply any hints. While this paper made me realize that the internet is a lot more complicated than I expected, it also taught me that it is easier to understant than I expected.
