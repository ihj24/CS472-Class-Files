# Section 1
AI used: Claude AI
Prompts Used:
  1. What is a CDN (Content Delivery Network)? How does it work at a high level?
  2. Explain the recent aws crash
  3. Why does most of the internet use aws when most of the internet goes down if it goes down?
Reflection:
I researched why so much of the internet relies on AWS, and I'm honestly disappointed. There are multiple reasons why so many people use AWS. First, AWS came out in 2006, which is years before other companies. Another reason AWS is used is because its cheaper than setting it up yourself. However, this comes with a massive downside since many major companies rely on AWS. When AWS us-east-1 goes down, Netflix, Air Bnb, Slack, and many others go down with it. Due to the cost of AWS and how rarely it goes down, comapnies won't switch even though there are other viable options. However, for how frequently it has gone down in the past few years, it seems like there needs to be a change. It is also ironic that AWS runs its status page on its own servers, so when it goes down so does the status page.

# Section 2
A Content Delivery Network is a geographically distributed network of servers that cache and serve content from servers clsoer. Instead of a user in Philadelphia waiting for data to travel to a server in Virginia and back, a CDN edge server in Philadelphia handles the request locally. This dramatically reduces latency, offloads traffic from the origin server, and provides redundancy in case the origin goes down.

1. Gepgraphic Latency: A CDN places edge servers within milliseconds of the client (~5-10ms to a nearby edge vs ~40-80ms cross-country), something that a protocol can't do.
2. Origin Server Overload: CDNs reduce the amount of direct requests to the origin server, reducing back up.
3. Security: CDNs are great against DDos attacks as they absorb requests with edge servers so they don't reach the origin

Cloudfare specifically:
DDoS Protection — Cloudflare absorbs  attack traffic at its edge network across 300+ cities, preventing origin servers from being overwhelmed by volumetric attacks that can reach terabits per second.
Web Application Firewall — Cloudflare inspects every HTTP request and blocks malicious patterns like SQL injection and cross-site scripting (XSS).before they reach the origin server.
DNS Resolution — Cloudflare operates one of the world's fastest public DNS resolvers, reducing the time it takes to resolve hostnames into IP addresses before a TCP connection is even established.
SSL/TLS Termination — Cloudflare handles the HTTPS encryption handshake at the edge, decrypts the request, inspects it, and forwards it to the origin — offloading cryptographic computation from the origin server.

# Section 3:
Incident: Cloudfare Outage
Company: Cloudfare
Date: 11/18/24
Duration: 6hrs

What Broke/Was The Impact: The root cause was a ClickHouse database configuration error. Cloudflare's Bot Management feature had a configuration file that grew to more than double its expected size (from ~200 to 400+ features), which caused proxy servers to crash globally. Because Cloudflare's proxy layer sits between users and virtually every website it protects, crashing those proxies meant requests could not be routed at all.

Why Did One Company's Failure Cause Such Widespread Impact: Cloudflare handles roughly 20% of all global web traffic and sits in the critical path between users and millions of websites. When its proxy layer fails it completely severs the connection between users and any service behind Cloudflare, because DNS has already routed users to Cloudflare's IP addresses rather than the origin.

# Section 4:
| blank | Keep-Alive | CDN Caching |
|-------|------------|-------------|
| What it avoids | TCP handshake (~150ms) | Full round trip to origin (~40-80ms + processing time) |
| Scales with users? | No | Yes |
| Geographic benefit? | No | Yes |
| Reduces origin load? | No | Yes |

Would My Client Benefit from a CDN Edge Server? 
Assume Philadelphia → Virginia origin server: approximately 10-15ms round trip latency.
Philadelphia → Cloudflare edge in Philadelphia: approximately 1-2ms round trip latency.
For the /delay/2 test, the 2-second server-side delay was always better, so the latency difference would be small in absolute terms. But for a real workload with no artificial delay, each request would be ~10-13ms faster just from proximity. Multiplied across thousands of users and dozens of requests per page load, that compounds significantly.
Additionally, with a CDN, static content would be served from the edge with no origin contact at all, meaning the Keep-Alive socket to the origin would only need to carry dynamic requests, reducing load on the origin substantially.

What's the Bigger Lesson?
"Just implement the protocol correctly" is necessary but not sufficient at internet scale. Our client-ka was a real optimization — 0.308 seconds saved, ~4.7% faster. But it does nothing for:

A user 5,000 miles from the origin server
An origin server receiving 100,000 simultaneous requests
A DDoS attack sending 1 million malicious requests per second
A single misconfigured server taking down all users

The protocol layer (HTTP/1.1, Keep-Alive) and the infrastructure layer (CDNs, load balancers, DDoS protection) solve fundamentally different problems. A correct protocol implementation handles the conversation between one client and one server efficiently. Infrastructure handles the reality of millions of clients, adversarial traffic, hardware failure, and geographic distribution. Both layers are necessary — neither is sufficient alone.
