#include "MiniGit.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

// ANSI color codes for better CLI output
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
}

// Utility function to split string into lines
std::vector<std::string> MiniGit::splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    return lines;
}

// Compute Longest Common Subsequence for diff algorithm
std::vector<std::vector<int>> MiniGit::computeLCS(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    int m = a.size();
    int n = b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (a[i-1] == b[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
            } else {
                dp[i][j] = std::max(dp[i-1][j], dp[i][j-1]);
            }
        }
    }
    return dp;
}

// Generate unified diff between two text contents
void MiniGit::showUnifiedDiff(const std::string& filename, const std::string& oldContent, const std::string& newContent) {
    std::vector<std::string> oldLines = splitLines(oldContent);
    std::vector<std::string> newLines = splitLines(newContent);
    
    if (oldLines == newLines) {
        return; // No differences
    }
    
    std::cout << Colors::BOLD << "diff --git a/" << filename << " b/" << filename << Colors::RESET << "\n";
    std::cout << Colors::BOLD << "--- a/" << filename << Colors::RESET << "\n";
    std::cout << Colors::BOLD << "+++ b/" << filename << Colors::RESET << "\n";
    
    auto lcs = computeLCS(oldLines, newLines);
    
    // Simple diff implementation - could be improved with context lines
    int i = 0, j = 0;
    int oldSize = oldLines.size();
    int newSize = newLines.size();
    
    while (i < oldSize || j < newSize) {
        if (i < oldSize && j < newSize && oldLines[i] == newLines[j]) {
            // Lines are the same
            i++;
            j++;
        } else if (i < oldSize && (j >= newSize || lcs[i][j+1] >= lcs[i+1][j])) {
            // Line deleted
            std::cout << Colors::RED << "-" << oldLines[i] << Colors::RESET << "\n";
            i++;
        } else {
            // Line added
            std::cout << Colors::GREEN << "+" << newLines[j] << Colors::RESET << "\n";
            j++;
        }
    }
    std::cout << "\n";
}

// Show diff between working directory and staged files
void MiniGit::showWorkingDiff() {
    loadStagedFiles();
    bool hasDiffs = false;
    
    // Check staged files for differences with working directory
    for (const auto& filename : stagedFiles) {
        if (!fs::exists(filename)) {
            std::cout << Colors::BOLD << "File deleted: " << filename << Colors::RESET << "\n\n";
            hasDiffs = true;
            continue;
        }
        
        std::string currentContent = readFromFile(filename);
        std::string stagedHash = computeHash(currentContent);
        
        // Find the staged version
        std::string stagedContent = readFromFile(objectsDir + "/" + stagedHash);
        
        if (currentContent != stagedContent) {
            showUnifiedDiff(filename, stagedContent, currentContent);
            hasDiffs = true;
        }
    }
    
    if (!hasDiffs) {
        std::cout << Colors::DIM << "No differences between staged files and working directory." << Colors::RESET << "\n";
    }
}

// Show diff between staged files and last commit
void MiniGit::showStagedDiff() {
    loadStagedFiles();
    std::string headHash = getHEAD();
    
    if (headHash.empty()) {
        std::cout << Colors::YELLOW << "No commits yet. Showing staged files:" << Colors::RESET << "\n";
        for (const auto& filename : stagedFiles) {
            std::string content = readFromFile(filename);
            std::cout << Colors::BOLD << "new file: " << filename << Colors::RESET << "\n";
            std::vector<std::string> lines = splitLines(content);
            for (const auto& line : lines) {
                std::cout << Colors::GREEN << "+" << line << Colors::RESET << "\n";
            }
            std::cout << "\n";
        }
        return;
    }
    
    Commit lastCommit = loadCommit(headHash);
    std::map<std::string, std::string> committedFiles;
    
    for (size_t i = 0; i < lastCommit.filenames.size(); ++i) {
        committedFiles[lastCommit.filenames[i]] = lastCommit.blobHashes[i];
    }
    
    bool hasDiffs = false;
    
    // Check each staged file
    for (const auto& filename : stagedFiles) {
        std::string currentContent = readFromFile(filename);
        std::string oldContent = "";
        
        if (committedFiles.count(filename)) {
            oldContent = readFromFile(objectsDir + "/" + committedFiles[filename]);
        }
        
        if (currentContent != oldContent) {
            if (oldContent.empty()) {
                std::cout << Colors::BOLD << "new file: " << filename << Colors::RESET << "\n";
            }
            showUnifiedDiff(filename, oldContent, currentContent);
            hasDiffs = true;
        }
    }
    
    if (!hasDiffs) {
        std::cout << Colors::DIM << "No differences between staged files and last commit." << Colors::RESET << "\n";
    }
}

