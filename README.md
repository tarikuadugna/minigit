# MiniGit: A Custom Version Control System

## 📌 Project Overview

**MiniGit** is a lightweight version control system implemented in **C++**, simulating core functionalities of Git. It manages a local repository, enabling versioning, branching, and history tracking using basic file I/O and STL containers.

---

## ✅ Features Implemented

### 🔧 Core Features

* **Repository Initialization (`init`)** – Creates the `.minigit/` structure
* **File Staging (`add`)** – Tracks files in the staging area, stores file snapshots as blobs
* **Committing (`commit`)** – Saves a commit object with metadata and file content hashes
* **View History (`log`)** – Displays commit logs by traversing the commit DAG
* **Repository Status (`status`)** – Displays current branch and staged files

### 🌿 Advanced Features

* **Branching (`branch`)** – Create new branches or list existing ones
* **Checkout (`checkout`)** – Switch to a specific branch or commit
* **Merge (`merge`)** – Merge another branch into current (skeleton included)
* **HEAD Tracking** – Follows current commit per branch

---

## 🧠 Data Structures Used

### 1. **Blob Storage**

* **Purpose:** Store file content based on hash
* **DSA Concept:** Hashing for content-based addressing
* **Location:** `.minigit/objects/<hash>`

### 2. **Commit Objects (DAG)**

```cpp
struct Commit {
    std::string hash;
    std::string message;
    std::string timestamp;
    std::string parent;
    std::vector<std::string> blobHashes;
    std::vector<std::string> filenames;
};
```

* **DSA Concept:** Directed Acyclic Graph (DAG) structure for history

### 3. **Branch References**

* **Purpose:** Map branch names to latest commit hash
* **Implementation:** `std::map<std::string, std::string> branches`
* **DSA Concept:** Hash Map

### 4. **Staging Area**

* **Purpose:** Track files to commit
* **Implementation:** `std::set<std::string> stagedFiles`
* **DSA Concept:** Set (unique entries)

---

## 🏗️ Architecture Design

### 📁 Directory Layout

```
.minigit/
├── objects/         # Blobs and commits (content-addressed)
├── refs/
│   └── branches     # Branch-to-hash mappings
├── HEAD             # Current branch:hash pointer
└── index            # Staged files
```

### 🧩 Design Principles

* **Modular Code:** Separated logic into `MiniGit.h`, `MiniGit.cpp`, and `main.cpp`
* **Hashing for IDs:** Files and commits are stored based on hashes
* **Persistence:** File-based storage (text format for simplicity)
* **Commit Graph:** Parent hash links form a DAG
* **Conflict Detection:** Merge detects file-level conflicts

---

## 🚀 Usage Examples

### Basic Workflow

```bash
./minigit init

# Create and track a file
echo "Hello" > file1.txt
./minigit add file1.txt
./minigit commit -m "Initial commit"

./minigit log
./minigit status
```

### Branching & Checkout

```bash
./minigit branch feature
./minigit checkout feature

echo "New feature" > feature.txt
./minigit add feature.txt
./minigit commit -m "Add feature"

./minigit checkout master
./minigit merge feature
```

---

## 💡 Implementation Highlights

### 1. Commit Hashing

```cpp
std::string commitContent = message + commit.timestamp + commit.parent;
for (const auto& hash : commit.blobHashes) commitContent += hash;
commit.hash = computeHash(commitContent);
```

### 2. DAG Traversal

```cpp
while (!currentCommit.empty()) {
    Commit c = loadCommit(currentCommit);
    // Show commit details
    currentCommit = c.parent;
}
```

### 3. Conflict Detection (Future Expansion)

```cpp
if (currentHash != targetHash) {
    conflicts.insert(file);
}
```

---

## 🛠️ Build & Run

### Prerequisites

* C++17 compiler (g++, clang++)

### Build Manually

```bash
g++ main.cpp MiniGit.cpp -o minigit
```

### Run

```bash
./minigit init
./minigit add file.txt
./minigit commit -m "example"
```

---

## ⚠️ Limitations & Opportunities

### Known Limitations

* Uses `std::hash` (non-cryptographic)
* No remote push/pull
* No line-level merge conflict resolution
* No diff or compression

### Suggested Improvements

* Replace hash with SHA-1/SHA-256
* Implement line-level 3-way merges
* Add remote repository support
* Add object compression
* Garbage collection for unreachable commits

---

## ✅ Code Quality Checklist

* [x] Modular Header/Source separation
* [x] Consistent file structure
* [x] Error handling (file existence, invalid operations)
* [x] RAII principles (no manual memory management)

---

## 🎓 Educational Value

This project reinforces:

* Custom file-based persistence systems
* Graph-based history management
* Set and map STL structures for version control
* Principles behind Git internals

**MiniGit** is an ideal tool to understand how real-world version control systems like Git operate internally.
