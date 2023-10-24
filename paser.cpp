#include<string>
#include<vector>
#include<type_traits>
#include<variant>

using Terminal = char;
using Nonterminal = std::string;

template <typename T>
struct Token {
    std::string token_type;// Terminal | Nonterminal
    T token_content;

    Token(T content) {
        token_content = content;

        if (std::is_same<T, char>::value) {
            token_type = Terminal;
            token_content = ;
        }
        else if (std::is_same<T, std::string>::value) {
            token_type = Nonterminal;
            token_content = {}
        }
        else {
            token_type = "Unknown";
        }
    }
};

using AnyToken = std::variant<Token<Terminal>, Token<Nonterminal>>;

std::vector<AnyToken> Parser(std::string rhs) {
    std::vector<AnyToken> token_list;
    // lower_case is a [Terminal] :: a b c
    // upper_case (with a num) is a Nonterminal :: A, A1

    auto isLowerCase = [&](char c) ->bool {
        if (c >= 'a' && c <= 'z') return true;
        else return false;
        };
    auto isNum = [&](char c) ->bool {
        if (c >= '0' && c <= '9') return true;
        else return false;
        };

    for (auto& it : rhs) {
        if (isLowerCase(rhs[it])) {
            //new token
            Terminal terminal = rhs[it];
            Token tk = Token(rhs[it]);
            token_list.push_back(tk);
        }
        else {
            //new token
            std::string ct = { rhs[it] };
            it++;

            while (isNum(it)) {
                ct += rhs[it];
                it++;
            }

            Token tk = Token(ct);
        }

    }


    return token_list;
}