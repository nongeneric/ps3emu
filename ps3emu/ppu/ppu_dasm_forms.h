#pragma once

#include "ps3emu/BitField.h"

struct FormBase {
    uint32_t value{};
};

struct IForm : public FormBase {
    BIT_FIELD(OPCD, 0, 6)
    BIT_FIELD(LI, 6, 30, BitFieldType::Signed, 2)
    BIT_FIELD(AA, 30, 31)
    BIT_FIELD(LK, 31, 32)
};

struct BForm : public FormBase {
    BIT_FIELD(BO, 6, 11)
    BIT_FIELD(BI, 11, 16)
    BIT_FIELD(BD, 16, 30, BitFieldType::Signed, 2)
    BIT_FIELD(AA, 30, 31)
    BIT_FIELD(LK, 31, 32)
    BIT_FIELD(BO0, 6, 7)
    BIT_FIELD(BO1, 7, 8)
    BIT_FIELD(BO2, 8, 9)
    BIT_FIELD(BO3, 9, 10)
};

struct XLForm_1 : public FormBase {
    BIT_FIELD(BT, 6, 11)
    BIT_FIELD(BA, 11, 16)
    BIT_FIELD(BB, 16, 21)
    BIT_FIELD(XO, 21, 31)
};

struct XLForm_2 : public FormBase {
    BIT_FIELD(BO, 6, 11)
    BIT_FIELD(BI, 11, 16)
    BIT_FIELD(BH, 19, 21)
    BIT_FIELD(LK, 31, 32)

    BIT_FIELD(BO0, 6, 7)
    BIT_FIELD(BO1, 7, 8)
    BIT_FIELD(BO2, 8, 9)
    BIT_FIELD(BO3, 9, 10)
};

struct XLForm_3 : public FormBase {
    BIT_FIELD(BF, 6, 9, BitFieldType::CR)
    BIT_FIELD(BFA, 11, 14, BitFieldType::CR)
};

struct XForm_1 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(XO, 21, 31)
};

struct XForm_6 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(Rc, 31, 32)
};

struct XForm_8 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XForm_10 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(SH, 16, 21)
    BIT_FIELD(Rc, 31, 32)
};

struct XForm_11 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(Rc, 31, 32)
};

struct XForm_16 : public FormBase {
    BIT_FIELD(BF, 6, 9, BitFieldType::CR)
    BIT_FIELD(L, 10, 11)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XForm_17 : public FormBase {
    BIT_FIELD(BF, 6, 9, BitFieldType::CR)
    BIT_FIELD(FRA, 11, 16, BitFieldType::FPR)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
};

struct XForm_24 : public FormBase {
    BIT_FIELD(L, 9, 11)
};

struct XForm_25 : public FormBase {
    BIT_FIELD(TO, 6, 11)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XForm_26 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XForm_27 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct XForm_28 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct XForm_29 : public FormBase {
    BIT_FIELD(FRS, 6, 11, BitFieldType::FPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XForm_31 : public FormBase {
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
};

struct XFLForm : public FormBase {
    BIT_FIELD(FLM, 7, 15)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct DForm_1 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(D, 16, 32, BitFieldType::Signed)
};

struct DForm_2 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(SI, 16, 32, BitFieldType::Signed)
};

struct DForm_3 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(D, 16, 32, BitFieldType::Signed)
};

struct DForm_4 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(UI, 16, 32, BitFieldType::Unsigned)
};

struct DForm_5 : public FormBase {
    BIT_FIELD(BF, 6, 9, BitFieldType::CR)
    BIT_FIELD(L, 10, 11)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(SI, 16, 32, BitFieldType::Signed)
};

struct DForm_6 : public FormBase {
    BIT_FIELD(BF, 6, 9, BitFieldType::CR)
    BIT_FIELD(L, 10, 11)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(UI, 16, 32, BitFieldType::Unsigned)
};

struct DSForm_1 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(DS, 16, 30, BitFieldType::Signed, 2)
    BIT_FIELD(XO, 30, 32)
};

struct DSForm_2 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(DS, 16, 30, BitFieldType::Signed, 2)
    BIT_FIELD(XO, 30, 32)
};

struct DForm_8 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(D, 16, 32, BitFieldType::Signed)
};

