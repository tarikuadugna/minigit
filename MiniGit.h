#ifndef MINIGIT_H
#define MINIGIT_H

#include <string>
#include <set>
#include <map>
#include <vector>

class MiniGit {
public:
    // Core data structure for commits
    struct Commit {
        std::string hash;
        std::string message;
        std::string timestamp;
        std::string parent;
        std::vector<std::string> filenames;
        std::vector<std::string> blobHashes;
    };

private:
    // Directory and file paths
    std::string minigitDir = ".minigit";
    std::string objectsDir = ".minigit/objects";
    std::string refsDir = ".minigit/refs";
    std::string headFile = ".minigit/HEAD";
    std::string indexFile = ".minigit/index";
    
    // Runtime state
    std::set<std::string> stagedFiles;
    std::map<std::string, std::string> branches;
    std::string currentBranch = "master";

    // Core utility functions
    std::string computeHash(const std::string& content);
    std::string getCurrentTime();
    void writeToFile(const std::string& filepath, const std::string& content);
    std::string readFromFile(const std::string& filepath);
    
    // Commit and branch management
    void saveCommit(const Commit& commit);
    Commit loadCommit(const std::string& hash);
    void updateHEAD(const std::string& commitHash);
    std::string getHEAD();
    void loadBranches();
    void saveBranches();
    void loadStagedFiles();
    void saveStagedFiles();
    
    // Merge helpers
    void cleanupMergeState();
    std::string findCommonAncestor(const std::string& commit1, const std::string& commit2);
    bool isAncestor(const std::string& child, const std::string& ancestor);
    void fastForwardMerge(const std::string& newHead);
    std::map<std::string, std::string> getCommitFiles(const std::string& commitHash);
    bool filesAreSame(const std::string& hash1, const std::string& hash2);
    void performThreeWayMerge(const std::string& currentHead, const std::string& targetHead, 
                             const std::string& ancestor, const std::string& branchName);
    
    // New diff utility functions
    std::vector<std::string> splitLines(const std::string& text);
    std::vector<std::vector<int>> computeLCS(const std::vector<std::string>& a, const std::vector<std::string>& b);
    void showUnifiedDiff(const std::string& filename, const std::string& oldContent, const std::string& newContent);
    void showWorkingDiff();
    void showStagedDiff();
    void showCommitDiff(const std::string& commit1, const std::string& commit2);

public:
    // Main git operations
    void init();
    void add(const std::string& filename);
    void commit(const std::string& message);
    void status();
    void log(int maxCommits = 0);  // Enhanced with optional limit
    void branch(const std::string& name);
    void listBranches();
    void checkout(const std::string& target);
    void merge(const std::string& branchName);
    
    // New diff command with multiple options
    void diff(const std::string& option1 = "", const std::string& option2 = "");
    
    // Help system
    void showHelp();

    void reset(const std::string& filename = "");
    void remove(const std::string& filename);
    void resetHard(const std::string& commitHash);
};

#endif // MINIGIT_H
