#include "MiniGit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

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
    if (stagedFiles.empty()) std::cout << "No files staged for commit.\n";
    else {
        std::cout << "Files staged:\n";
        for (const auto& file : stagedFiles) std::cout << "  " << file << "\n";
    }
}

// Creates a new branch pointing to current HEAD
void MiniGit::branch(const std::string& name) {
    loadBranches();
    if (branches.find(name) != branches.end()) {
        std::cout << "Branch already exists.\n";
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
    for (const auto& [name, hash] : branches)
        std::cout << (name == currentBranch ? "* " : "  ") << name << "\n";
}

// Switches to another branch and restores files from the commit
void MiniGit::checkout(const std::string& target) {
    loadBranches();
    if (branches.count(target)) {
        currentBranch = target;
        std::string hash = branches[target];
        if (!hash.empty()) {
            Commit c = loadCommit(hash);
            for (size_t i = 0; i < c.filenames.size(); ++i)
                writeToFile(c.filenames[i], readFromFile(objectsDir + "/" + c.blobHashes[i]));
        }
        updateHEAD(hash);
        std::cout << "Switched to branch '" << target << "'.\n";
    } else {
        std::cout << "Unknown branch or hash.\n";
    }
}

// Placeholder for merge functionality
void MiniGit::merge(const std::string& branchName) {
    std::cout << "Merge not implemented in this version.\n";
}
