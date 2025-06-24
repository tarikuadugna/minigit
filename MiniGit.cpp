#include "MiniGit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace fs = std::filesystem;

// Computes a hash for the given file content using std::hash
std::string MiniGit::computeHash(const std::string& content) {
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << std::hex << hasher(content);
    return ss.str(); // Return hexadecimal representation of hash
}

// Returns the current system time as a formatted string
std::string MiniGit::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// Writes content to a file
void MiniGit::writeToFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

// Reads and returns file content from the given path
std::string MiniGit::readFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Saves a commit object to a file in the .minigit/objects directory
void MiniGit::saveCommit(const Commit& commit) {
    std::string commitPath = objectsDir + "/" + commit.hash;
    std::stringstream ss;
    ss << "message:" << commit.message << "\n"
       << "timestamp:" << commit.timestamp << "\n"
       << "parent:" << commit.parent << "\n"
       << "files:";
    // Add file-to-blob mappings
    for (size_t i = 0; i < commit.filenames.size(); ++i) {
        ss << commit.filenames[i] << ":" << commit.blobHashes[i];
        if (i < commit.filenames.size() - 1) ss << ",";
    }
    ss << "\n";
    writeToFile(commitPath, ss.str());
}

// Loads and reconstructs a commit from a saved commit file
MiniGit::Commit MiniGit::loadCommit(const std::string& hash) {
    Commit commit;
    commit.hash = hash;
    std::string content = readFromFile(objectsDir + "/" + hash);
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("message:") == 0) commit.message = line.substr(8);
        else if (line.find("timestamp:") == 0) commit.timestamp = line.substr(10);
        else if (line.find("parent:") == 0) commit.parent = line.substr(7);
        else if (line.find("files:") == 0) {
            std::string fileEntries = line.substr(6);
            std::istringstream fileStream(fileEntries);
            std::string fileEntry;
            while (std::getline(fileStream, fileEntry, ',')) {
                size_t colon = fileEntry.find(':');
                if (colon != std::string::npos) {
                    commit.filenames.push_back(fileEntry.substr(0, colon));
                    commit.blobHashes.push_back(fileEntry.substr(colon + 1));
                }
            }
        }
    }
    return commit;
}

// Updates HEAD pointer to a new commit hash and saves branch info
void MiniGit::updateHEAD(const std::string& commitHash) {
    writeToFile(headFile, currentBranch + ":" + commitHash);
    branches[currentBranch] = commitHash;
    saveBranches();
}

// Retrieves the current HEAD commit hash
std::string MiniGit::getHEAD() {
    std::string content = readFromFile(headFile);
    size_t colon = content.find(':');
    return (colon != std::string::npos) ? content.substr(colon + 1) : "";
}

// Loads all branches and their latest commit hashes
void MiniGit::loadBranches() {
    branches.clear();
    std::string content = readFromFile(refsDir + "/branches");
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        size_t colon = line.find(':');
        if (colon != std::string::npos)
            branches[line.substr(0, colon)] = line.substr(colon + 1);
    }
    if (branches.empty()) branches["master"] = "";
}

// Saves all branch mappings to file
void MiniGit::saveBranches() {
    std::stringstream ss;
    for (const auto& b : branches)
        ss << b.first << ":" << b.second << "\n";
    writeToFile(refsDir + "/branches", ss.str());
}

// Loads staged files from the index file
void MiniGit::loadStagedFiles() {
    stagedFiles.clear();
    std::istringstream iss(readFromFile(indexFile));
    std::string filename;
    while (std::getline(iss, filename))
        if (!filename.empty()) stagedFiles.insert(filename);
}

// Saves currently staged files to index file
void MiniGit::saveStagedFiles() {
    std::stringstream ss;
    for (const auto& file : stagedFiles) ss << file << "\n";
    writeToFile(indexFile, ss.str());
}

// Clean up merge state files after successful merge
void MiniGit::cleanupMergeState() {
    std::string mergeHeadFile = minigitDir + "/MERGE_HEAD";
    if (fs::exists(mergeHeadFile)) {
        fs::remove(mergeHeadFile);
    }
}

// Helper function to find common ancestor of two commits
std::string MiniGit::findCommonAncestor(const std::string& commit1, const std::string& commit2) {
    if (commit1.empty() || commit2.empty()) return "";
    
    // Get all ancestors of commit1
    std::set<std::string> ancestors1;
    std::string current = commit1;
    while (!current.empty()) {
        ancestors1.insert(current);
        Commit c = loadCommit(current);
        current = c.parent;
    }
    
    // Find first common ancestor in commit2's history
    current = commit2;
    while (!current.empty()) {
        if (ancestors1.count(current)) return current;
        Commit c = loadCommit(current);
        current = c.parent;
    }
    
    return ""; // No common ancestor found
}

