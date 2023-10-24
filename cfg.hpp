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
#include <queue>
#include <tuple>

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
    std::ofstream outfile("File 2.txt");

    // Output the production of the start symbol first
    auto start_symbol_iter = cfg.productions.find(cfg.start_symbol);
    if (start_symbol_iter != cfg.productions.end()) {
        outfile << start_symbol_iter->first << " -> ";
        for (size_t i = 0; i < start_symbol_iter->second.size(); ++i) {
            outfile << start_symbol_iter->second[i];
            if (i < start_symbol_iter->second.size() - 1) {
                outfile << " | ";
            }
        }
        outfile << std::endl;
    }

    // Output the rest of the productions
    for (const auto& production : cfg.productions) {
        if (production.first == cfg.start_symbol) {
            // Skip the start symbol as it has already been processed
            continue;
        }
        outfile << production.first << " -> ";
        for (size_t i = 0; i < production.second.size(); ++i) {
            outfile << production.second[i];
            if (i < production.second.size() - 1) {
                outfile << " | ";
            }
        }
        outfile << std::endl;
    }

    outfile.close();
}


// In this example, we suppose that all of the Non-Terminal is upper-case.
bool isNonterminal(const char& c) {
    if (isupper(c)) {
        return true;
    }

    return false;
}

// B.1 Eliminate Unreachable Non-terminal and Non-terminating Symbols
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
            cfg.non_terminals.erase(it->first);
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

        // check the RHS, any part of RHS which includes Non-Terminating Symbols should be removed
        for (auto rhs_it = it->second.begin(); rhs_it != it->second.end();) {

            auto includes_non_terminating = [&](auto this_string) {// whether a RHS includes non_terminating
                for (char ch : this_string) {
                    if (isNonterminal(ch) && terminating.find(ch) == terminating.end()) {
                        return true;
                    }
                }
                return false;
                };

            if (includes_non_terminating(*rhs_it)) {
                rhs_it = it->second.erase(rhs_it);
            }
            else {
                ++rhs_it;
            }


        }


        // For a specific production, the LHS symbol can't conclude Terminal, remove it.
        if (terminating.find(it->first) == terminating.end()) {
            cfg.non_terminals.erase(it->first);
            it = cfg.productions.erase(it);
        }
        else {
            ++it;
        }
    }

    return cfg;

}


// B.2 Eliminate all the Single Production A->B
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

// B.3 Eliminating epsilon productions
//
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
        std::vector<std::string> generated_all_rhs;
        // we hold a new copy for a specific non_terminal's rhs_list to dynamic insert it.
        for (const auto& rhs : rhs_list) {
            //for each production's right-hand-side's each part
            std::queue<std::string> bfsQueue;
            std::set<std::string> generated_rhs;//A specific rhs may generate more rhs strings
            generated_rhs.insert(rhs);
            bfsQueue.push(rhs);

            while (!bfsQueue.empty()) {
                std::string this_rhs = bfsQueue.front();
                bfsQueue.pop();

                for (char nullable_non_terminal : nullableSet) {
                    std::string new_rhs = this_rhs;
                    new_rhs.erase(std::remove(new_rhs.begin(), new_rhs.end(), nullable_non_terminal), new_rhs.end());
                    if (!new_rhs.empty() && generated_rhs.find(new_rhs) == generated_rhs.end()) {
                        bfsQueue.push(new_rhs);
                        generated_rhs.insert(new_rhs);
                    }
                }
            }
            // update the specific production
            for (auto& _ : generated_rhs) {
                generated_all_rhs.push_back(_);
            }
        }

        cfg.productions[non_terminal] = generated_all_rhs;
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



// B.4 Eliminate Left-Recursion Part-1
// Step 1:: Eliminate Indirect Recursion
//     1.1 sort(Non-Terminals)
//     1.2 foreach Non-Terminal A_i,
//         iterate A_j (j<i)
//         replace A_j in A_i -> A_jα by rhs of A_j
CFG EliminateIndirectLeftRecursion(CFG& cfg) {
    std::map<char, int> nonTerminalSortMap;
    int cnt = 0;
    for (const auto& _ : cfg.non_terminals) {
        nonTerminalSortMap[_] = cnt;
        ++cnt;
    }

    for (const auto& [nonterminal, production] : cfg.productions) {
        //select a production
        std::set<std::string>new_rhs_list;

        for (const auto& rhs : production) {
            //select a specific rhs
            if (isNonterminal(rhs[0]) && nonTerminalSortMap[rhs[0]] < nonTerminalSortMap[nonterminal]) {
                //         replace A_j in A_i -> A_jα by rhs of A_j
                char rhs_non_terminal = rhs[0];// this is A_j

                // Now we need all of the A_j -> <rhs1> | <rhs2> | ...
                for (const auto& _ : cfg.productions[rhs_non_terminal]) {
                    new_rhs_list.insert(_ + rhs.substr(1));
                }
            }
            else {
                new_rhs_list.insert(rhs);
            }
        }

        cfg.productions[nonterminal] = std::vector<std::string>(new_rhs_list.begin(), new_rhs_list.end());
    }

    return cfg;
}

