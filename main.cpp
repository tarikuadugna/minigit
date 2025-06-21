#include "MiniGit.h"
#include <iostream>

// Print the usage/help instructions for MiniGit commands
void printUsage() {
    std::cout << "MiniGit - Lightweight VCS\n\nUsage:\n"
              << "  minigit init                    - Initialize repo\n"
              << "  minigit add <file>              - Stage file\n"
              << "  minigit commit -m \"msg\"         - Commit\n"
              << "  minigit log                     - Show history\n"
              << "  minigit status                  - Repo status\n"
              << "  minigit branch <name>           - Create branch\n"
              << "  minigit branch                  - List branches\n"
              << "  minigit checkout <name/hash>    - Checkout\n"
              << "  minigit help                    - Help\n";
}

int main(int argc, char* argv[]) {
    MiniGit git;  // Create MiniGit instance to perform operations

    // Check if at least one command-line argument is given
    if (argc < 2) {
        printUsage();  // If no command, print usage instructions
        return 1;      // Exit with error code
    }

    std::string cmd = argv[1];  // Command is first argument

    // Dispatch commands to corresponding MiniGit methods based on input
    if (cmd == "init")
        git.init();  // Initialize repository

    else if (cmd == "add" && argc >= 3)
        git.add(argv[2]);  // Stage a file (second argument)

    else if (cmd == "commit" && argc >= 4 && std::string(argv[2]) == "-m")
        git.commit(argv[3]);  // Commit staged files with message (third argument)

    else if (cmd == "log")
        git.log();  // Show commit history

    else if (cmd == "status")
        git.status();  // Show repository status and staged files

    else if (cmd == "branch") {
        // If branch name specified, create new branch; else list branches
        if (argc >= 3)
            git.branch(argv[2]);
        else
            git.listBranches();
    }

    else if (cmd == "checkout" && argc >= 3)
        git.checkout(argv[2]);  // Switch to branch or commit hash

    else if (cmd == "help")
        printUsage();  // Print help instructions

    else {
        std::cout << "Invalid command.\n";
        printUsage();  // Print usage if command not recognized
    }

    return 0;  // Exit program successfully
}