// Helper function to get all files from a commit
std::map<std::string, std::string> MiniGit::getCommitFiles(const std::string& commitHash) {
    std::map<std::string, std::string> files;
    if (commitHash.empty()) return files;
    
    Commit commit = loadCommit(commitHash);
    for (size_t i = 0; i < commit.filenames.size(); ++i) {
        files[commit.filenames[i]] = commit.blobHashes[i];
    }
    return files;
}

// Helper function to check if two file contents are the same
bool MiniGit::filesAreSame(const std::string& hash1, const std::string& hash2) {
    if (hash1 == hash2) return true;
    if (hash1.empty() || hash2.empty()) return false;
    
    std::string content1 = readFromFile(objectsDir + "/" + hash1);
    std::string content2 = readFromFile(objectsDir + "/" + hash2);
    return content1 == content2;
}

// Initializes a new MiniGit repository with directories and base files
void MiniGit::init() {
    if (fs::exists(minigitDir)) {
        std::cout << "Repository already initialized.\n";
        return;
    }
    fs::create_directory(minigitDir);
    fs::create_directory(objectsDir);
    fs::create_directory(refsDir);
    writeToFile(headFile, "master:");
    writeToFile(refsDir + "/branches", "master:\n");
    writeToFile(indexFile, "");
    branches["master"] = "";
    std::cout << "Initialized empty MiniGit repository.\n";
}

// Stages a file by hashing its contents and adding it to index
void MiniGit::add(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cout << "File '" << filename << "' not found.\n";
        return;
    }
    std::string content = readFromFile(filename);
    std::string hash = computeHash(content);
    writeToFile(objectsDir + "/" + hash, content);
    loadStagedFiles();
    stagedFiles.insert(filename);
    saveStagedFiles();
    std::cout << "Added '" << filename << "' to staging area.\n";
}

// Commits staged changes with a message and saves it as a new commit
void MiniGit::commit(const std::string& message) {
    loadStagedFiles();
    loadBranches();
    if (stagedFiles.empty()) {
        std::cout << "No changes to commit.\n";
        return;
    }

    Commit commit;
    commit.message = message;
    commit.timestamp = getCurrentTime();
    commit.parent = getHEAD();

    // Add file contents and hashes to commit
    for (const auto& file : stagedFiles) {
        std::string content = readFromFile(file);
        std::string hash = computeHash(content);
        commit.filenames.push_back(file);
        commit.blobHashes.push_back(hash);
    }

    // Create commit hash from metadata
    std::string fullContent = message + commit.timestamp + commit.parent;
    for (const auto& h : commit.blobHashes) fullContent += h;
    commit.hash = computeHash(fullContent);

    saveCommit(commit);
    updateHEAD(commit.hash);
    stagedFiles.clear();
    saveStagedFiles();
    std::cout << "Committed changes with hash: " << commit.hash << "\n";
}

// Prints commit history from HEAD backwards through parent hashes
void MiniGit::log() {
    loadBranches();
    std::string current = getHEAD();
    if (current.empty()) {
        std::cout << "No commits yet.\n";
        return;
    }
    while (!current.empty()) {
        Commit c = loadCommit(current);
        std::cout << "Commit: " << c.hash << "\nDate: " << c.timestamp << "\nMessage: " << c.message << "\n\n";
        current = c.parent;
    }
}

// Shows current branch and files staged for the next commit
void MiniGit::status() {
    loadBranches();
    loadStagedFiles();
    std::cout << "On branch " << currentBranch << "\n";
    
    // Check if there's an ongoing merge
    if (fs::exists(minigitDir + "/MERGE_HEAD")) {
        std::cout << "You are in the middle of a merge.\n";
        std::cout << "  (fix conflicts and run commit to complete the merge)\n";
    }
    
    if (stagedFiles.empty()) {
        std::cout << "No files staged for commit.\n";
    } else {
        std::cout << "Files staged for commit:\n";
        for (const auto& file : stagedFiles) {
            std::cout << "  " << file << "\n";
        }
    }
}

// Creates a new branch pointing to current HEAD
void MiniGit::branch(const std::string& name) {
    loadBranches();
    if (branches.find(name) != branches.end()) {
        std::cout << "Branch '" << name << "' already exists.\n";
        return;
    }
    branches[name] = getHEAD();
    saveBranches();
    std::cout << "Created branch '" << name << "'.\n";
}

