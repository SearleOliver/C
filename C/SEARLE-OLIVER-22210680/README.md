# Arbre Généalogique — Genealogy Tree in C

A modular C project for building, managing, and visualizing family trees. People and their parentage links are loaded from text files, and the tree can be exported to **Graphviz DOT** format for graphical rendering.

---

## Project Structure

```
.
├── identite.h            # Header — identity (person) data structure & API
├── identite.c            # Identity functions implementation
├── genea.h               # Header — genealogy tree data structure & API
├── genea.c               # Tree functions implementation
│
├── testidentite.c        # Program: unit tests for the identity module
├── testgenea.c           # Program: load & display a full genealogy tree
├── visuarbre.c           # Program: export full tree to DOT format
├── visuarbreasc.c        # Program: export a person's ancestors to DOT format
│
├── Makefile.testidentite # Builds → testidentite
├── Makefile.testgenea    # Builds → testgenea
├── Makefile.visuarbre    # Builds → visuarbre
└── Makefile.visuarbreasc # Builds → visuarbreasc
```

---

## Building

Each program has its own Makefile. Use the `-f` flag to specify which one to use:

```bash
make -f Makefile.testidentite
make -f Makefile.testgenea
make -f Makefile.visuarbre
make -f Makefile.visuarbreasc
```

To clean build artifacts for a given target:

```bash
make -f Makefile.visuarbre clean
```

All Makefiles compile with `gcc` and the flags `-Wall -Wextra` (plus `-Wpedantic` for `visuarbre` and `visuarbreasc`).

---

## Programs & Usage

### `testidentite` — Identity module unit tests

Tests `IdentiteCreer`, `IdentiteAfficher`, `IdentiteLiref`, and `IdentiteLiberer`. Reads a sample person from a file named `personne.ind`.

```bash
./testidentite
```

---

### `testgenea` — Load and display a tree

Loads people and parentage links from two files and prints the tree to the terminal.

```bash
./testgenea <fichier-personnes> <fichier-liens-parente>
```

| Argument                | Description                                |
|-------------------------|--------------------------------------------|
| `fichier-personnes`     | File containing people's data              |
| `fichier-liens-parente` | File containing parent-child links         |

**Example:**
```bash
./testgenea personnes.ind liens.par
```

---

### `visuarbre` — Export full tree to DOT

Loads the tree and exports the entire genealogy to a Graphviz DOT file.

```bash
./visuarbre <fichier-personnes> <fichier-liens-parente> <fichier-dot>
```

| Argument                | Description                                |
|-------------------------|--------------------------------------------|
| `fichier-personnes`     | File containing people's data              |
| `fichier-liens-parente` | File containing parent-child links         |
| `fichier-dot`           | Output path for the generated DOT file     |

**Example:**
```bash
./visuarbre personnes.ind liens.par arbre.gv
dot -Tpng arbre.gv -o arbre.png
```

---

### `visuarbreasc` — Export one person's ancestors to DOT

Loads the tree and exports only the ancestors of a given person to a Graphviz DOT file.

```bash
./visuarbreasc <fichier-personnes> <fichier-liens-parente> <identifiant> <fichier-dot>
```

| Argument                | Description                                         |
|-------------------------|-----------------------------------------------------|
| `fichier-personnes`     | File containing people's data                       |
| `fichier-liens-parente` | File containing parent-child links                  |
| `identifiant`           | Numeric ID of the person whose ancestors to display |
| `fichier-dot`           | Output path for the generated DOT file              |

**Example:**
```bash
./visuarbreasc personnes.ind liens.par 3 ascendants.gv
dot -Tpng ascendants.gv -o ascendants.png
```

---

## Input File Formats

### People file

Each person occupies 5 consecutive lines:

```
<ID (integer)>
<Last name>
<First name>
<Sex: M or F>
<Date of birth: dd/mm/yyyy>
```

**Example (`personnes.ind`):**
```
1
Dupont
Jean
M
01/01/1950
2
Martin
Marie
F
15/06/1952
3
Dupont
Pierre
M
10/03/1975
```

---

### Parentage links file

Each line defines one parent-child relationship:

```
<child ID> <parent ID> <P or M>
```

- `P` — Father (*Père*)
- `M` — Mother (*Mère*)