// Show diff between two commits
void MiniGit::showCommitDiff(const std::string& commit1, const std::string& commit2) {
    if (commit1.empty() || commit2.empty()) {
        std::cout << Colors::RED << "Error: Invalid commit hashes provided." << Colors::RESET << "\n";
        return;
    }
    
    Commit c1 = loadCommit(commit1);
    Commit c2 = loadCommit(commit2);
    
    std::map<std::string, std::string> files1, files2;
    
    for (size_t i = 0; i < c1.filenames.size(); ++i) {
        files1[c1.filenames[i]] = c1.blobHashes[i];
    }
    
    for (size_t i = 0; i < c2.filenames.size(); ++i) {
        files2[c2.filenames[i]] = c2.blobHashes[i];
    }
    
    // Get all files from both commits
    std::set<std::string> allFiles;
    for (const auto& [name, hash] : files1) allFiles.insert(name);
    for (const auto& [name, hash] : files2) allFiles.insert(name);
    
    bool hasDiffs = false;
    
    for (const std::string& filename : allFiles) {
        std::string content1 = files1.count(filename) ? readFromFile(objectsDir + "/" + files1[filename]) : "";
        std::string content2 = files2.count(filename) ? readFromFile(objectsDir + "/" + files2[filename]) : "";
        
        if (content1 != content2) {
            if (content1.empty()) {
                std::cout << Colors::BOLD << "new file: " << filename << Colors::RESET << "\n";
            } else if (content2.empty()) {
                std::cout << Colors::BOLD << "deleted file: " << filename << Colors::RESET << "\n";
            }
            showUnifiedDiff(filename, content1, content2);
            hasDiffs = true;
        }
    }
    
    if (!hasDiffs) {
        std::cout << Colors::DIM << "No differences between the commits." << Colors::RESET << "\n";
    }
}

// Enhanced diff command with multiple options
void MiniGit::diff(const std::string& option1, const std::string& option2) {
    if (option1.empty()) {
        // Show working directory vs staged
        std::cout << Colors::CYAN << "=== Working Directory vs Staged ===" << Colors::RESET << "\n";
        showWorkingDiff();
    } else if (option1 == "--staged" || option1 == "--cached") {
        // Show staged vs last commit
        std::cout << Colors::CYAN << "=== Staged vs Last Commit ===" << Colors::RESET << "\n";
        showStagedDiff();
    } else if (!option2.empty()) {
        // Show diff between two commits
        std::cout << Colors::CYAN << "=== Commit " << option1.substr(0, 8) << " vs " << option2.substr(0, 8) << " ===" << Colors::RESET << "\n";
        showCommitDiff(option1, option2);
    } else {
        // Show working directory vs specific commit
        std::cout << Colors::CYAN << "=== Working Directory vs Commit " << option1.substr(0, 8) << " ===" << Colors::RESET << "\n";
        loadStagedFiles();
        std::map<std::string, std::string> commitFiles = getCommitFiles(option1);
        
        // Get all files in working directory and commit
        std::set<std::string> allFiles;
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().filename() != ".minigit") {
                allFiles.insert(entry.path().filename().string());
            }
        }
        for (const auto& [name, hash] : commitFiles) {
            allFiles.insert(name);
        }
        
        bool hasDiffs = false;
        for (const std::string& filename : allFiles) {
            if (filename.find(".minigit") != std::string::npos) continue;
            
            std::string workingContent = fs::exists(filename) ? readFromFile(filename) : "";
            std::string commitContent = commitFiles.count(filename) ? readFromFile(objectsDir + "/" + commitFiles[filename]) : "";
            
            if (workingContent != commitContent) {
                if (commitContent.empty()) {
                    std::cout << Colors::BOLD << "new file: " << filename << Colors::RESET << "\n";
                } else if (workingContent.empty()) {
                    std::cout << Colors::BOLD << "deleted file: " << filename << Colors::RESET << "\n";
                }
                showUnifiedDiff(filename, commitContent, workingContent);
                hasDiffs = true;
            }
        }
        
        if (!hasDiffs) {
            std::cout << Colors::DIM << "No differences found." << Colors::RESET << "\n";
        }
    }
}

