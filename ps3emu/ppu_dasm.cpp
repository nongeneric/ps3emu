#include "ppu_dasm.h"

BranchMnemonicType getExtBranchMnemonic(
    bool lr, bool abs, bool tolr, bool toctr, BO_t btbo, BI_t bi, std::string& mnemonic)
{
    auto type = BranchMnemonicType::Generic;
    auto bo = btbo.u();
    // see fig 21
    auto at = (bo >> 2 == 1 || bo >> 2 == 3) ? bo & 3
            : ((bo & 22) == 16 || (bo & 22) == 18) ? (((bo >> 2) & 2) | (bo & 1))
            : 0;
    auto atchar = at == 2 ? "-" : at == 3 ? "+" : "";

    std::string m;
    if (bo >> 2 == 3 || bo >> 2 == 1) { // table 4
        bool crbiSet = ((bo >> 2) & 3) == 3;
        const char* f;
        switch (bi.u() % 4) {
        case 0:
            f = crbiSet ? "lt" : "ge";
            break;
        case 1:
            f = crbiSet ? "gt" : "le";
            break;
        case 2:
            f = crbiSet ? "eq" : "ne";
            break;
        case 3:
            f = crbiSet ? "so" : "ns";
        }

        m = ssnprintf("b%s%s%s%s%s",
                      f,
                      tolr ? "lr" : "",
                      toctr ? "ctr" : "",
                      lr ? "l" : "",
                      abs ? "a" : "");
        type = BranchMnemonicType::ExtCondition;
    } else { // table 3
        if ((bo & 20) == 20) { // branch unconditionally
            if (lr) {
                if (tolr) m = "blrl";
                if (toctr) m = "bctrl";
            } else {
                if (tolr) m = "blr";
                if (toctr) m = "bctr";
            }
            if (tolr || toctr) {
                type = BranchMnemonicType::ExtSimple;
            }
        }
        // "Branch if CR_BI=1|0" overlaps with table 4 and table 4 is preferred
        else if (!toctr) {
            const char* sem;
            if ((bo & 22) == 16) {
                sem = "nz";
            } else if ((bo & 22) == 18) {
                sem = "z";
            } else if ((bo >> 1) == 4) {
                sem = "nzt";
            } else if ((bo >> 1) == 5) {
                sem = "zt";
            } else if ((bo >> 1) == 0) {
                sem = "nzf";
            } else if ((bo >> 1) == 1) {
                sem = "zf";
            } else {
                throw std::runtime_error("invalid instruction");
            }
            m = ssnprintf("bd%s%s%s%s",
                          sem,
                          tolr ? "lr" : "",
                          lr ? "l" : "",
                          abs ? "a" : "");
            type = BranchMnemonicType::ExtSimple;
        }
    }
    if (type != BranchMnemonicType::Generic) {
        mnemonic = m + atchar;
    }
    return type;
}

std::string formatCRbit(BI_t bi) {
    const char* crbit;
    switch (bi.u() % 4) {
    case 0:
        crbit = "lt";
        break;
    case 1:
        crbit = "gt";
        break;
    case 2:
        crbit = "eq";
        break;
    case 3:
        crbit = "so";
    }
    return ssnprintf("4*cr%d+%s", bi.u() / 4, crbit);
}