struct DForm_9 : public FormBase {
    BIT_FIELD(FRS, 6, 11, BitFieldType::FPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(D, 16, 32, BitFieldType::Signed)
};

struct XOForm_1 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(OE, 21, 22)
    BIT_FIELD(XO, 22, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct XOForm_3 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(OE, 21, 22)
    BIT_FIELD(Rc, 31, 32)
};

struct XFXForm_1 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(spr, 11, 21)
    BIT_FIELD(XO, 21, 31)
};

struct XFXForm_2 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(tbr, 11, 21)
};

struct XFXForm_3 : public FormBase {
    BIT_FIELD(RT, 6, 11, BitFieldType::GPR)
    BIT_FIELD(_0, 11, 12)
};

struct XFXForm_5 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(FXM, 12, 20)
};

struct XFXForm_7 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(spr, 11, 21)
};

struct XFXForm_6 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(_1, 11, 12)
    BIT_FIELD(FXM, 12, 20)
};

struct MDForm_1 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(sh04, 16, 21)
    BIT_FIELD(mb04, 21, 26)
    BIT_FIELD(mb5, 26, 27)
    BIT_FIELD(XO, 27, 30)
    BIT_FIELD(sh5, 30, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct MDForm_2 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(sh04, 16, 21)
    BIT_FIELD(me04, 21, 26)
    BIT_FIELD(me5, 26, 27)
    BIT_FIELD(sh5, 30, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct MDSForm_1 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(mb, 21, 27)
    BIT_FIELD(XO, 27, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct MDSForm_2 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(me, 21, 27)
    BIT_FIELD(XO, 27, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct MForm_1 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(RB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(mb, 21, 26)
    BIT_FIELD(me, 26, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct MForm_2 : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(SH, 16, 21)
    BIT_FIELD(mb, 21, 26)
    BIT_FIELD(me, 26, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct SCForm : public FormBase {
    BIT_FIELD(LEV, 20, 27)
    BIT_FIELD(_1, 30, 31)
};

struct NCallForm : public FormBase {
    BIT_FIELD(idx, 6, 32)
};

struct XSForm : public FormBase {
    BIT_FIELD(RS, 6, 11, BitFieldType::GPR)
    BIT_FIELD(RA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(sh04, 16, 21)
    BIT_FIELD(XO, 21, 30)
    BIT_FIELD(sh5, 30, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct AForm_1 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(FRA, 11, 16, BitFieldType::FPR)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
    BIT_FIELD(FRC, 21, 26, BitFieldType::FPR)
    BIT_FIELD(XO, 26, 31)
    BIT_FIELD(Rc, 31, 32)
};

struct AForm_2 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(FRA, 11, 16, BitFieldType::FPR)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct AForm_3 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(FRA, 11, 16, BitFieldType::FPR)
    BIT_FIELD(FRC, 21, 26, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct AForm_4 : public FormBase {
    BIT_FIELD(FRT, 6, 11, BitFieldType::FPR)
    BIT_FIELD(FRB, 16, 21, BitFieldType::FPR)
    BIT_FIELD(Rc, 31, 32)
};

struct SIMDForm : public FormBase {
    BIT_FIELD(rA, 11, 16, BitFieldType::GPR)
    BIT_FIELD(rB, 16, 21, BitFieldType::GPR)
    BIT_FIELD(Rc, 21, 22)
    BIT_FIELD(vA, 11, 16, BitFieldType::Vector)
    BIT_FIELD(vB, 16, 21, BitFieldType::Vector)
    BIT_FIELD(vC, 21, 26, BitFieldType::Vector)
    BIT_FIELD(vD, 6, 11, BitFieldType::Vector)
    BIT_FIELD(vS, 6, 11, BitFieldType::Vector)
    BIT_FIELD(SHB, 22, 26, BitFieldType::Unsigned)
    BIT_FIELD(SIMM, 11, 16, BitFieldType::Signed)
    BIT_FIELD(UIMM, 11, 16)
    BIT_FIELD(UIMM3, 13, 16)
    BIT_FIELD(UIMM2, 14, 16)
    BIT_FIELD(VA_XO, 26, 32)
    BIT_FIELD(VX_XO, 21, 32)
    BIT_FIELD(VXR_XO, 22, 32)
};