// Enhanced status with color coding and more information
void MiniGit::status() {
    loadBranches();
    loadStagedFiles();
    
    std::cout << Colors::BOLD << Colors::CYAN << "On branch " << currentBranch << Colors::RESET << "\n";
    
    // Check if there's an ongoing merge
    if (fs::exists(minigitDir + "/MERGE_HEAD")) {
        std::cout << Colors::YELLOW << "⚠️  You are in the middle of a merge." << Colors::RESET << "\n";
        std::cout << Colors::DIM << "   (fix conflicts and run 'commit' to complete the merge)" << Colors::RESET << "\n\n";
    }
    
    // Check for unstaged changes
    std::set<std::string> modifiedFiles;
    std::set<std::string> untrackedFiles;
    
    // Get all files in working directory
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find(".minigit") != std::string::npos) continue;
            
            if (stagedFiles.count(filename)) {
                // Check if file was modified after staging
                std::string currentContent = readFromFile(filename);
                std::string currentHash = computeHash(currentContent);
                std::string stagedContent = readFromFile(objectsDir + "/" + currentHash);
                
                if (currentContent != stagedContent) {
                    modifiedFiles.insert(filename);
                }
            } else {
                // Check if file exists in last commit
                std::string headHash = getHEAD();
                bool isTracked = false;
                
                if (!headHash.empty()) {
                    Commit lastCommit = loadCommit(headHash);
                    for (const auto& trackedFile : lastCommit.filenames) {
                        if (trackedFile == filename) {
                            isTracked = true;
                            // Check if it's modified
                            std::string currentContent = readFromFile(filename);
                            auto it = std::find(lastCommit.filenames.begin(), lastCommit.filenames.end(), filename);
                            if (it != lastCommit.filenames.end()) {
                                size_t index = std::distance(lastCommit.filenames.begin(), it);
                                std::string committedContent = readFromFile(objectsDir + "/" + lastCommit.blobHashes[index]);
                                if (currentContent != committedContent) {
                                    modifiedFiles.insert(filename);
                                }
                            }
                            break;
                        }
                    }
                }
                
                if (!isTracked) {
                    untrackedFiles.insert(filename);
                }
            }
        }
    }
    
    // Show staged files
    if (!stagedFiles.empty()) {
        std::cout << Colors::GREEN << "Changes to be committed:" << Colors::RESET << "\n";
        for (const auto& file : stagedFiles) {
            std::cout << Colors::GREEN << "  modified:   " << file << Colors::RESET << "\n";
        }
        std::cout << "\n";
    }
    
    // Show modified files
    if (!modifiedFiles.empty()) {
        std::cout << Colors::RED << "Changes not staged for commit:" << Colors::RESET << "\n";
        std::cout << Colors::DIM << "  (use 'add <file>' to stage changes)" << Colors::RESET << "\n";
        for (const auto& file : modifiedFiles) {
            std::cout << Colors::RED << "  modified:   " << file << Colors::RESET << "\n";
        }
        std::cout << "\n";
    }
    
    // Show untracked files
    if (!untrackedFiles.empty()) {
        std::cout << Colors::RED << "Untracked files:" << Colors::RESET << "\n";
        std::cout << Colors::DIM << "  (use 'add <file>' to include in what will be committed)" << Colors::RESET << "\n";
        for (const auto& file : untrackedFiles) {
            std::cout << Colors::RED << "  " << file << Colors::RESET << "\n";
        }
        std::cout << "\n";
    }
    
    if (stagedFiles.empty() && modifiedFiles.empty() && untrackedFiles.empty()) {
        std::cout << Colors::GREEN << "✅ Working tree clean" << Colors::RESET << "\n";
    }
}

