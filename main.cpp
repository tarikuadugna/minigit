#include "MiniGit.h"
#include <iostream>

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
    MiniGit git;
    if (argc < 2) {
        printUsage();
        return 1;
    }
    std::string cmd = argv[1];
    if (cmd == "init") git.init();
    else if (cmd == "add" && argc >= 3) git.add(argv[2]);
    else if (cmd == "commit" && argc >= 4 && std::string(argv[2]) == "-m") git.commit(argv[3]);
    else if (cmd == "log") git.log();
    else if (cmd == "status") git.status();
    else if (cmd == "branch") {
        if (argc >= 3) git.branch(argv[2]);
        else git.listBranches();
    }
    else if (cmd == "checkout" && argc >= 3) git.checkout(argv[2]);
    else if (cmd == "help") printUsage();
    else {
        std::cout << "Invalid command.\n";
        printUsage();
    }
    return 0;
}