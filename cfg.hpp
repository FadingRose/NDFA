#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <cctype>
#include <algorithm>
#include <unordered_set>

struct CFG {
    std::set<char> non_terminals;
    std::set<char> terminals;
    std::map<char, std::vector<std::string>> productions;
    char start_symbol;
    char epsilon_symbol = '@';
};

CFG Read_CFG_File(const std::string& filename) {
    CFG cfg;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not open file." << std::endl;
        return cfg; // Return empty CFG if the file couldn't be opened
    }

    std::string line;
    bool isFirst = true; // To identify the start symbol

    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        char non_terminal = line[0];
        cfg.non_terminals.insert(non_terminal);

        if (isFirst) {
            cfg.start_symbol = non_terminal;
            isFirst = false;
        }

        std::string right_hand_side = line.substr(5); // Skipping "A -> "
        std::istringstream ss(right_hand_side);
        std::string production;

        while (std::getline(ss, production, '|')) {
            //clean the rhs
            std::string filtered_rhs;
            for (const auto& c : production) {
                if (c != ' ') {
                    if (std::isalpha(c) || c == '@') {
                        filtered_rhs += c;
                    }
                }

            }

            production = filtered_rhs;
            cfg.productions[non_terminal].push_back(production);

            // Identify terminals
            for (char c : production) {
                if (!isupper(c) && c != ' ' && c != '\t') {
                    cfg.terminals.insert(c);
                }
            }
        }
    }

    return cfg;
}

void Output_CFG_Struct(const CFG& cfg) {
    for (const auto& production : cfg.productions) {
        std::cout << production.first << " -> ";
        for (size_t i = 0; i < production.second.size(); ++i) {
            std::cout << production.second[i];
            if (i < production.second.size() - 1) {
                std::cout << " | ";
            }
        }
        std::cout << std::endl;
    }
}

// In this example, we suppose that all of the Non-Terminal is upper-case.
bool isNonterminal(const char& c) {
    if (isupper(c)) {
        return true;
    }

    return false;
}

