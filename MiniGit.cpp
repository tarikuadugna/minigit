#include "MiniGit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

std::string MiniGit::computeHash(const std::string& content) {
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << std::hex << hasher(content);
    return ss.str();
}

std::string MiniGit::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void MiniGit::writeToFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

std::string MiniGit::readFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void MiniGit::saveCommit(const Commit& commit) {
    std::string commitPath = objectsDir + "/" + commit.hash;
    std::stringstream ss;
    ss << "message:" << commit.message << "\n"
       << "timestamp:" << commit.timestamp << "\n"
       << "parent:" << commit.parent << "\n"
       << "files:";
    for (size_t i = 0; i < commit.filenames.size(); ++i) {
        ss << commit.filenames[i] << ":" << commit.blobHashes[i];
        if (i < commit.filenames.size() - 1) ss << ",";
    }
    ss << "\n";
    writeToFile(commitPath, ss.str());
}

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

void MiniGit::updateHEAD(const std::string& commitHash) {
    writeToFile(headFile, currentBranch + ":" + commitHash);
    branches[currentBranch] = commitHash;
    saveBranches();
}

std::string MiniGit::getHEAD() {
    std::string content = readFromFile(headFile);
    size_t colon = content.find(':');
    return (colon != std::string::npos) ? content.substr(colon + 1) : "";
}

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

void MiniGit::saveBranches() {
    std::stringstream ss;
    for (const auto& b : branches)
        ss << b.first << ":" << b.second << "\n";
    writeToFile(refsDir + "/branches", ss.str());
}

void MiniGit::loadStagedFiles() {
    stagedFiles.clear();
    std::istringstream iss(readFromFile(indexFile));
    std::string filename;
    while (std::getline(iss, filename))
        if (!filename.empty()) stagedFiles.insert(filename);
}

void MiniGit::saveStagedFiles() {
    std::stringstream ss;
    for (const auto& file : stagedFiles) ss << file << "\n";
    writeToFile(indexFile, ss.str());
}

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
    for (const auto& file : stagedFiles) {
        std::string content = readFromFile(file);
        std::string hash = computeHash(content);
        commit.filenames.push_back(file);
        commit.blobHashes.push_back(hash);
    }
    std::string fullContent = message + commit.timestamp + commit.parent;
    for (const auto& h : commit.blobHashes) fullContent += h;
    commit.hash = computeHash(fullContent);
    saveCommit(commit);
    updateHEAD(commit.hash);
    stagedFiles.clear();
    saveStagedFiles();
    std::cout << "Committed changes with hash: " << commit.hash << "\n";
}

void MiniGit::log() {
    loadBranches();
    std::string current = getHEAD();
    while (!current.empty()) {
        Commit c = loadCommit(current);
        std::cout << "Commit: " << c.hash << "\nDate: " << c.timestamp << "\nMessage: " << c.message << "\n\n";
        current = c.parent;
    }
}

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

void MiniGit::listBranches() {
    loadBranches();
    std::cout << "Branches:\n";
    for (const auto& [name, hash] : branches)
        std::cout << (name == currentBranch ? "* " : "  ") << name << "\n";
}

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

void MiniGit::merge(const std::string& branchName) {
    std::cout << "Merge not implemented in this version.\n";
}