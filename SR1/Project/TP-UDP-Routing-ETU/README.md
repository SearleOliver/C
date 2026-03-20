# Simulation de Protocole de Routage Dynamique — UDP en C

A simplified dynamic routing protocol simulation over UDP. Routers exchange their routing tables with neighbours using datagrams. Two build targets are provided: a split emitter/receiver pair (`tpEmRec`) and a combined peer process (`tpPeer`).

*Original base code: T. Desprats — Novembre 2022, Université Paul Sabatier.*

---

## Project Structure

```
.
├── tabrout.h       # Routing table type definition & API
├── tabrout.c       # Routing table implementation
│
├── routPem.c       # Emitter-only router process
├── routPrec.c      # Receiver-only router process
├── routP.c         # Peer router (emitter + receiver via fork)
│
├── R1Cfg.txt       # Example initial config file for router 1
├── R2Cfg.txt       # Example initial config file for router 2
│   ...
└── Makefile
```

---

## Building

```bash
# Build the split emitter/receiver pair
make tpEmRec      # produces: routPem  routPRec

# Build the combined peer process
make tpPeer       # produces: routP

# Clean all binaries and object files
make clean
```

---

## Configuration Files

Each router loads its initial routing table from a text file named `R<N>Cfg.txt`, where `<N>` is the router's number. Each line in the file is one routing entry (a plain string, e.g. a destination/next-hop description):

```
# R1Cfg.txt
10.1.1.0/24 via R1
192.168.1.0/24 via R1
```

The file must be present in the working directory before launching the router process.

---

## Programs & Usage

All three programs share the same argument signature:

```bash
./<program> <IP_address> <MyNumber> <NeighborNumber>
```

| Argument | Description |
|---|---|
| `IP_address` | IP address of this router (used in its string ID) |
| `MyNumber` | Unique router number (used to compute its UDP port) |
| `NeighborNumber` | Number of the neighbour to exchange routes with |

Ports are computed as: `17900 + <RouterNumber>`

---

### `tpEmRec` — Split emitter / receiver

Run the receiver first, then the emitter (they perform a single one-way exchange):

**Terminal 1 — Receiver (router 1):**
```bash
./routPRec 10.1.1.1 1 2
```

**Terminal 2 — Emitter (router 2):**
```bash
./routPem 10.1.1.2 2 1
```

Router 2 sends its routing table to router 1. Router 1 merges any new entries into its own table and displays the result.

---

### `tpPeer` — Combined peer

Each router simultaneously acts as both emitter (to its neighbour) and receiver (from its neighbour). The two roles are handled by a parent (emitter) and a forked child (receiver):

**Terminal 1 — Router 1:**
```bash
./routP 10.1.1.1 1 2
```

**Terminal 2 — Router 2:**
```bash
./routP 10.1.1.2 2 1
```

Both routers exchange their tables in both directions in a single run.

---

## Architecture

### UDP Port Assignment

Each router listens on a fixed port derived from its number:

```
Port = 17900 + MyNumber
```

| Router | Port |
|---|---|
| R1 | 17901 |
| R2 | 17902 |
| R3 | 17903 |
| ... | ... |

All communication uses `127.0.0.1` (localhost). To deploy across real network interfaces, replace `LOCALHOST` in the source files with the appropriate IP address.

### Exchange Protocol (UDP)

The emitter first sends the number of entries as an ASCII string, then sends each routing entry as a separate datagram:

```
Emitter → Receiver:   "3"              (number of entries)
Emitter → Receiver:   "10.1.1.0/24 via R1"
Emitter → Receiver:   "192.168.1.0/24 via R1"
Emitter → Receiver:   "172.16.0.0/16 via R1"
```

The receiver discards any entry already present in its table (duplicate detection via `is_present_entry_table`).

### `routP` — Fork model

```
main()
  ├── fork()
  │     ├── [Child]  → receiver() — binds to own port, waits for datagrams
  │     └── [Parent] → emitter()  — sends table to neighbour's port
  └── wait()         — parent waits for child to finish
```

---

## Routing Table API (`tabrout.h`)

The routing table is a fixed-capacity array of string pointers:

```c
typedef struct {
    unsigned short int nb_entry;
    char *tab_entry[NB_MAX_ENTRY];  // max 10 entries
} routing_table_t;
```

| Function | Description |
|---|---|
| `init_routing_table(rt, fileConfig)` | Loads entries from a config file |
| `display_routing_table(rt, id_router)` | Prints all entries to stdout |
| `add_entry_routing_table(rt, entry)` | Appends a new entry (malloc'd copy) |
| `is_present_entry_table(rt, entry)` | Returns `true` if the entry already exists |

---

## Configuration Constants

| Constant | File | Default | Description |
|---|---|---|---|
| `NO_BASE_PORT` | all sources | `17900` | Base value for port calculation |
| `LOCALHOST` | all sources | `"127.0.0.1"` | Destination address for all datagrams |
| `NB_MAX_ENTRY` | `tabrout.h` | `10` | Maximum number of routing table entries |
| `BUF_SIZE` / `BUF_SIZE_IN` | source files | `64` | Receive buffer size in bytes |
| `BUF_SIZE_OUT` | `routPem.c` | `4` | Send buffer size (fits up to 3-digit count) |

---

## Notes

- Each routing entry is dynamically allocated with `malloc`. There is no corresponding `free` — memory is reclaimed on process exit. For longer-running routers, a `free_routing_table` function should be added.
- `init_routing_table` uses `feof`-based reading, which may read one extra blank line at end of file. The implementation accounts for this by setting `nb_entry = i - 1`.
- UDP is inherently unreliable. In this simulation, datagram loss would cause the receiver to block indefinitely on `recvfrom`. A production implementation would require timeouts (`SO_RCVTIMEO`) and retransmission logic.
- The peer version (`routP`) performs only a **one-shot** exchange. A real distance-vector protocol (RIP-style) would repeat this exchange periodically and handle route expiry.
