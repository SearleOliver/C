# Programmation Réseau en C — UDP & TCP

Two C projects exploring client/server and peer-to-peer network programming using BSD sockets.

---

## Structure

```
.
├── tresor/        # Treasure hunt game over TCP
└── routage/       # Dynamic routing protocol simulation over UDP
```

Each subdirectory contains its own detailed README.

---

## Projects

### `tresor/` — Trouve le Trésor (TCP)

A networked treasure hunt game on a 10×10 grid. The client sends cell coordinates; the server replies with a proximity score (0–3). The game ends when the player finds the treasure.

Three playable modes:
- **Standalone** — local game with plain ANSI or ncurses display
- **Iterative server** — one client at a time
- **Concurrent server** — multiple simultaneous clients via `fork()`

Communication uses **TCP** with ASCII-serialised messages.

→ See [`tresor/README.md`](tresor/README.md)

---

### `routage/` — Simulation de Routage Dynamique (UDP)

A simplified distance-vector routing protocol. Routers exchange their routing tables with neighbours over **UDP** datagrams. Each router loads an initial table from a config file, then sends and/or receives updates from one neighbour.

Two modes:
- **Split** — separate emitter (`routPem`) and receiver (`routPRec`) processes
- **Peer** — single process that forks into emitter + receiver (`routP`)

→ See [`routage/README.md`](routage/README.md)

---

## Common Concepts

| Concept | `tresor/` | `routage/` |
|---|---|---|
| Transport | TCP | UDP |
| Serialisation | ASCII strings | ASCII strings |
| Multi-process | `fork()` in concurrent server | `fork()` in peer mode |
| Port scheme | Fixed (`5555`) | Dynamic (`17900 + N`) |
| Communication | Client/Server | Peer-to-Peer |

---

## Building

Each project has its own Makefile. Build from the relevant subdirectory:

```bash
cd tresor  && make
cd routage && make
```
