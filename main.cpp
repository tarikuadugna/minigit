#include "MiniGit.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// ANSI color codes for CLI output
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string BOLD = "\033[1m";
}

void showWelcome() {
    std::cout << Colors::BOLD << Colors::CYAN;
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║            🚀 MiniGit CLI            ║\n";
    std::cout << "║     A Simplified Git Implementation  ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    std::cout << Colors::RESET << "\n";
    std::cout << "Type 'help' for available commands or 'exit' to quit.\n\n";
}

std::vector<std::string> parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    
    for (char c : input) {
        if (c == '"' || c == '\'') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

void runInteractiveMode() {
    MiniGit git;
    showWelcome();
    
    std::string input;
    while (true) {
        std::cout << Colors::BLUE << "minigit> " << Colors::RESET;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        auto tokens = parseCommand(input);
        if (tokens.empty()) continue;
        
        std::string command = tokens[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        try {
            if (command == "exit" || command == "quit") {
                std::cout << Colors::GREEN << "👋 Goodbye!" << Colors::RESET << "\n";
                break;
            }
            else if (command == "help" || command == "--help" || command == "-h") {
                git.showHelp();
            }
            else if (command == "init") {
                git.init();
            }
            else if (command == "add") {
                if (tokens.size() < 2) {
                    std::cout << Colors::RED << "❌ Usage: add <filename>" << Colors::RESET << "\n";
                } else {
                    git.add(tokens[1]);
                }
            }
            else if (command == "commit") {
                if (tokens.size() < 2) {
                    std::cout << Colors::RED << "❌ Usage: commit <message>" << Colors::RESET << "\n";
                } else {
                    // Join all tokens after "commit" as the message
                    std::string message;
                    for (size_t i = 1; i < tokens.size(); ++i) {
                        if (i > 1) message += " ";
                        message += tokens[i];
                    }
                    git.commit(message);
                }
            }
            else if (command == "status") {
                git.status();
            }
            else if (command == "log") {
                if (tokens.size() > 1) {
                    try {
                        int limit = std::stoi(tokens[1]);
                        git.log(limit);
                    } catch (const std::exception&) {
                        std::cout << Colors::RED << "❌ Invalid number for log limit" << Colors::RESET << "\n";
                    }
                } else {
                    git.log();
                }
            }
            else if (command == "diff") {
                if (tokens.size() == 1) {
                    git.diff();
                } else if (tokens.size() == 2) {
                    git.diff(tokens[1]);
                } else if (tokens.size() == 3) {
                    git.diff(tokens[1], tokens[2]);
                } else {
                    std::cout << Colors::RED << "❌ Usage: diff [--staged] [commit1] [commit2]" << Colors::RESET << "\n";
                }
            }
            else if (command == "branch") {
                if (tokens.size() == 1) {
                    git.listBranches();
                } else if (tokens.size() == 2) {
                    if (tokens[1] == "-l" || tokens[1] == "--list") {
                        git.listBranches();
                    } else {
                        git.branch(tokens[1]);
                    }
                } else {
                    std::cout << Colors::RED << "❌ Usage: branch [name] or branch -l" << Colors::RESET << "\n";
                }
            }
            else if (command == "checkout") {
                if (tokens.size() < 2) {
                    std::cout << Colors::RED << "❌ Usage: checkout <branch>" << Colors::RESET << "\n";
                } else {
                    git.checkout(tokens[1]);
                }
            }
            else if (command == "merge") {
                if (tokens.size() < 2) {
                    std::cout << Colors::RED << "❌ Usage: merge <branch>" << Colors::RESET << "\n";
                } else {
                    git.merge(tokens[1]);
                }
            }
            else if (command == "clear" || command == "cls") {
                #ifdef _WIN32
                    system("cls");
                #else
                    system("clear");
                #endif
            }
            else {
                std::cout << Colors::RED << "❌ Unknown command: " << command << Colors::RESET << "\n";
                std::cout << Colors::GREEN << "Type 'help' for available commands." << Colors::RESET << "\n";
            }
        } catch (const std::exception& e) {
            std::cout << Colors::RED << "❌ Error: " << e.what() << Colors::RESET << "\n";
        }
        
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    MiniGit git;
    
    // If no arguments, run in interactive mode
    if (argc == 1) {
        runInteractiveMode();
        return 0;
    }
    
    // Command line mode
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
    
    std::string command = args[0];
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    try {
        if (command == "help" || command == "--help" || command == "-h") {
            git.showHelp();
        }
        else if (command == "init") {
            git.init();
        }
        else if (command == "add") {
            if (args.size() < 2) {
                std::cout << Colors::RED << "❌ Usage: minigit add <filename>" << Colors::RESET << "\n";
                return 1;
            }
            git.add(args[1]);
        }
        else if (command == "commit") {
            if (args.size() < 2) {
                std::cout << Colors::RED << "❌ Usage: minigit commit <message>" << Colors::RESET << "\n";
                return 1;
            }
            // Join all arguments after "commit" as the message
            std::string message;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) message += " ";
                message += args[i];
            }
            git.commit(message);
        }
        else if (command == "status") {
            git.status();
        }
        else if (command == "log") {
            if (args.size() > 1) {
                try {
                    int limit = std::stoi(args[1]);
                    git.log(limit);
                } catch (const std::exception&) {
                    std::cout << Colors::RED << "❌ Invalid number for log limit" << Colors::RESET << "\n";
                    return 1;
                }
            } else {
                git.log();
            }
        }
        else if (command == "diff") {
            if (args.size() == 1) {
                git.diff();
            } else if (args.size() == 2) {
                git.diff(args[1]);
            } else if (args.size() == 3) {
                git.diff(args[1], args[2]);
            } else {
                std::cout << Colors::RED << "❌ Usage: minigit diff [--staged] [commit1] [commit2]" << Colors::RESET << "\n";
                return 1;
            }
        }
        else if (command == "branch") {
            if (args.size() == 1) {
                git.listBranches();
            } else if (args.size() == 2) {
                if (args[1] == "-l" || args[1] == "--list") {
                    git.listBranches();
                } else {
                    git.branch(args[1]);
                }
            } else {
                std::cout << Colors::RED << "❌ Usage: minigit branch [name] or minigit branch -l" << Colors::RESET << "\n";
                return 1;
            }
        }
        else if (command == "checkout") {
            if (args.size() < 2) {
                std::cout << Colors::RED << "❌ Usage: minigit checkout <branch>" << Colors::RESET << "\n";
                return 1;
            }
            git.checkout(args[1]);
        }
        else if (command == "merge") {
            if (args.size() < 2) {
                std::cout << Colors::RED << "❌ Usage: minigit merge <branch>" << Colors::RESET << "\n";
                return 1;
            }
            git.merge(args[1]);
        }
        else {
            std::cout << Colors::RED << "❌ Unknown command: " << command << Colors::RESET << "\n";
            std::cout << Colors::GREEN << "Use 'minigit help' for available commands." << Colors::RESET << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << Colors::RED << "❌ Error: " << e.what() << Colors::RESET << "\n";
        return 1;
    }
    
    return 0;
}
