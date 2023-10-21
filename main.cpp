#include "cfg.hpp"
#include "automatic.hpp"

int main() {
    CFG cfg = Read_CFG_File("File1.txt"); // Assuming the CFG is stored in File1.txt
    cfg = EliminateUselessSymbols(cfg);
    cfg = EliminateIndirectLeftRecursion(cfg);
    cfg = EliminateDirectLeftRecursion(cfg);
    cfg = EliminateSingleProduction(cfg);
    cfg = EliminateEpsilonProductions(cfg);

    Output_CFG_Struct(cfg);

    return 0;
}