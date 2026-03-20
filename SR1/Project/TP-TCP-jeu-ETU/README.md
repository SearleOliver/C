# Trouve le Trésor — Jeu de chasse au trésor en C

A treasure hunt game on a 10×10 grid, available in three modes: standalone local game (plain text or ncurses), and networked client/server (iterative or concurrent). On each turn the player guesses a cell; the server or local engine returns a proximity score until the treasure is found.

*Original base code: T. Desprats, E. Lavinal — Nov. 2020, Université Paul Sabatier.*

---

## Project Structure

```
.
├── tresor.h                    # Header declaring recherche_tresor()
├── tresor.c                    # Proximity scoring function
│
├── test-jeu-centralise.c       # Standalone game — plain ANSI terminal
├── test-jeu-centralise-nc.c    # Standalone game — ncurses display
│
├── client.c                    # TCP client — connects to a server to play
├── serveur1.c                  # Iterative TCP server (1 client at a time)
├── serveur2.c                  # Concurrent TCP server (N clients via fork)
│
└── Makefile
```

---

## Dependencies

### ncurses

Required to build `test-jeu-centralise-nc` and `client`:

```bash
# Ubuntu / Debian
sudo apt install libncurses-dev

# CentOS / RHEL / Fedora ≤ 21
sudo yum install ncurses-devel

# Fedora ≥ 22
sudo dnf install ncurses-devel

# macOS (Homebrew)
brew install ncurses
```

---

## Building

A single `make` command builds all five executables:

```bash
make
```

To clean all binaries and object files:

```bash
make clean
```

Individual targets:

```bash
make test-jeu-centralise       # Standalone plain text
make test-jeu-centralise-nc    # Standalone ncurses
make client                    # Network client
make serveur1                  # Iterative server
make serveur2                  # Concurrent server
```

---

## Programs & Usage

### Standalone — Plain text (`test-jeu-centralise`)

No network required. The scoring logic runs locally.

```bash
./test-jeu-centralise           # Fixed treasure at (4, 5)
./test-jeu-centralise rand      # Random treasure position
```

---

### Standalone — ncurses (`test-jeu-centralise-nc`)

Same as above but with a richer, full-colour terminal display powered by ncurses.

```bash
./test-jeu-centralise-nc        # Fixed treasure at (4, 5)
./test-jeu-centralise-nc rand   # Random treasure position
```

Requires a colour-capable terminal. The game waits for a key press on the victory screen before exiting.

---

### Networked — Client + Server

#### Start the server

Both servers listen on port **5555**.

```bash
./serveur1   # Iterative: one player at a time
./serveur2   # Concurrent: multiple simultaneous players
```

#### Start the client

```bash
./client <server_ip> <port>
```

**Example (local machine):**
```bash
./client 127.0.0.1 5555
```

---

## How to Play

The grid is a **10×10** board. On each turn, enter a row (1–10) and a column (1–10). The result is colour-coded on the cell:

| Score | Colour / Symbol | Meaning |
|---|---|---|
| `0` | 🟢 Green — `T` | Treasure found! |
| `1` | 🟡 Yellow | Directly adjacent (horizontal or vertical) |
| `2` | 🔴 Red | Two steps in a straight line, or diagonally adjacent |
| `3` | 🟣 Magenta | Farther away |
| `-1` | White — `0` | Cell not yet guessed |
| `10` | — | Guess is out of bounds |

The status bar shows points earned on the last move, total accumulated points, and the number of moves taken.

---

## Scoring Logic (`recherche_tresor`)

Defined in `tresor.c`, declared in `tresor.h`:

```c
int recherche_tresor(int n, int xt, int yt, int xp, int yp);
```

| Parameter | Description |
|---|---|
| `n` | Board size |
| `xt`, `yt` | Treasure coordinates |
| `xp`, `yp` | Player's guessed coordinates |

Return values:

```
10  →  guess is outside the grid (xp or yp < 1 or > n)
 0  →  exact hit — treasure found
 1  →  horizontally or vertically adjacent to the treasure
 2  →  two steps away in a straight line, OR diagonally adjacent
 3  →  everywhere else
```

---

## Network Architecture

### Protocol

Communication uses plain TCP with ASCII-encoded messages:

| Direction | Format | Example |
|---|---|---|
| Client → Server | `"<row> <col>"` | `"4 7"` |
| Server → Client | `"<score>"` | `"2"` |

### Iterative server (`serveur1`)

Handles one client at a time. The server loops back to `accept()` only after the current client disconnects.

```
[Server] listen()
    └── accept() ──► recv/send loop until score == 0
                         └── close() → accept() next client
```

### Concurrent server (`serveur2`)

Uses `fork()` to spawn a child process per client, allowing multiple simultaneous games.

```
[Parent] listen()
    └── accept() ──► fork()
                       ├── [Parent] → accept() next client
                       └── [Child]  → recv/send loop → exit()
```

---

## Configuration

| Setting | File | Default |
|---|---|---|
| Server port | `serveur1.c`, `serveur2.c` | `5555` |
| Treasure position | `main()` in server files | `(4, 5)` |
| Board size | `#define N` in all files | `10` |

To change the treasure position on the server, edit before compiling:

```c
int treasure_x = 4, treasure_y = 5;
```

To randomise the treasure in standalone mode, pass `rand` as an argument (see usage above).

---

## Notes

- The standalone versions (`test-jeu-centralise*`) call `recherche_tresor` with `N = 10`; the network servers call it with `n = 8`. Any guess with row or column 9–10 will therefore return `10` (out of bounds) in network mode unless the server-side board size is updated to match.
- The concurrent server (`serveur2`) does not handle `SIGCHLD`, which can leave zombie child processes. Adding `signal(SIGCHLD, SIG_IGN)` in the parent before the `accept()` loop is recommended.
- The `nbclient` counter in `serveur2` is not shared safely across processes after `fork()` — it is cosmetic only and does not reflect the true number of connected clients.
