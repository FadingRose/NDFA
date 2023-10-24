#include<vector>
#include<string>
#include<stack>
#include<set>
#include<map>

typedef char State;
typedef char Symbol;

//[state, input_symbol, top_stack_symbol] -> [next state, symbols pushed into stack]
using TransitionFunction = std::multimap<std::tuple<State, Symbol, Symbol>, std::tuple<State, std::string>>;

// struct CFG {
//     std::set<char> non_terminals;
//     std::set<char> terminals;
//     std::map<char, std::vector<std::string>> productions;
//     char start_symbol;
//     char epsilon_symbol = '@';
// };

struct NPFA {
    std::stack<Symbol> st;
    TransitionFunction delta_map;
    Symbol start_symbol;

    NPFA(const CFG& cfg) {
        // Transition: left->right

        // For a spefiic production's specific rhs
        // Example::
        // S -> aAbBC | abBC
        // [q, a, S] => [q, AbBC]
        // [q, a, S] => [q, bBC]
        //
        // A -> a
        // [q, A, a] => [q, @]
        //
        // left_constructor => right_constructor
        auto left_constructor = [&cfg](const char non_terminal, const std::string rhs)->std::tuple<State, Symbol, Symbol> {
            return { 'q', rhs[0], non_terminal };
            };
        auto right_constructor = [&cfg](const char non_terminal, const std::string rhs)->std::tuple<State, std::string > {
            if (rhs.substr(1).empty()) {
                return { 'q', "@" };
            }
            else {
                return { 'q', rhs.substr(1) };
            }
            };

        // productions -> delta transition
        for (const auto& [non_terminal, rhs_list] : cfg.productions) {
            for (const auto& rhs : rhs_list) {
                auto left = left_constructor(non_terminal, rhs);
                auto right = right_constructor(non_terminal, rhs);

                delta_map.insert({ left,right });

            }
        }

        //Initiate the start_symbol
        start_symbol = cfg.start_symbol;
    };

    // For a specific input_string
    // Execute the NPDA
    // return ACCEPT or REJECT
    bool Execute_NPDA(const std::string input_string) {
        // Step 1 Initiate the stack
        // We push the start_symbol into stack
        st.push(start_symbol);

        // Step 2 Execute DFS
        return DFS(st, input_string, 0);

    };

    // for a input_string, we need to execute DFS to find all of the available paths.
    // parameters::
    //  - a copy of a symbol stack
    //  - full input_string
    //  - index of ch
    bool DFS(std::stack<Symbol> st_cur, const std::string& input_string, int _ch) {
        char ch = input_string[_ch];
        // Step 1.1
        // if the stack is Empty() -> ACCEPT, otherwise -> REJECT
        if (_ch >= input_string.length()) {
            while (st_cur.top() == '@') {
                st_cur.pop();
            }
            return st_cur.empty();
        }


        // Step 1.1 
        // if the top_stack_symbol is '@'
        // pop it untill::
        // a. meet a symbol -> CONTINUE
        // b. stack.empty() -> ACCEPT
        if (st_cur.top() == '@') {
            while (st_cur.top() == '@') {
                st_cur.pop();
                if (st_cur.empty()) return true; // -> ACCEPT
            }
        }

        // meet a symbol -> CONTINUE

        // Step 1.3 if the stack.top() is a Terminal, match it with ch
        // if stack.top() == ch, pop it, _ch++, untill meeting a Non-Terminal or stack.empty(), if reach empty, ACCEPT
        // if stack.top() != ch, REJECT

        if (!isNonterminal(st_cur.top())) {
            while (!isNonterminal(st_cur.top())) {
                if (ch != st_cur.top()) {
                    return false;// terminal not match -> REJECT
                }

                _ch++;
                st_cur.pop();

                if (_ch == input_string.length() && st_cur.empty()) {
                    return true;// all terminal matched and stack is empty -> ACCEPT
                }

                if (_ch >= input_string.length() && !st_cur.empty()) {
                    return false;// if reach the end of input_string but stack is not empty -> REJECT
                }

                ch = input_string[_ch];
            }


        }

        // if we reach here, stack.top() is a Non-Terminal
        // Check the delta_list

        // delta ::
        // //[state, input_symbol, top_stack_symbol] -> [next state, symbols pushed into stack]
        // we know that,
        //  state == 'q'         //  input_symbol == ch
        //  top_stack_symbol == st.top()


        // Step 2.1 if there are no matched transition
        // REJECT
        if (delta_map.find({ 'q',ch,st_cur.top() }) == delta_map.end()) {
            return false;
        }

        // Step 2.2 if we can reach hear, that means there is at least one transition is matched
        // But for a specific Left-Hand, there are may more RHSs, as follow shows:
        //       S -> aAbBC | abBC
        //       {'q', a, S}-> {'q', AbBC} | {'q', bBC}

        std::tuple<State, Symbol, Symbol> left = { 'q', ch, st_cur.top() };
        const auto& lower = delta_map.lower_bound(left);
        const auto& upper = delta_map.upper_bound(left);

        // for all of the RHSs, DFS
        for (auto it = lower;it != upper;++it) {
            // foreach RHS, we build a new stack by push the different rhs in it
            // before enter next dfs, we provide::
            // - a new stack
            auto& _ = it->second;
            auto& _string = std::get<1>(_);
            const auto& construct_new_stack = [&](const std::string& rhs) -> std::stack<Symbol> {
                std::stack<Symbol> new_st = st_cur;
                new_st.pop();
                for (int i = rhs.length() - 1;i >= 0;--i) {
                    new_st.push(rhs[i]);
                }
                return new_st;
                };
            const auto& new_st = construct_new_stack(_string);

            if (DFS(new_st, input_string, _ch + 1)) {
                // one of the available branch is ACCEPTED
                // ACCEPTED
                return true;
            }
        }

        // all of the available branch is REJECT
        return false;

    }
};