// Enhanced log with better formatting and color
void MiniGit::log(int maxCommits) {
    loadBranches();
    std::string current = getHEAD();
    if (current.empty()) {
        std::cout << Colors::YELLOW << "No commits yet." << Colors::RESET << "\n";
        return;
    }
    
    int count = 0;
    while (!current.empty() && (maxCommits <= 0 || count < maxCommits)) {
        Commit c = loadCommit(current);
        
        std::cout << Colors::YELLOW << "commit " << c.hash << Colors::RESET;
        if (current == getHEAD()) {
            std::cout << Colors::CYAN << " (HEAD -> " << currentBranch << ")" << Colors::RESET;
        }
        std::cout << "\n";
        
        std::cout << Colors::DIM << "Date: " << c.timestamp << Colors::RESET << "\n";
        std::cout << "\n    " << c.message << "\n\n";
        
        current = c.parent;
        count++;
    }
}

// Rest of the original methods with improved output formatting...

// Computes a hash for the given file content using std::hash
std::string MiniGit::computeHash(const std::string& content) {
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << std::hex << hasher(content);
    return ss.str();
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
            if (!fileEntries.empty()) {
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
    if (content.empty()) return "";
    
    content.erase(content.find_last_not_of(" \n\r\t") + 1);
    
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
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line.empty()) continue;
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string branchName = line.substr(0, colon);
            std::string commitHash = line.substr(colon + 1);
            branches[branchName] = commitHash;
        }
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
    while (std::getline(iss, filename)) {
        filename.erase(filename.find_last_not_of(" \n\r\t") + 1);
        if (!filename.empty()) stagedFiles.insert(filename);
    }
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
    
    std::set<std::string> ancestors1;
    std::string current = commit1;
    while (!current.empty()) {
        ancestors1.insert(current);
        Commit c = loadCommit(current);
        current = c.parent;
    }
    
    current = commit2;
    while (!current.empty()) {
        if (ancestors1.count(current)) return current;
        Commit c = loadCommit(current);
        current = c.parent;
    }
    
    return "";
}

bool MiniGit::isAncestor(const std::string& child, const std::string& ancestor) {
    std::string temp = child;
    while (!temp.empty()) {
        if (temp == ancestor) return true;
        Commit c = loadCommit(temp);
        temp = c.parent;
    }
    return false;
}

void MiniGit::fastForwardMerge(const std::string& newHead) {
    branches[currentBranch] = newHead;
    updateHEAD(newHead);

    if (!newHead.empty()) {
        Commit c = loadCommit(newHead);
        for (size_t i = 0; i < c.filenames.size(); ++i) {
            std::string content = readFromFile(objectsDir + "/" + c.blobHashes[i]);
            writeToFile(c.filenames[i], content);
        }
    }
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
        std::cout << Colors::YELLOW << "Repository already initialized." << Colors::RESET << "\n";
        return;
    }
    fs::create_directory(minigitDir);
    fs::create_directory(objectsDir);
    fs::create_directory(refsDir);
    writeToFile(headFile, "master:");
    writeToFile(refsDir + "/branches", "master:\n");
    writeToFile(indexFile, "");
    branches["master"] = "";
    currentBranch = "master";
    std::cout << Colors::GREEN << "✅ Initialized empty MiniGit repository." << Colors::RESET << "\n";
}

// Stages a file by hashing its contents and adding it to index
void MiniGit::add(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cout << Colors::RED << "❌ File '" << filename << "' not found." << Colors::RESET << "\n";
        return;
    }
    std::string content = readFromFile(filename);
    std::string hash = computeHash(content);
    writeToFile(objectsDir + "/" + hash, content);
    loadStagedFiles();
    stagedFiles.insert(filename);
    saveStagedFiles();
    std::cout << Colors::GREEN << "✅ Added '" << filename << "' to staging area." << Colors::RESET << "\n";
}

