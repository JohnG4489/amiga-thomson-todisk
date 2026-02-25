# ToDisk – Utilitaire de manipulation et transfert Thomson pour AmigaOS

## 🇫🇷 Objectif

**ToDisk** est une commande CLI complète conçue pour manipuler, convertir et transférer des fichiers ou des images disques issus de l'écosystème **Thomson** (MO5, TO7, TO8, TO9+).

Alors que le handler offre un accès transparent, **ToDisk** est l'outil de "chirurgie" permettant de gérer les formats d'images (.FD, .SAP, .TDS), de formater des disquettes réelles et d'assurer le pipeline de **Cross-Développement** entre l'Amiga et le Thomson.

---

## ✨ Fonctionnalités principales

* **Gestion multi-formats** : Support complet des images `.FD`, `.SAP` et `.TDS`.
* **Support Cassette** : Lecture des fichiers `.K7` standard comme des unités disques virtuelles.
* **Transfert physique** : Backup d'images vers disquettes réelles et inversement via le lecteur Amiga.
* **Visualisation d'assets** : Visualiseurs intégrés pour les images **MAP** et les niveaux du jeu **Androïdes**.
* **Cross-Développement** :
* Conversion d'images **IFF Amiga** vers **MAP Thomson**.
* Import de texte **ANSI** vers sources **Assdesass (.ASS)** avec numérotation auto (compatible éditeur **EPI**).


* **Formatage** : Création de disquettes physiques au format natif Thomson (3.5").
* S'appuie sur **`todisk.device`** pour une fiabilité optimale des secteurs.

---

## 🧾 Correspondance des formats d'images

ToDisk permet de jongler entre les standards de la communauté Thomson :

| Format | Description | Caractéristique |
| --- | --- | --- |
| **.FD** | Format brut (Raw) | Image binaire directe, supporte plusieurs unités/faces. |
| **.SAP** | Format sécurisé | Secteurs avec en-têtes et checksums d'intégrité. |
| **.TDS** | Format packé | Format optimisé n'enregistrant que les secteurs utilisés. |
| **.K7** | Image Cassette | Supporté en lecture seule pour l'extraction de fichiers. |

---

## 🛠️ Aspects techniques

* Développé en **C** (68020+ recommandé pour les routines de conversion).
* **todisk.device** : Driver spécifique gérant les secteurs de 256 octets et le bug de numérotation des faces Thomson.
* **MultiDOS** : Architecture modulaire permettant le portage de la logique FS.
* Gestion du jeu de caractères **G2** (accents Thomson) vers **ANSI** Amiga.
* Respect strict des types de fichiers Thomson (BIN, BAS, ASS, ASC).

---

## ⚙️ Configuration requise / Installation

### Requis

* **CPU** : 68020 ou supérieur.
* **OS** : AmigaOS 2.04 (V37) minimum.
* **Hardware** : Lecteur de disquettes interne ou externe (pour accès physique).

### Installation

1. Copier l'exécutable `todisk` dans votre répertoire `C:`.
2. Copier le pilote `todisk.device` dans `DEVS:`.
3. (Optionnel) Pour utiliser le handler, copier `tofilesystem` dans `L:` et monter les unités via vos DOSDrivers.

---

# ToDisk – Thomson Transfer and Manipulation Tool for AmigaOS

## 🇬🇧 Purpose

**ToDisk** is a comprehensive CLI utility designed to manipulate, convert, and transfer files or disk images from the **Thomson** 8-bit ecosystem (MO5, TO7, TO8, TO9+).

While the handler provides transparent access, **ToDisk** is the "surgical" tool used to manage image formats (.FD, .SAP, .TDS), format physical floppies, and handle the **Cross-Development** pipeline between Amiga and Thomson systems.

---

## ✨ Main Features

* **Multi-format support**: Full management of `.FD`, `.SAP`, and `.TDS` images.
* **Tape Support**: Read standard `.K7` files as virtual disk units.
* **Physical Transfer**: Backup images to real floppies and vice versa.
* **Asset Visualization**: Built-in viewers for **MAP** images and **Androïdes** game levels.
* **Cross-Development**:
* **IFF Amiga** to **MAP Thomson** image conversion.
* **ANSI text** to **Assdesass (.ASS)** source import with auto-line numbering (compatible with **EPI** editor).


* **Formatting**: Create physical 3.5" floppies in native Thomson format.
* Relies on **`todisk.device`** for optimal sector reliability.

---

## 🧾 Image Format Correspondence

| Format | Description | Key Feature |
| --- | --- | --- |
| **.FD** | Raw format | Direct binary image, supports multiple units/sides. |
| **.SAP** | Secured format | Sectors with headers and integrity checksums. |
| **.TDS** | Packed format | Optimized format only recording used sectors. |
| **.K7** | Cassette Image | Supported in read-only mode for file extraction. |

---

## 🛠️ Technical details

* Written in **C** (68020+ recommended for conversion routines).
* **todisk.device**: Specific driver handling 256-byte sectors and the Thomson side-numbering bug.
* **MultiDOS**: Modular architecture allowing FS logic to be ported to other systems.
* **G2 Character Set** management (Thomson accents) to Amiga **ANSI** conversion.
* Strict adherence to Thomson file types (BIN, BAS, ASS, ASC).

---

## ⚙️ Requirements / Installation

### Requirements

* **CPU**: 68020 or higher.
* **OS**: AmigaOS 2.04 (V37) minimum.
* **Hardware**: Internal or external floppy drive (for physical access).

### Installation

1. Copy `todisk` to your `C:` directory.
2. Copy `todisk.device` to `DEVS:`.
3. (Optional) For handler usage, copy `tofilesystem` to `L:` and mount units via your DOSDrivers.