// Lists all branches and marks the current one with '*'
void MiniGit::listBranches() {
    loadBranches();
    std::cout << "Branches:\n";
    for (const auto& [name, hash] : branches) {
        std::cout << (name == currentBranch ? "* " : "  ") << name << "\n";
    }
}

// Switches to another branch and restores files from the commit
void MiniGit::checkout(const std::string& target) {
    loadBranches();
    loadStagedFiles();
    
    // Check for uncommitted changes
    if (!stagedFiles.empty()) {
        std::cout << "Cannot checkout: you have uncommitted changes. Please commit them first.\n";
        return;
    }
    
    if (branches.count(target)) {
        currentBranch = target;
        std::string hash = branches[target];
        
        // Remove all current files first (simple approach)
        // In a real implementation, you'd be more careful about this
        
        if (!hash.empty()) {
            Commit c = loadCommit(hash);
            for (size_t i = 0; i < c.filenames.size(); ++i) {
                writeToFile(c.filenames[i], readFromFile(objectsDir + "/" + c.blobHashes[i]));
            }
        }
        updateHEAD(hash);
        std::cout << "Switched to branch '" << target << "'.\n";
    } else {
        std::cout << "Branch '" << target << "' does not exist.\n";
    }
}

// Main merge implementation
void MiniGit::merge(const std::string& branchName) {
    loadBranches();
    loadStagedFiles();
    
    // Check if branch exists
    if (branches.find(branchName) == branches.end()) {
        std::cout << "Branch '" << branchName << "' does not exist.\n";
        return;
    }
    
    // Can't merge branch with itself
    if (branchName == currentBranch) {
        std::cout << "Cannot merge branch with itself.\n";
        return;
    }
    
    // Check for uncommitted changes
    if (!stagedFiles.empty()) {
        std::cout << "Cannot merge: you have uncommitted changes. Please commit or unstage them first.\n";
        return;
    }
    
    std::string currentHead = getHEAD();
    std::string targetHead = branches[branchName];
    
    // Handle empty branches
    if (currentHead.empty() && targetHead.empty()) {
        std::cout << "Nothing to merge - both branches are empty.\n";
        return;
    }
    
    if (currentHead.empty()) {
        // Current branch is empty, fast-forward to target
        branches[currentBranch] = targetHead;
        updateHEAD(targetHead);
        if (!targetHead.empty()) {
            Commit c = loadCommit(targetHead);
            for (size_t i = 0; i < c.filenames.size(); ++i) {
                writeToFile(c.filenames[i], readFromFile(objectsDir + "/" + c.blobHashes[i]));
            }
        }
        std::cout << "Fast-forward merge completed.\n";
        return;
    }
    
    if (targetHead.empty()) {
        std::cout << "Nothing to merge - target branch is empty.\n";
        return;
    }
    
    // Check if target is already merged (current contains target)
    std::string temp = currentHead;
    while (!temp.empty()) {
        if (temp == targetHead) {
            std::cout << "Already up to date.\n";
            return;
        }
        Commit c = loadCommit(temp);
        temp = c.parent;
    }
    
    // Check for fast-forward merge (target contains current)
    temp = targetHead;
    while (!temp.empty()) {
        if (temp == currentHead) {
            // Fast-forward merge
            branches[currentBranch] = targetHead;
            updateHEAD(targetHead);
            Commit c = loadCommit(targetHead);
            for (size_t i = 0; i < c.filenames.size(); ++i) {
                writeToFile(c.filenames[i], readFromFile(objectsDir + "/" + c.blobHashes[i]));
            }
            std::cout << "Fast-forward merge completed.\n";
            return;
        }
        Commit c = loadCommit(temp);
        temp = c.parent;
    }
    
    // Find common ancestor
    std::string ancestor = findCommonAncestor(currentHead, targetHead);
    if (ancestor.empty()) {
        std::cout << "No common ancestor found. Cannot merge unrelated histories.\n";
        return;
    }
    
    // Get file states for three-way merge
    auto ancestorFiles = getCommitFiles(ancestor);
    auto currentFiles = getCommitFiles(currentHead);
    auto targetFiles = getCommitFiles(targetHead);
    
    // Collect all files involved in the merge
    std::set<std::string> allFiles;
    for (const auto& [file, hash] : ancestorFiles) allFiles.insert(file);
    for (const auto& [file, hash] : currentFiles) allFiles.insert(file);
    for (const auto& [file, hash] : targetFiles) allFiles.insert(file);
    
    // Perform three-way merge
    bool hasConflicts = false;
    std::vector<std::string> conflictFiles;
    
    for (const std::string& filename : allFiles) {
        std::string ancestorHash = ancestorFiles.count(filename) ? ancestorFiles[filename] : "";
        std::string currentHash = currentFiles.count(filename) ? currentFiles[filename] : "";
        std::string targetHash = targetFiles.count(filename) ? targetFiles[filename] : "";
        
        // Determine merge action
        if (ancestorHash == currentHash && ancestorHash == targetHash) {
            // No changes in any branch - keep as is
            continue;
        } else if (ancestorHash == currentHash && ancestorHash != targetHash) {
            // Only target branch changed - use target version
            if (targetHash.empty()) {
                // File was deleted in target
                if (fs::exists(filename)) {
                    fs::remove(filename);
                    std::cout << "Deleted: " << filename << "\n";
                }
            } else {
                // File was modified in target
                std::string content = readFromFile(objectsDir + "/" + targetHash);
                writeToFile(filename, content);
                std::cout << "Updated: " << filename << "\n";
            }
        } else if (ancestorHash != currentHash && ancestorHash == targetHash) {
            // Only current branch changed - keep current version
            std::cout << "Kept: " << filename << " (modified in current branch)\n";
        } else if (currentHash == targetHash) {
            // Both branches made same change - keep as is
            std::cout << "Kept: " << filename << " (same changes in both branches)\n";
        } else {
            // Conflict: both branches modified differently
            hasConflicts = true;
            conflictFiles.push_back(filename);
            
            // Create conflict markers in the file
            std::string currentContent = currentHash.empty() ? "" : readFromFile(objectsDir + "/" + currentHash);
            std::string targetContent = targetHash.empty() ? "" : readFromFile(objectsDir + "/" + targetHash);
            
            std::stringstream conflictContent;
            conflictContent << "<<<<<<< HEAD (" << currentBranch << ")\n";
            conflictContent << currentContent;
            if (!currentContent.empty() && currentContent.back() != '\n') conflictContent << "\n";
            conflictContent << "=======\n";
            conflictContent << targetContent;
            if (!targetContent.empty() && targetContent.back() != '\n') conflictContent << "\n";
            conflictContent << ">>>>>>> " << branchName << "\n";
            
            writeToFile(filename, conflictContent.str());
            std::cout << "CONFLICT (content): Merge conflict in " << filename << "\n";
        }
    }
    
    if (hasConflicts) {
        std::cout << "\nAutomatic merge failed; fix conflicts and then commit the result.\n";
        std::cout << "Conflicted files:\n";
        for (const std::string& file : conflictFiles) {
            std::cout << "  " << file << "\n";
        }
        
        // Stage all files for the merge commit
        for (const std::string& filename : allFiles) {
            if (fs::exists(filename)) {
                stagedFiles.insert(filename);
            }
        }
        saveStagedFiles();
        
        // Create merge state file to track ongoing merge
        std::stringstream mergeState;
        mergeState << "merging:" << branchName << "\n";
        mergeState << "head:" << currentHead << "\n";
        mergeState << "target:" << targetHead << "\n";
        writeToFile(minigitDir + "/MERGE_HEAD", mergeState.str());
        
    } else {
        // No conflicts - create merge commit automatically
        Commit mergeCommit;
        mergeCommit.message = "Merge branch '" + branchName + "' into " + currentBranch;
        mergeCommit.timestamp = getCurrentTime();
        mergeCommit.parent = currentHead;
        
        // Add all current files to the merge commit
        for (const std::string& filename : allFiles) {
            if (fs::exists(filename)) {
                std::string content = readFromFile(filename);
                std::string hash = computeHash(content);
                writeToFile(objectsDir + "/" + hash, content);
                mergeCommit.filenames.push_back(filename);
                mergeCommit.blobHashes.push_back(hash);
            }
        }
        
        // Create commit hash
        std::string fullContent = mergeCommit.message + mergeCommit.timestamp + mergeCommit.parent + targetHead;
        for (const auto& h : mergeCommit.blobHashes) fullContent += h;
        mergeCommit.hash = computeHash(fullContent);
        
        saveCommit(mergeCommit);
        updateHEAD(mergeCommit.hash);
        cleanupMergeState();
        
        std::cout << "Merge completed successfully.\n";
        std::cout << "Created merge commit: " << mergeCommit.hash << "\n";
    }
}
