# MiniGit: A Custom Version Control System

## Project Overview

MiniGit is a lightweight version control system implemented in C++ that simulates the core functionality of Git. It provides local repository management with support for commits, branching, merging, and history tracking.

## Features Implemented

### Core Features ✅
- **Repository Initialization** (`init`) - Creates `.minigit` directory structure
- **File Staging** (`add`) - Adds files to staging area with blob storage
- **Committing** (`commit`) - Creates commit objects with metadata and file snapshots
- **History Viewing** (`log`) - Traverses and displays commit history
- **Repository Status** (`status`) - Shows current branch and staged files

### Advanced Features ✅
- **Branching** (`branch`) - Create and list branches
- **Checkout** (`checkout`) - Switch between branches or specific commits
- **Merging** (`merge`) - Merge branches with conflict detection
- **Branch Listing** - Display all branches with current branch indicator

## Data Structures Used

### 1. Blob Storage
- **Purpose**: Store file content with hash-based identification
- **Implementation**: Hash-based content addressing in `.minigit/objects/`
- **DSA Concept**: Hashing for efficient content identification

### 2. Commit Objects (DAG Structure)
- **Purpose**: Store commit metadata and file references
- **Structure**:
  ```cpp
  struct Commit {
      std::string hash;      // Unique commit identifier
      std::string message;   // Commit message
      std::string timestamp; // Creation time
      std::string parent;    // Parent commit hash (forms DAG)
      std::vector<std::string> blobHashes;  // File content hashes
      std::vector<std::string> filenames;   // Associated filenames
  };
  ```
- **DSA Concept**: Directed Acyclic Graph (DAG) for commit history

### 3. Branch References
- **Purpose**: Map branch names to commit hashes
- **Implementation**: `std::map<std::string, std::string> branches`
- **DSA Concept**: Hash Map for O(1) branch lookup

### 4. Staging Area
- **Purpose**: Track files ready for next commit
- **Implementation**: `std::set<std::string> stagedFiles`
- **DSA Concept**: Set for unique file tracking



## 🧠 Core Data Structures

### 1. **Commit Objects (DAG Structure)**
```cpp
struct Commit {
    std::string hash;                    // Unique commit identifier
    std::string message;                 // Commit message
    std::string timestamp;               // Creation time
    std::string parent;                  // Parent commit hash (forms DAG)
    std::vector<std::string> blobHashes; // File content hashes
    std::vector<std::string> filenames;  // Associated filenames
};
```
- **DSA Concept**: **Directed Acyclic Graph (DAG)**
- **Purpose**: Represents commit history with parent-child relationships
- **Usage**: History traversal, branch tracking, merge operations

### 2. **Branch References (Hash Map)**
```cpp
std::map<std::string, std::string> branches;
```
- **DSA Concept**: **Hash Map / Dictionary**
- **Purpose**: Maps branch names to their latest commit hashes
- **Usage**: O(1) branch lookup, switching between branches
- **Example**: `{"master": "abc123", "feature": "def456"}`

### 3. **Staging Area (Set)**
```cpp
std::set<std::string> stagedFiles;
```
- **DSA Concept**: **Set (Unique Collection)**
- **Purpose**: Tracks files ready for next commit
- **Usage**: Prevents duplicate file staging, efficient membership testing
- **Persistence**: Stored in `.minigit/index` file

### 4. **Blob Storage (Hash-based)**
```cpp
struct Blob {
    std::string hash;        // Content-based hash
    std::string content;     // File content
    std::string filename;    // Original filename
};
```
- **DSA Concept**: **Hashing for Content-Addressable Storage**
- **Purpose**: Store file snapshots with unique identifiers
- **Storage**: Files saved as `.minigit/objects/<hash>`
- **Benefit**: Deduplication - identical files share same hash

## 🔄 How These Structures Work Together

### **DAG Traversal for History**
```cpp
// Log traversal using linked structure
while (!currentCommit.empty()) {
    Commit commit = loadCommit(currentCommit);
    displayCommit(commit);
    currentCommit = commit.parent;  // Follow parent pointer
}
```

### **Hash Map for Branch Operations**
```cpp
// O(1) branch switching
if (branches.find(branchName) != branches.end()) {
    currentBranch = branchName;
    std::string commitHash = branches[branchName];
}
```

