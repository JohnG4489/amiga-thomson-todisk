# ToDisk — Thomson Disk & Tape Toolkit for AmigaOS (CLI)

## 🇫🇷 Français : Outil “chirurgical” pour manipuler/convertir/transférer des images disques et cassettes Thomson depuis AmigaOS.

### Objectif

**ToDisk** est une commande CLI dédiée à l’écosystème **Thomson 8-bit** (MO5, TO7, TO8, TO9+).  
Là où le *handler* apporte un accès transparent, **ToDisk** est l’outil de **maintenance / conversion / transfert physique** : images disques (`.FD/.SAP/.TDS`), cassettes (`.K7`), formatage et pipeline de **cross-dev Amiga ↔ Thomson**.

### Points forts

- **Multi-formats** : prise en charge complète des images `.FD`, `.SAP`, `.TDS`
- **Cassette `.K7`** : lecture des cassettes standard comme unité virtuelle (extraction)
- **Transfert physique** : écriture d’images vers disquettes réelles + dump depuis disquettes
- **Outils d’assets** : visualiseurs intégrés (MAP, niveaux *Androïdes*)
- **Cross-développement** :
  - conversion **IFF Amiga → MAP Thomson**
  - import **ANSI → Assdesass (.ASS)** avec numérotation auto (éditeur **EPI**)
- **Formatage** : création de disquettes 3.5" au format Thomson
- S’appuie sur **`todisk.device`** pour une gestion fiable des secteurs (256 octets, etc.)

### Formats supportés

| Format | Description | Particularité |
|---|---|---|
| **.FD** | Image brute (raw) | binaire direct, multi-unités / multi-faces |
| **.SAP** | Image “sécurisée” | secteurs avec en-têtes + checksums |
| **.TDS** | Image packée | n’enregistre que les secteurs utilisés |
| **.K7** | Image cassette | lecture seule (extraction) |

### Détails techniques (résumé)

- Écrit en **C** (68020+ recommandé pour les conversions)
- **`todisk.device`** : pilote spécifique (secteurs 256 octets, bug de numérotation des faces Thomson, etc.)
- **MultiDOS** : architecture modulaire (portabilité de la logique FS)
- Conversion **G2 (accents Thomson) → ANSI Amiga**
- Respect strict des types Thomson (BIN, BAS, ASS, ASC)

### Prérequis

- **CPU** : 68020+
- **OS** : AmigaOS 2.04 (V37) minimum
- **Matériel** : lecteur disquette interne/externe requis pour le transfert physique

### Installation (binaire)

1. Copier `todisk` dans `C:`
2. Copier `todisk.device` dans `DEVS:`
3. *(Optionnel)* Pour l’accès transparent via handler : installer `tofilesystem` dans `L:` et monter via vos DOSDrivers

### Utilisation

- Lister les commandes / options :
  - `todisk ?`
- Bon réflexe : garder un workflow “image → conversion → transfert physique” selon le besoin.

> Astuce : ToDisk est pensé comme une “boîte à outils” : il fait le sale boulot (formats, checks, conversions), le handler fait le confort (montage transparent).

### Limitations / Notes

- Certaines opérations nécessitent un matériel compatible (lecteur, timings, etc.)
- Les conversions “assets” dépendent des conventions de formats côté Thomson (MAP, etc.)

### Licence

Licence MIT

---

## 🇬🇧 English : “Surgical” CLI tool to manipulate/convert/transfer Thomson disk & tape images from AmigaOS.

### Purpose

**ToDisk** is a CLI tool for the **Thomson 8-bit** ecosystem (MO5, TO7, TO8, TO9+).  
While the *handler* provides transparent access, **ToDisk** is the **maintenance / conversion / physical transfer** toolkit: disk images (`.FD/.SAP/.TDS`), tape images (`.K7`), formatting, and an **Amiga ↔ Thomson cross-dev** pipeline.

### Highlights

- **Multi-format** support: full handling of `.FD`, `.SAP`, `.TDS` images
- **Tape `.K7`** support: read standard tapes as a virtual unit (extraction)
- **Physical transfer**: write images to real floppies + dump real floppies back to images
- **Asset tools**: built-in viewers (MAP images, *Androïdes* levels)
- **Cross-development**:
  - **Amiga IFF → Thomson MAP** conversion
  - **ANSI text → Assdesass (.ASS)** import with auto line numbering (EPI editor compatible)
- **Formatting**: create 3.5" Thomson-native floppies
- Relies on **`todisk.device`** for reliable sector handling (256-byte sectors, etc.)

### Supported formats

| Format | Description | Key point |
|---|---|---|
| **.FD** | Raw disk image | direct binary, multi-drive / multi-side |
| **.SAP** | “Secured” image | sector headers + integrity checksums |
| **.TDS** | Packed image | stores only used sectors |
| **.K7** | Tape image | read-only (extraction) |

### Technical overview

- Written in **C** (68020+ recommended for conversion routines)
- **`todisk.device`**: dedicated driver (256-byte sectors, Thomson side-numbering quirks, etc.)
- **MultiDOS**: modular architecture (portable FS logic)
- **G2 charset (Thomson accents) → Amiga ANSI** handling
- Strict Thomson file type handling (BIN, BAS, ASS, ASC)

### Requirements

- **CPU**: 68020+
- **OS**: AmigaOS 2.04 (V37) minimum
- **Hardware**: internal/external floppy drive required for physical operations

### Installation (binary)

1. Copy `todisk` to `C:`
2. Copy `todisk.device` to `DEVS:`
3. *(Optional)* For transparent access via handler: install `tofilesystem` into `L:` and mount using your DOSDrivers

### Usage

- List commands / options:
  - `todisk ?`
- Recommended workflow: “image → convert → physical write” depending on your target.

> Tip: Think of ToDisk as the “surgical toolkit” (formats, checks, conversions), and the handler as the comfort layer (transparent mount).

### Limitations / Notes

- Some operations require compatible hardware (drive, timings, etc.)
- Asset conversions depend on Thomson-side format conventions (MAP, etc.)

### License

Licence MIT