// Eliminate Unreachable Non-terminal and Non-terminating Symbols
CFG EliminateUselessSymbols(CFG& cfg) {
    // Step 1 : Eliminate Unreachable Non-terminal Symbols
    std::set<char> reachable = { cfg.start_symbol };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& production : cfg.productions) {
            //set.find(element):: FALSE -> return [set.end()]

            //if a left-hand char is reachable, then all chars in the right-hand string are reachable.
            if (reachable.find(production.first) != reachable.end()) {
                for (const auto& rhs : production.second) {
                    for (char c : rhs) {
                        if (isNonterminal(c) && reachable.find(c) == reachable.end()) {
                            reachable.insert(c);
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    // Remove Unreachable Symbols from Productions and Non-Terminals
    // For a specific Production P*, if the left-hand is UNREACHABLE, then we can erase the P*.
    for (auto it = cfg.productions.begin(); it != cfg.productions.end();) {
        if (reachable.find(it->first) == reachable.end()) {
            it = cfg.productions.erase(it);
        }
        else {
            ++it;
        }
    }


    // Step 2 Eliminate Non-Terminating Symbols
    std::set<char> terminating;
    changed = true;
    while (changed) {
        changed = false;
        for (const auto& production : cfg.productions) {
            for (const auto& rhs : production.second) {
                // For a specific production, we check the right-hand first,
                // True :: c is a Terminal || c is not a Terminal and has been marked in the terminating
                if (std::all_of(rhs.begin(), rhs.end(), [&](char c) {
                    return !isNonterminal(c) || terminating.find(c) != terminating.end();
                    })) {
                    if (terminating.find(production.first) == terminating.end()) {
                        terminating.insert(production.first);
                        changed = true;
                    }
                }
            }
        }
    }

    // Remove Non-Terminating Symbols from Productions amd Non-Terminals
    for (auto it = cfg.productions.begin();it != cfg.productions.end();) {
        // For a specific production, the lhs symbol can't conclude Terminal, remove it.
        if (terminating.find(it->first) == terminating.end()) {
            it = cfg.productions.erase(it);
        }
        else {
            ++it;
        }
    }

    return cfg;

}


// Eliminate all the Single Production A->B
// A -> B
// B -> abc
//----eliminate-----
// A ->abc
CFG EliminateSingleProduction(CFG& cfg) {
    // This map will track all the unitProduction A -> B
    std::map<char, std::set<char>> unitProduction;

    // Initialize unitProduction map
    // unitProduction[A] = A
    // self must inherit itself's right-hand
    for (const auto& non_terminal : cfg.non_terminals) {
        unitProduction[non_terminal].insert(non_terminal);
    }

    //Step 1 : Find all the unit production
    for (const auto& non_terminal : cfg.non_terminals) {
        bool changed = true;
        while (changed) {
            changed = false;
            for (const auto& target : unitProduction[non_terminal]) {
                for (const auto& rhs : cfg.productions[target]) {
                    if (rhs.length() == 1 && isNonterminal(rhs[0])) {
                        //Find a unit production
                        // For a specific non-terminal A at the left-hand
                        // A-> B | aa | C
                        // We got that:
                        // unitProduction['A'] = {'B', 'C'}
                        if (unitProduction[non_terminal].insert(rhs[0]).second) {
                            // 'set' will ensure that the elements is UNIQUE
                            // if a existed element is inserted repeatedly, it will return false
                            changed = true;
                        }
                    }
                }
            }
        }
    }

    // Step 2 : Replace all unit productions
    for (const auto& non_terminal : cfg.non_terminals) {
        std::unordered_set<std::string> newProduction;
        // For a specific non-terminal A at the left-hand
        // A-> B | aa | C
        // B-> <string> | <string>

        // In the source CFG, there're some strings are not unit production
        // A -> aa
        // we push_back('aa') into the newProduction
        for (const auto& rhs : cfg.productions[non_terminal]) {

            // Not a unit production
            if (!(rhs.length() == 1 && isNonterminal(rhs[0]))) {
                newProduction.insert(rhs);
            }
        }

        // In the mapping, we know that A->B and A->C is a Unit Production
        // unitProduction['A'] = {'B', 'C'}
        for (const auto& target : unitProduction[non_terminal]) {
            // target is selected from {'B', 'C'}
            for (const auto& rhs : cfg.productions[target]) {
                // focusing on the B->...rhs... | C->...rhs... from the source CFG

                //select the Non-unit rhs, push_back into new production
                if (!(rhs.length() == 1 && isNonterminal(rhs[0]))) {
                    newProduction.insert(rhs);
                }
            }
        }

        // update the source CFG

        cfg.productions[non_terminal] = std::vector<std::string>(newProduction.begin(), newProduction.end());
    }

    return cfg;
}



// Eliminating epsilon productions
// Step 1:: Find all of the Non-Terminals product ε.
// Step 2:: For each A->α, if α includes some Non-Terminals which can conclude ε, then add some external productions, and these Non-Terminals will be deleted.
// Step 3:: Deleting all the pure A->ε production, exclude S->ε
//
// For example, we have some productions as follow shows:
// A -> BCD , assume that B and D can product ε
// then we add some new productions:
// A -> BCD // reserved
// A -> CD  // delete B
// A -> BC  // delete D
// A -> C   // delete B, D
CFG EliminateEpsilonProductions(CFG& cfg) {
    // Step 1:: Find all of the Non-Terminals product ε.
    std::set<char>nullableSet;

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [non_terminal, rhs_list] : cfg.productions) {
            for (const auto& rhs : rhs_list) {
                if (std::all_of(rhs.begin(), rhs.end(), [&](char c) {
                    // Indirect products || Direct products
                    return nullableSet.count(c) > 0 || c == '@';
                    })) {
                    if (nullableSet.insert(non_terminal).second) {
                        changed = true;
                    }
                }
            }
        }
    }

    // Step 2: Add new productions
    for (const auto& [non_terminal, rhs_list] : cfg.productions) {
        // for each production
        std::set<std::string> generate_rhs_list = std::set<std::string>(rhs_list.begin(), rhs_list.end());

        // we hold a new copy for a specific non_terminal's rhs_list to dynamic insert it.
        for (const auto& rhs : generate_rhs_list) {
            //for each production's right-hand side <string>
            bool changed = true;
            while (changed) {
                changed = false;

                for (char nullable_non_terminal : nullableSet) {
                    if (rhs.find(nullable_non_terminal) != std::string::npos) {
                        //In the rhs, we find a nullable-non-terminal
                        //then we erase it.
                        std::string new_rhs = rhs;
                        new_rhs.erase(std::remove(new_rhs.begin(), new_rhs.end(), nullable_non_terminal), new_rhs.end());
                        changed = true;
                        if (!new_rhs.empty()) {
                            generate_rhs_list.insert(new_rhs);
                        }
                    }
                }
            }
        }
        cfg.productions[non_terminal] = std::vector<std::string>(generate_rhs_list.begin(), generate_rhs_list.end());
    }

    // Step 3: Remove pure epsilon productions
    for (const auto& non_terminal : cfg.non_terminals) {
        auto& rhs_list = cfg.productions[non_terminal];
        rhs_list.erase(remove(rhs_list.begin(), rhs_list.end(), "@"), rhs_list.end());
        if (non_terminal == cfg.start_symbol && nullableSet.count(non_terminal) > 0) {
            rhs_list.push_back("@");
        }
    }


    return cfg;
}