// B.4 Eliminate Left-Recursion Part-2
//      A -> Aα∣β
//      --------
//      A -> βA'
//      A' -> αA' ∣ ε
CFG EliminateDirectLeftRecursion(CFG& cfg) {

    auto isLeftRecursion = [&](const char& non_terminal, const std::string& rhs) -> std::tuple<bool, std::string> {
        if (non_terminal == rhs[0]) {
            return { true,rhs.substr(1) }; //return α
        }
        else {
            return { false, rhs }; // return β
        }
        };

    auto generateNewNonTerminal = [&](const std::set<char>& non_terminal_set) -> std::tuple<bool, char> {
        for (char ch = 'A'; ch <= 'Z'; ++ch) {
            if (non_terminal_set.find(ch) == non_terminal_set.end()) {
                return { true, ch };
            }
        }
        return { false, '\0' };
        };


    for (const auto& [non_terminal, rhs_list] : cfg.productions) {
        //foreach production

        std::vector<std::string> alpha_list, beta_list;
        std::set<std::string> new_alpha_list, new_beta_list;
        char new_non_terminal = '\0';

        for (const auto& rhs : rhs_list) {
            auto [is_left_recursion, _] = isLeftRecursion(non_terminal, rhs);
            // update alpha_list | beta_list
            is_left_recursion ? alpha_list.push_back(_) : beta_list.push_back(_);
        }

        // in this case, the production is a Single-Production, ignore it in this func.
        if (alpha_list.empty()) {
            cfg.productions[non_terminal] = rhs_list;
            continue;
        }

        // if we can reach here, means there must is at least a Left-Recursion occurs.
        auto [suc, _] = generateNewNonTerminal(cfg.non_terminals);
        if (!suc) {
            throw std::runtime_error("All non-terminal symbols are used, cannot generate a new one.");
        }
        //update a new non-terminal
        new_non_terminal = _;
        cfg.non_terminals.insert(new_non_terminal);

        //construct RHS part of A -> αA' ∣ ε
        for (const auto& alpha : alpha_list) {
            new_alpha_list.insert(alpha + std::string(1, new_non_terminal));
        }
        new_alpha_list.insert("@");

        // construct RHS part of  A -> βA'
        for (const auto& beta : beta_list) {
            new_beta_list.insert(beta + std::string(1, new_non_terminal));
        }

        // update this production
        cfg.productions[non_terminal] = std::vector<std::string>(new_beta_list.begin(), new_beta_list.end());//A -> βA'
        cfg.productions[new_non_terminal] = std::vector<std::string>(new_alpha_list.begin(), new_alpha_list.end());//A' -> αA' ∣ ε
    }

    return cfg;
}


// B.5 Transit it to Gribach Standard Form
// such as
// A -> Cca
// C -> cC
// all the rhs cannot start with non-terminal
CFG EliminateSingleLikeProduction(CFG& cfg) {

    bool flag = true;

    while (flag) {
        flag = false;
        std::set<char> lhs_invalid_set;

        for (auto& [non_terminal, rhs_list] : cfg.productions) {
            //foreach production

            for (auto& rhs : rhs_list) {
                //foreach part of rhs_list
                //check is there any rhs start with non-terminal
                if (isNonterminal(rhs[0])) {
                    // We must replace the Non-Terminal by its rhs_list
                    flag = true;
                    lhs_invalid_set.insert(non_terminal);
                }
            }
        }

        if (!flag) break;// if all productions are valid, break

        // if reach here, there are some productions are invalid
        for (auto& lhs_invalid : lhs_invalid_set) {
            std::vector<std::string> new_rhs_list;
            auto& invalid_rhs_list = cfg.productions[lhs_invalid];

            for (auto& rhs : invalid_rhs_list) {

                if (!isNonterminal(rhs[0])) {
                    // valid rhs part, pass
                    new_rhs_list.push_back(rhs);
                }
                else {
                    // invalid rhs part, handle it
                    // A -> Cca
                    // C -> cC | c
                    // rhs="Cca", rhs[0]='C'
                    // next_rhs_list = ["cC", "c"]
                    auto& next_rhs_list = cfg.productions[rhs[0]];
                    for (auto& _ : next_rhs_list) {
                        // constructed_string = "cCca" | "cca"
                        std::string constructed_string = _ + rhs.substr(1);

                        // A -> cCca | cca
                        new_rhs_list.push_back(constructed_string);
                    }
                }
            }

            cfg.productions[lhs_invalid] = new_rhs_list;

        }

    }

    return cfg;

}