**Example (`liens.par`):**
```
3 1 P
3 2 M
```

This means person 3 has person 1 as father and person 2 as mother.

---

## DOT Output

The generated `.gv` files use the following conventions:

- Layout direction: **bottom to top** (`rankdir = BT`) — children at the bottom, ancestors at the top
- **Men** are displayed with a blue border
- **Women** are displayed with a green border
- Edges are undirected (`dir = none`)
- Each node shows: last name, first name, and date of birth

Render with [Graphviz](https://graphviz.org/):

```bash
dot -Tpng arbre.gv -o arbre.png
# or
dot -Tsvg arbre.gv -o arbre.svg
```

---

## Data Structures

### `sIdentite` (in `identite.h`)

```c
struct sIdentite {
    int   Identifiant;              // Unique numeric ID
    char *Nom;                      // Last name (dynamically allocated)
    char *Prenom;                   // First name (dynamically allocated)
    char  Sexe;                     // 'M' or 'F'
    char  DateNaissance[LG_DATE+1]; // "dd/mm/yyyy"
};
typedef struct sIdentite *tIdentite;
```

### `sArbre` / `sFiche` (in `genea.h`)

```c
struct sFiche {
    tIdentite     Identite;   // Person's identity
    struct sFiche *pPere;     // Pointer to father's fiche (or NULL)
    struct sFiche *pMere;     // Pointer to mother's fiche (or NULL)
    struct sFiche *pSuivante; // Next fiche in the linked list
};

struct sArbre {
    struct sFiche *pPremiere; // First fiche in the list
    struct sFiche *pDerniere; // Last fiche in the list
};
typedef struct sArbre *tArbre;
```

The tree is implemented as a **singly linked list** of fiches. Parent relationships are represented as internal pointers between fiches (no separate allocation).

---

## API Reference

### Identity (`identite.h`)

| Function | Description |
|---|---|
| `IdentiteCreer(id, nom, prenom, sexe, date)` | Allocates and initializes a new identity |
| `IdentiteIdentifiant(id)` | Returns the numeric ID |
| `IdentiteNom(id)` | Returns the last name |
| `IdentitePrenom(id)` | Returns the first name |
| `IdentiteSexe(id)` | Returns the sex (`'M'` or `'F'`) |
| `IdentiteDateNaissance(id)` | Returns the date of birth string |
| `IdentiteAfficher(id)` | Prints the identity to stdout |
| `IdentiteLiberer(id)` | Frees the identity's memory |
| `IdentiteLiref(f)` | Reads and returns one identity from an open file |

### Tree (`genea.h`)

| Function | Description |
|---|---|
| `ArbreCreer()` | Creates and returns an empty tree |
| `ArbreLiberer(arbre)` | Frees all fiches, identities, and the tree itself |
| `ArbreAjouterPersonne(arbre, identite)` | Appends a person to the tree |
| `ArbreAjouterLienParente(arbre, idEnfant, idParent, type)` | Links a child to a parent (`'P'` or `'M'`) |
| `ArbreAfficher(arbre)` | Prints all people with their parents to stdout |
| `ArbreAfficherAscendants(arbre, id)` | Recursively prints ancestors of a person |
| `ArbreLirePersonnesFichier(fichier)` | Loads all people from a file into a new tree |
| `ArbreLireLienParenteFichier(arbre, fichier)` | Loads parentage links from a file |
| `ArbreEcrireGV(arbre, fichier)` | Exports the full tree to a DOT file |
| `ArbreEcrireAscendantsGV(arbre, id, fichier)` | Exports one person's ancestors to a DOT file |

---

## Memory Management

- `IdentiteCreer` dynamically allocates `Nom` and `Prenom` — always free with `IdentiteLiberer`.
- `ArbreLiberer` walks the linked list, calls `IdentiteLiberer` on each fiche's identity, frees each fiche, then frees the `sArbre` struct itself.
- Parent pointers (`pPere`, `pMere`) are **internal references** into the same list — they are not freed separately, avoiding double-free issues.

---

## Dependencies

- Standard C library (`stdio.h`, `stdlib.h`, `string.h`)
- [Graphviz](https://graphviz.org/) — to render `.gv` DOT files into images