### **Set for Staging Management**
```cpp
// Efficient file tracking
stagedFiles.insert(filename);        // Add to staging
if (stagedFiles.empty()) {          // Check if anything to commit
    std::cout << "No changes to commit\n";
}
```

## 📊 Data Structure Complexity

| Operation | Data Structure | Time Complexity |
|-----------|---------------|-----------------|
| Branch lookup | Hash Map | O(1) |
| File staging | Set | O(log n) |
| History traversal | DAG | O(h) where h = history depth |
| Blob retrieval | Hash Table | O(1) |
| Commit creation | Vector | O(n) where n = files |


## Architecture Design

### Directory Structure
```
.minigit/
├── objects/          # Blob and commit storage
├── refs/
│   └── branches      # Branch references
├── HEAD              # Current branch and commit
└── index             # Staging area
```

### Key Design Decisions

1. **File-based Persistence**: All data stored in filesystem for simplicity
2. **Hash-based Content Addressing**: Files identified by content hash
3. **Simple Hash Function**: Using std::hash for demonstration (production would use SHA-1)
4. **Commit Graph**: Parent-child relationships form DAG structure
5. **Conflict Detection**: Basic file-level conflict detection in merge

## Usage Examples

### Basic Workflow
```bash
# Initialize repository
./minigit init

# Add and commit files
echo "Hello World" > file1.txt
./minigit add file1.txt
./minigit commit -m "Initial commit"

# View history
./minigit log
./minigit status
```

### Branching Workflow
```bash
# Create and switch to new branch
./minigit branch feature
./minigit checkout feature

# Make changes and commit
echo "New feature" > feature.txt
./minigit add feature.txt
./minigit commit -m "Add new feature"

# Merge back to master
./minigit checkout master
./minigit merge feature
```

## Implementation Highlights

### 1. Commit Hash Generation
```cpp
std::string commitContent = message + commit.timestamp + commit.parent;
for (const auto& hash : commit.blobHashes) {
    commitContent += hash;
}
commit.hash = computeHash(commitContent);
```

### 2. DAG Traversal for Log
```cpp
while (!currentCommit.empty()) {
    Commit commit = loadCommit(currentCommit);
    // Display commit info
    currentCommit = commit.parent;  // Traverse to parent
}
```

### 3. Merge Conflict Detection
```cpp
// Check if file exists in both branches with different content
if (currentCommitObj.blobHashes[currentIndex] != 
    targetCommitObj.blobHashes[targetIndex]) {
    conflicts.insert(targetFile);
}
```


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

## Limitations and Future Improvements

### Current Limitations
1. **Hash Function**: Uses simple std::hash instead of cryptographic SHA-1
2. **Merge Strategy**: Basic file-level merging, no line-by-line conflict resolution
3. **Remote Operations**: No remote repository support (local only)
4. **Binary Files**: No special handling for binary files
5. **Performance**: Not optimized for large repositories

### Potential Improvements
1. **Cryptographic Hashing**: Implement SHA-1 or SHA-256
2. **Advanced Merging**: Three-way merge with line-by-line conflict resolution
3. **Remote Support**: Add push/pull functionality
4. **Diff Viewer**: Implement line-by-line diff display
5. **Compression**: Add zlib compression for blob storage
6. **Index Optimization**: Implement Git-style index file format
7. **Garbage Collection**: Remove unreachable objects

## Code Quality Features

### Modularity
- Separate methods for each Git operation
- Clear separation of concerns between storage and logic
- Consistent error handling

### Error Handling
- Repository existence checks
- File existence validation
- Graceful handling of missing commits/branches

### Memory Management
- RAII principles with automatic cleanup
- No manual memory allocation
- STL containers for automatic memory management

## Testing Strategy

The project includes basic functional testing through the Makefile:
- Repository initialization
- File staging and committing
- History display
- Automated cleanup

## Conclusion

MiniGit successfully demonstrates the core concepts underlying version control systems. It implements essential data structures (DAGs, hash maps, sets) and algorithms (graph traversal, hashing) that power real-world VCS like Git. The modular design allows for easy extension and improvement.

The project provides hands-on experience with:
- Complex data structure design and implementation
- File I/O and persistence
- Command-line interface development
- Software engineering best practices
- Understanding of distributed version control concepts

This implementation serves as an excellent educational tool for understanding how modern version control systems work under the hood.
