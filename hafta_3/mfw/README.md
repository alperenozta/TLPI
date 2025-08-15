# pktfilter — Netfilter-based TCP/UDP port dropper with counters

This repo contains:

* **Kernel module (`pktfilter.c`)**: Hooks **IPv4 PRE\_ROUTING** to inspect **incoming** packets. Drops TCP/UDP packets whose **destination port** matches the configured block port. Keeps atomic counters.
* **User tool (`user.c`)**: Talks to the module over `/dev/pktfilter` using **ioctl** to set ports, read & clear stats.

> ⚠️ PRE\_ROUTING sees **incoming** traffic on an interface. It does **not** see `localhost (127.0.0.1)` traffic. For local tests, use your real interface IP or the **network namespace** method below.

---

## Build & Install

```bash
# Kernel headers & build tools (Debian/Ubuntu)
sudo apt-get install -y build-essential linux-headers-$(uname -r)

# Build kernel module (run in repo root)
make 

# Load module
sudo insmod pktfilter.ko

# Check logs & device node
dmesg | tail
ls -l /dev/pktfilter


Unload later:

```bash
sudo rmmod pktfilter
```

---

## What it does (quick)

* Drops packets if:

  * Protocol is **TCP** and `dest_port == tcp_block_port`, or
  * Protocol is **UDP** and `dest_port == udp_block_port`.
* Updates atomic counters:

  * `total, tcp_pass, udp_pass, tcp_drop, udp_drop`
* Exposes a char device `/dev/pktfilter` for control.

---

## CLI (user tool) — All Commands

```bash
# Set TCP block port (0 disables)
sudo ./user set-tcp <port>

# Set UDP block port (0 disables)
sudo ./user set-udp <port>

# Read counters
sudo ./user stats
# prints: total=... tcp_pass=... udp_pass=... tcp_drop=... udp_drop=...

# Clear counters
sudo ./user clear
```

Examples:

```bash
sudo ./user set-tcp 8080
sudo ./user set-udp 5353
sudo ./user stats
sudo ./user set-tcp 0      # disable TCP blocking
sudo ./user set-udp 0      # disable UDP blocking
sudo ./user clear
```

---

## Local Test Without VM — Network Namespace (works on one machine)

Create “fake external” traffic so PRE\_ROUTING triggers:

```bash
# 1) Create namespace + veth pair
sudo ip netns add ns1
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth1 netns ns1

# 2) Assign IPs
sudo ip addr add 10.10.0.1/24 dev veth0
sudo ip link set veth0 up
sudo ip netns exec ns1 ip addr add 10.10.0.2/24 dev veth1
sudo ip netns exec ns1 ip link set lo up
sudo ip netns exec ns1 ip link set veth1 up

# 3) Module & rule
sudo insmod pktfilter.ko
sudo ./user clear
sudo ./user set-tcp 8080

# 4) (Optional) server
python3 -m http.server 8080 --bind 10.10.0.1 &

# 5) Generate traffic FROM namespace → TO host (PRE_ROUTING path)
sudo ip netns exec ns1 curl -m 2 http://10.10.0.1:8080/ || echo "dropped"
or
sudo ip netns exec ns1 hping3 -S -p 8080 -c 5 10.10.0.1
# 6) Check counters
sudo ./user stats
```

---

## Notes & Tips

* **IPv4 only.** IPv6 is not inspected by this module.
* **PRE\_ROUTING** means: packets must **arrive** on an interface (not loopback).
  Use a second device, VM, or **network namespace** to generate “external” traffic.
* If another firewall rule very early **drops** traffic first, you may not see counters change. (This module registers with `NF_IP_PRI_FIRST` to run early.)
* Set port to **0** to disable blocking for that protocol.
* `sudo ./user clear` before each test run gives clean numbers.

---

## Uninstall / Cleanup

```bash
# Remove module
sudo rmmod pktfilter

# Optional: tear down namespace lab
sudo ip netns del ns1 2>/dev/null || true
sudo ip link del veth0 2>/dev/null || true
```

---
