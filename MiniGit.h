#ifndef MINIGIT_H
#define MINIGIT_H

#include <string>
#include <vector>
#include <map>
#include <set>

class MiniGit {
private:
    struct Blob {
        std::string hash;
        std::string content;
        std::string filename;
    };
    
    struct Commit {
        std::string hash;
        std::string message;
        std::string timestamp;
        std::string parent;
        std::vector<std::string> blobHashes;
        std::vector<std::string> filenames;
    };
    
    // Directory and file paths
    const std::string minigitDir = ".minigit";
    const std::string objectsDir = minigitDir + "/objects";
    const std::string refsDir = minigitDir + "/refs";
    const std::string headFile = minigitDir + "/HEAD";
    const std::string indexFile = minigitDir + "/index";
    
    // Repository state
    std::map<std::string, std::string> branches;
    std::set<std::string> stagedFiles;
    std::string currentBranch = "master";
    
    // Core utility methods
    std::string computeHash(const std::string& content);
    std::string getCurrentTime();
    void writeToFile(const std::string& filepath, const std::string& content);
    std::string readFromFile(const std::string& filepath);
    
    // Commit management
    void saveCommit(const Commit& commit);
    Commit loadCommit(const std::string& hash);
    void updateHEAD(const std::string& commitHash);
    std::string getHEAD();
    
    // Branch and staging management
    void loadBranches();
    void saveBranches();
    void loadStagedFiles();
    void saveStagedFiles();
    
    // Merge helper methods
    void cleanupMergeState();
    std::string findCommonAncestor(const std::string& commit1, const std::string& commit2);
    std::map<std::string, std::string> getCommitFiles(const std::string& commitHash);
    bool filesAreSame(const std::string& hash1, const std::string& hash2);
    
public:
    // Public interface methods
    void init();
    void add(const std::string& filename);
    void commit(const std::string& message);
    void log();
    void status();
    void branch(const std::string& name);
    void listBranches();
    void checkout(const std::string& target);
    void merge(const std::string& branchName);
};

#endif // MINIGIT_H