// Commits staged changes with a message and saves it as a new commit
void MiniGit::commit(const std::string& message) {
    loadStagedFiles();
    loadBranches();
    if (stagedFiles.empty()) {
        std::cout << Colors::YELLOW << "⚠️  No changes to commit." << Colors::RESET << "\n";
        return;
    }

    Commit commit;
    commit.message = message;
    commit.timestamp = getCurrentTime();
    commit.parent = getHEAD();

    for (const auto& file : stagedFiles) {
        std::string content = readFromFile(file);
        std::string hash = computeHash(content);
        writeToFile(objectsDir + "/" + hash, content);
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
    cleanupMergeState(); // Clean up any merge state
    std::cout << Colors::GREEN << "✅ Committed changes with hash: " << Colors::BOLD << commit.hash.substr(0, 8) << Colors::RESET << "\n";
}

// Creates a new branch pointing to current HEAD
void MiniGit::branch(const std::string& name) {
    loadBranches();
    if (branches.find(name) != branches.end()) {
        std::cout << Colors::YELLOW << "⚠️  Branch '" << name << "' already exists." << Colors::RESET << "\n";
        return;
    }
    branches[name] = getHEAD();
    saveBranches();
    std::cout << Colors::GREEN << "✅ Created branch '" << name << "'." << Colors::RESET << "\n";
}

// Lists all branches and marks the current one with '*'
void MiniGit::listBranches() {
    loadBranches();
    std::cout << Colors::BOLD << "Branches:" << Colors::RESET << "\n";
    for (const auto& [name, hash] : branches) {
        if (name == currentBranch) {
            std::cout << Colors::GREEN << "* " << name << Colors::RESET;
            if (!hash.empty()) {
                std::cout << Colors::DIM << " (" << hash.substr(0, 8) << ")" << Colors::RESET;
            }
            std::cout << "\n";
        } else {
            std::cout << "  " << name;
            if (!hash.empty()) {
                std::cout << Colors::DIM << " (" << hash.substr(0, 8) << ")" << Colors::RESET;
            }
            std::cout << "\n";
        }
    }
}

// Switches to another branch and restores files from the commit
void MiniGit::checkout(const std::string& target) {
    loadBranches();
    loadStagedFiles();
    
    // Check for uncommitted changes
    if (!stagedFiles.empty()) {
        std::cout << Colors::RED << "❌ Cannot checkout: you have uncommitted changes. Please commit them first." << Colors::RESET << "\n";
        return;
    }
    
    if (branches.count(target)) {
        // Get current and target commit files
        std::string currentHash = getHEAD();
        std::string targetHash = branches[target];
        
        std::map<std::string, std::string> currentFiles = getCommitFiles(currentHash);
        std::map<std::string, std::string> targetFiles = getCommitFiles(targetHash);
        
        // Remove files that exist in current but not in target
        for (const auto& [filename, hash] : currentFiles) {
            if (targetFiles.find(filename) == targetFiles.end()) {
                if (fs::exists(filename)) {
                    fs::remove(filename);
                    std::cout << Colors::DIM << "Removed: " << filename << Colors::RESET << "\n";
                }
            }
        }
        
        // Add/update files from target commit
        for (const auto& [filename, hash] : targetFiles) {
            std::string content = readFromFile(objectsDir + "/" + hash);
            writeToFile(filename, content);
            std::cout << Colors::DIM << "Updated: " << filename << Colors::RESET << "\n";
        }
        
        currentBranch = target;
        updateHEAD(targetHash);
        std::cout << Colors::GREEN << "✅ Switched to branch '" << target << "'." << Colors::RESET << "\n";
    } else {
        std::cout << Colors::RED << "❌ Branch '" << target << "' does not exist." << Colors::RESET << "\n";
    }
}

void MiniGit::merge(const std::string& branchName) {
    loadBranches();
    loadStagedFiles();

    // Check if the branch exists
    if (branches.find(branchName) == branches.end()) {
        std::cout << Colors::RED << "❌ Branch '" << branchName << "' does not exist." << Colors::RESET << "\n";
        return;
    }

    // Prevent merging the branch into itself
    if (branchName == currentBranch) {
        std::cout << Colors::RED << "❌ Cannot merge a branch with itself." << Colors::RESET << "\n";
        return;
    }

    // Ensure no uncommitted/staged changes
    if (!stagedFiles.empty()) {
        std::cout << Colors::RED << "❌ Uncommitted changes detected. Please commit or unstage before merging." << Colors::RESET << "\n";
        return;
    }

    std::string currentHead = getHEAD();
    std::string targetHead = branches[branchName];

    // Handle cases where branches are empty
    if (currentHead.empty() && targetHead.empty()) {
        std::cout << Colors::BLUE << "ℹ️  Nothing to merge: both branches are empty." << Colors::RESET << "\n";
        return;
    }

    // Fast-forward if current is empty
    if (currentHead.empty()) {
        fastForwardMerge(targetHead);
        std::cout << Colors::GREEN << "✅ Fast-forward merge completed (current was empty)." << Colors::RESET << "\n";
        return;
    }

    if (targetHead.empty()) {
        std::cout << Colors::BLUE << "ℹ️  Target branch is empty. Nothing to merge." << Colors::RESET << "\n";
        return;
    }

    // Check if target is ancestor of current (already merged)
    if (isAncestor(currentHead, targetHead)) {
        std::cout << Colors::GREEN << "✅ Already up to date. Nothing to merge." << Colors::RESET << "\n";
        return;
    }

    // Check if current is ancestor of target (fast-forward possible)
    if (isAncestor(targetHead, currentHead)) {
        fastForwardMerge(targetHead);
        std::cout << Colors::GREEN << "✅ Fast-forward merge completed." << Colors::RESET << "\n";
        return;
    }

    // Find common ancestor for proper merge
    std::string ancestor = findCommonAncestor(currentHead, targetHead);
    if (ancestor.empty()) {
        std::cout << Colors::RED << "❌ No common ancestor found. Cannot merge unrelated histories." << Colors::RESET << "\n";
        return;
    }

    // Perform three-way merge
    performThreeWayMerge(currentHead, targetHead, ancestor, branchName);
}

// Three-way merge implementation
void MiniGit::performThreeWayMerge(const std::string& currentHead, const std::string& targetHead, 
                                   const std::string& ancestor, const std::string& branchName) {
    std::map<std::string, std::string> baseFiles = getCommitFiles(ancestor);
    std::map<std::string, std::string> currentFiles = getCommitFiles(currentHead);
    std::map<std::string, std::string> targetFiles = getCommitFiles(targetHead);
    
    // Collect all file names
    std::set<std::string> allFiles;
    for (const auto& [name, hash] : baseFiles) allFiles.insert(name);
    for (const auto& [name, hash] : currentFiles) allFiles.insert(name);
    for (const auto& [name, hash] : targetFiles) allFiles.insert(name);
    
    bool hasConflicts = false;
    std::map<std::string, std::string> mergedFiles;
    
    std::cout << Colors::CYAN << "🔄 Performing three-way merge..." << Colors::RESET << "\n";
    
    for (const std::string& filename : allFiles) {
        std::string baseHash = baseFiles.count(filename) ? baseFiles[filename] : "";
        std::string currentHash = currentFiles.count(filename) ? currentFiles[filename] : "";
        std::string targetHash = targetFiles.count(filename) ? targetFiles[filename] : "";
        
        // Case 1: File unchanged in both branches
        if (currentHash == targetHash) {
            if (!currentHash.empty()) {
                mergedFiles[filename] = currentHash;
                std::cout << Colors::DIM << "  Unchanged: " << filename << Colors::RESET << "\n";
            }
            continue;
        }
        
        // Case 2: File only changed in current branch
        if (baseHash == targetHash && baseHash != currentHash) {
            if (!currentHash.empty()) {
                mergedFiles[filename] = currentHash;
                std::cout << Colors::GREEN << "  Keep current: " << filename << Colors::RESET << "\n";
            }
            continue;
        }
        
        // Case 3: File only changed in target branch
        if (baseHash == currentHash && baseHash != targetHash) {
            if (!targetHash.empty()) {
                mergedFiles[filename] = targetHash;
                std::cout << Colors::BLUE << "  Take target: " << filename << Colors::RESET << "\n";
            }
            continue;
        }
        
        // Case 4: File added in both branches with same content
        if (baseHash.empty() && currentHash == targetHash && !currentHash.empty()) {
            mergedFiles[filename] = currentHash;
            std::cout << Colors::GREEN << "  Same addition: " << filename << Colors::RESET << "\n";
            continue;
        }
        
        // Case 5: File deleted in both branches
        if (!baseHash.empty() && currentHash.empty() && targetHash.empty()) {
            std::cout << Colors::DIM << "  Deleted in both: " << filename << Colors::RESET << "\n";
            continue;
        }
        
        // Case 6: Conflict - file changed in both branches differently
        std::cout << Colors::RED << "  ⚠️  Conflict: " << filename << Colors::RESET << "\n";
        hasConflicts = true;
        
        // Create conflict markers
        std::string currentContent = currentHash.empty() ? "" : readFromFile(objectsDir + "/" + currentHash);
        std::string targetContent = targetHash.empty() ? "" : readFromFile(objectsDir + "/" + targetHash);
        
        std::string conflictContent = "<<<<<<< HEAD\n" + currentContent + 
                                     "\n=======\n" + targetContent + 
                                     "\n>>>>>>> " + branchName + "\n";
        
        writeToFile(filename, conflictContent);
    }
    
    if (hasConflicts) {
        // Save merge state
        writeToFile(minigitDir + "/MERGE_HEAD", targetHead);
        std::cout << Colors::RED << "❌ Merge conflicts detected. Fix conflicts and commit to complete merge." << Colors::RESET << "\n";
        std::cout << Colors::DIM << "   Files with conflicts have been marked with conflict markers." << Colors::RESET << "\n";
        return;
    }
    
    // No conflicts - create merge commit
    std::cout << Colors::GREEN << "✅ Auto-merge successful. Creating merge commit..." << Colors::RESET << "\n";
    
    // Stage merged files
    stagedFiles.clear();
    for (const auto& [filename, hash] : mergedFiles) {
        std::string content = readFromFile(objectsDir + "/" + hash);
        writeToFile(filename, content);
        stagedFiles.insert(filename);
    }
    
    // Remove files that don't exist in merge result
    for (const auto& [filename, hash] : currentFiles) {
        if (mergedFiles.find(filename) == mergedFiles.end() && fs::exists(filename)) {
            fs::remove(filename);
        }
    }
    
    saveStagedFiles();
    
    // Create merge commit
    std::string mergeMessage = "Merge branch '" + branchName + "'";
    commit(mergeMessage);
    
    std::cout << Colors::GREEN << "✅ Merge completed successfully." << Colors::RESET << "\n";
}

// Helper function to show help/usage information
void MiniGit::showHelp() {
    std::cout << Colors::BOLD << Colors::CYAN << "MiniGit - A Simplified Git Implementation" << Colors::RESET << "\n\n";
    
    std::cout << Colors::BOLD << "USAGE:" << Colors::RESET << "\n";
    std::cout << "  minigit <command> [arguments]\n\n";
    
    std::cout << Colors::BOLD << "COMMANDS:" << Colors::RESET << "\n";
    std::cout << Colors::GREEN << "  init" << Colors::RESET << "                     Initialize a new repository\n";
    std::cout << Colors::GREEN << "  add <file>" << Colors::RESET << "              Add file to staging area\n";
    std::cout << Colors::GREEN << "  commit <message>" << Colors::RESET << "       Commit staged changes\n";
    std::cout << Colors::GREEN << "  status" << Colors::RESET << "                  Show working tree status\n";
    std::cout << Colors::GREEN << "  log [n]" << Colors::RESET << "                Show commit history (optionally limit to n commits)\n";
    std::cout << Colors::GREEN << "  diff [options]" << Colors::RESET << "          Show differences\n";
    std::cout << Colors::GREEN << "  branch <name>" << Colors::RESET << "          Create a new branch\n";
    std::cout << Colors::GREEN << "  branch -l" << Colors::RESET << "              List all branches\n";
    std::cout << Colors::GREEN << "  checkout <branch>" << Colors::RESET << "      Switch to a branch\n";
    std::cout << Colors::GREEN << "  merge <branch>" << Colors::RESET << "         Merge a branch into current\n";
    std::cout << Colors::GREEN << "  help" << Colors::RESET << "                   Show this help message\n\n";
    
    std::cout << Colors::BOLD << "DIFF OPTIONS:" << Colors::RESET << "\n";
    std::cout << "  diff                        Show unstaged changes\n";
    std::cout << "  diff --staged              Show staged changes vs last commit\n";
    std::cout << "  diff <commit>              Show working directory vs commit\n";
    std::cout << "  diff <commit1> <commit2>   Show differences between two commits\n\n";
    
    std::cout << Colors::BOLD << "EXAMPLES:" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit init" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit add file.txt" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit commit \"Initial commit\"" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit branch feature" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit checkout feature" << Colors::RESET << "\n";
    std::cout << Colors::DIM << "  minigit diff --staged" << Colors::RESET << "\n";
}
