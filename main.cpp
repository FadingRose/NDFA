#include "cfg.hpp"
#include "automatic.hpp"
#include<string>

int main() {
    CFG cfg = Read_CFG_File("File1.txt"); // Assuming the CFG is stored in File1.txt

    cfg = EliminateUselessSymbols(cfg);
    cfg = EliminateIndirectLeftRecursion(cfg);
    cfg = EliminateDirectLeftRecursion(cfg);
    cfg = EliminateSingleProduction(cfg);
    cfg = EliminateEpsilonProductions(cfg);
    cfg = EliminateSingleLikeProduction(cfg);



    Output_CFG_Struct(cfg);

    std::string input_string = "aabccac";
    NPFA npfa = NPFA(cfg);


    if (npfa.Execute_NPDA(input_string)) {
        std::cout << "ACCEPT." << std::endl;
    }
    else {
        std::cout << "REJECT." << std::endl;
    }


    return 0;
}