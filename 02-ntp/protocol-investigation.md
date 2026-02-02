# NTP Protocol Design Investigation

## Investigation 1: Packed Bit Fields (li_vn_mode)

### Implementation Context
SET_NTP_LI_VN_MODE(packet, NTP_LI_UNSYNC, NTP_VERSION, NTP_MODE_CLIENT);

### Investigation Journey  
1. Definition of li_vn_mode
2. Understanding macro math
3. Why we shove into one packet
4. Why we can't make new protocol
5. How much it would cost to make a new protocol and switch

### Design Rationale
The way the packet works is the first 2 bits in the 1 byte packet are for leap indicator, which in my use was 3, denoting that the clients clock ins't syncronized. The next 3 bits are for the version. The final 3 are for the mode, which is 3 for request mode. Despite how vast the banwidth is, we are locked into using 1 packet due to the protocol. Back when banwidth was expensive, they decided to use one packet. Theorhetically, we could switch the protocol, but that is a lot more trouble that it is worth to allow for more packets due to a lack of compatibility. Out of curiosity, I wanted an estimate on how much it would cost to switch to a new protocol that has more packets. Chat GPT said roughly $100 billion due to software and firmware updates, and testing.

### Implementation Insight
I did not have a massive "a-ha" moment. But the closest was realizing that banwidth used to be expensive, so using 1 packet for the details was very clever.

---

## Investigation 2: [Your Second Topic]

### Implementation Context
[What you coded and what puzzled you]

### Investigation Journey
[Your research/exploration process]

### Design Rationale
[Your synthesis in your own words]

### Implementation Insight
[What you learned from implementing]
