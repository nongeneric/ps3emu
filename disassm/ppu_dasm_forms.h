#pragma once

#define START_FORM(name) struct name {
#define END_FORM(name) }; static_assert(sizeof(name) == 4, "form " #name " bad size");
#define FIELD(name, width) static constexpr int name##_bits = width; uint32_t name : width;

START_FORM(IForm)
    FIELD(LK, 1)
    FIELD(AA, 1)
    FIELD(LI, 24)
    FIELD(OPCD, 6)
END_FORM(IForm)

START_FORM(BForm)
    FIELD(LK, 1)
    FIELD(AA, 1)
    FIELD(BD, 14)
    FIELD(BI, 5)
    FIELD(BO, 5)
    FIELD(OPCD, 6)
END_FORM(BForm)

START_FORM(DForm_1)
    FIELD(D, 16)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
    END_FORM(DForm_1)

START_FORM(DForm_2)
    FIELD(SI, 16)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(DForm_2)

START_FORM(DForm_3)
    FIELD(D, 16)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(DForm_3)

START_FORM(DForm_4)
    FIELD(UI, 16)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
    END_FORM(DForm_4)

START_FORM(DForm_5)
    FIELD(SI, 16)
    FIELD(RA, 5)
    FIELD(L, 1)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(DForm_5)

START_FORM(DForm_6)
    FIELD(UI, 16)
    FIELD(RA, 5)
    FIELD(L, 1)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(DForm_6)

START_FORM(DForm_7)
    FIELD(SI, 16)
    FIELD(RA, 5)
    FIELD(TO, 5)
    FIELD(OPCD, 6)
END_FORM(DForm_7)

START_FORM(DForm_8)
    FIELD(D, 16)
    FIELD(RA, 5)
    FIELD(FRT, 5)
    FIELD(OPCD, 6)
END_FORM(DForm_8)

START_FORM(DForm_9)
    FIELD(D, 16)
    FIELD(RA, 5)
    FIELD(FRS, 5)
    FIELD(OPCD, 6)
END_FORM(DForm_9)

START_FORM(XLForm_1)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(BB, 5)
    FIELD(BA, 5)
    FIELD(BT, 5)
    FIELD(OPCD, 6)
END_FORM(XLForm_1)

START_FORM(XLForm_2)
    FIELD(LK, 1)
    FIELD(XO, 10)
    FIELD(BH, 2)
    FIELD(_, 3)
    FIELD(BI, 5)
    FIELD(BO, 5)
    FIELD(OPCD, 6)
END_FORM(XLForm_2)

START_FORM(XLForm_3)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 2)
    FIELD(BFA, 3)
    FIELD(____, 2)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(XLForm_3)

START_FORM(XLForm_4)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(____, 5)
    FIELD(OPCD, 6)
END_FORM(XLForm_4)

START_FORM(XForm_1)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_1)

START_FORM(XForm_2)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(NB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_2)

START_FORM(XForm_3)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(SR, 4)
    FIELD(___, 1)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_3)

START_FORM(XForm_4)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(__, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_4)

START_FORM(XForm_5)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_5)

START_FORM(XForm_6)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_6)

START_FORM(XForm_7)
    FIELD(_1, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_7)

START_FORM(XForm_8)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_8)

START_FORM(XForm_9)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(NB, 5)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_9)

START_FORM(XForm_10)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(SH, 5)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_10)

START_FORM(XForm_11)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(_, 5)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_11)

START_FORM(XForm_12)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(SR, 4)
    FIELD(___, 1)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_12)

START_FORM(XForm_13)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(__, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_13)

START_FORM(XForm_14)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_14)

START_FORM(XForm_15)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(L, 2)
    FIELD(___, 3)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_15)

START_FORM(XForm_16)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(L, 2)
    FIELD(__, 1)
    FIELD(BF, 2)
    FIELD(OPCD, 6)
END_FORM(XForm_16)

START_FORM(XForm_17)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(FRB, 5)
    FIELD(FRA, 5)
    FIELD(__, 2)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_17)

START_FORM(XForm_18)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(BFA, 3)
    FIELD(___, 2)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_18)

START_FORM(XForm_19)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(_, 1)
    FIELD(U, 4)
    FIELD(__, 5)
    FIELD(___, 2)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_19)

START_FORM(XForm_20)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(____, 2)
    FIELD(BF, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_20)

START_FORM(XForm_21)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(TH, 4)
    FIELD(__, 1)
    FIELD(OPCD, 6)
END_FORM(XForm_21)

START_FORM(XForm_22)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(L, 2)
    FIELD(__, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_22)

START_FORM(XForm_23)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(__, 5)
    FIELD(L, 2)
    FIELD(___, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_23)

START_FORM(XForm_24)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(L, 2)
    FIELD(____, 3)
    FIELD(OPCD, 6)
END_FORM(XForm_24)

START_FORM(XForm_25)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(TO, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_25)

START_FORM(XForm_26)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(FRT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_26)

START_FORM(XForm_27)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(FRB, 5)
    FIELD(_, 5)
    FIELD(FRT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_27)

START_FORM(XForm_28)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(FRS, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_28)

START_FORM(XForm_29)
    FIELD(Rc, 1)
    FIELD(XO, 10)
    FIELD(_, 5)
    FIELD(__, 5)
    FIELD(BT, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_29)

START_FORM(XForm_30)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(__, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_30)

START_FORM(XForm_31)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(RB, 5)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_31)

START_FORM(XForm_32)
    FIELD(_, 1)
    FIELD(XO, 10)
    FIELD(__, 5)
    FIELD(___, 5)
    FIELD(____, 5)
    FIELD(OPCD, 6)
END_FORM(XForm_32)

START_FORM(DSForm_1)
    FIELD(XO, 2)
    FIELD(DS, 14)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(DSForm_1)

START_FORM(DSForm_2)
    FIELD(XO, 2)
    FIELD(DS, 14)
    FIELD(RA, 5)
    FIELD(RS, 5)
    FIELD(OPCD, 6)
END_FORM(DSForm_2)

START_FORM(XOForm_1)
    FIELD(Rc, 1)
    FIELD(XO, 9)
    FIELD(OE, 1)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XOForm_1)

START_FORM(XOForm_2)
    FIELD(Rc, 1)
    FIELD(XO, 9)
    FIELD(_, 1)
    FIELD(RB, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XOForm_2)

START_FORM(XOForm_3)
    FIELD(Rc, 1)
    FIELD(XO, 9)
    FIELD(OE, 1)
    FIELD(_, 5)
    FIELD(RA, 5)
    FIELD(RT, 5)
    FIELD(OPCD, 6)
END_FORM(XOForm_3)