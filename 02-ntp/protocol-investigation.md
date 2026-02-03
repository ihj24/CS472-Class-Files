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

## Investigation 2: NTP Epoch (1900 vs 1970)

### Implementation Context
void get_current_ntp_time(ntp_timestamp_t *ntp_ts) and void ntp_time_to_string(const ntp_timestamp_t *ntp_ts, char *buffer, size_t buffer_size, int local) functions

### Investigation Journey
1. Why do we have 2 epochs?
2. Why not switch to 1900 epoch?
3. What warrants creating a new epoch?
4. How do servers protect against malicious time requests?
5. How can failure to convert between epochs effect security?

### Design Rationale
As we know, there are 2 main different epochs: Unix Epoch (1/1/1970) and NTP Epoch (1/1/1900). The reason we have 2 epochs is because when the first epoch was made, they arbitrarily used 1970 since it was close to when Unix was being made. NTP epoch has a much more interesting reasoning as David Mills wanted to cover the 20th century, the epoch was to be used for scientific and historical data, and was designed to be OS independent. Since the better epoch was made later, there were 15 years worth of electronics already set on unix epoch. It was not viable to have a massive switch because the epoch system does not have dates stored as dates, but milliseconds, which means the time conversions would break if they tried to switch. What warrants making a new epoch are new technology domains, incompatible requirements, and if it a self contained system. Time is shockingly important when it comes to security. Security protocols consult multiple independent clocks and reject outliers, and limit how fast or how far the system clock can change. Jumps in time are avoided by slewing time gradually, while security sensitive logic uses monotonic clocks that cannot go backward. If epoch conversion is off, timestamps can be interpreted as far off into time. This can cause expired credentials, tokens, or certificates to seem valid.

### Implementation Insight
I wonder if the habit of stacking systems with eventually come to backfire. It seems like having so many different systems in place can get confusing and convoluted. I was genuinely surprised how important time is to security. 
