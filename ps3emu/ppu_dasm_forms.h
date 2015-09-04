#pragma once

#include "BitField.h"

typedef BitField<16, 30, BitFieldType::Signed, 2> BD_t;
typedef BitField<6, 9, BitFieldType::CR> BF_t;
typedef BitField<11, 13, BitFieldType::CR> BFA_t;
typedef BitField<16, 32, BitFieldType::Signed> D_t;
typedef BitField<16, 30, BitFieldType::Signed, 2> DS_t;
typedef BitField<6, 30, BitFieldType::Signed, 2> LI_t;
typedef BitField<11, 16, BitFieldType::GPR> RA_t;
typedef BitField<16, 21, BitFieldType::GPR> RB_t;
typedef BitField<6, 11, BitFieldType::GPR> RS_t;
typedef BitField<6, 11, BitFieldType::GPR> RT_t;
typedef BitField<16, 32, BitFieldType::Signed> SI_t;
typedef BitField<16, 32, BitFieldType::Unsigned> UI_t;
typedef BitField<6, 11> BO_t;
typedef BitField<11, 16> BI_t;
typedef BitField<12, 20> FXM_t;
typedef BitField<11, 16, BitFieldType::FPR> FRA_t;
typedef BitField<16, 21, BitFieldType::FPR> FRB_t;
typedef BitField<21, 26, BitFieldType::FPR> FRC_t;
typedef BitField<6, 11, BitFieldType::FPR> FRS_t;
typedef BitField<6, 11, BitFieldType::FPR> FRT_t;
typedef BitField<31, 32> Rc_t;
typedef BitField<7, 15> FLM_t;

union IForm {
    BitField<0, 6> OPCD;
    LI_t LI;
    BitField<30, 31> AA;
    BitField<31, 32> LK;
};

union BForm {
    BO_t BO;
    BI_t BI;
    BD_t BD;
    BitField<30, 31> AA;
    BitField<31, 32> LK;
    
    BitField<6, 7> BO0;
    BitField<7, 8> BO1;
    BitField<8, 9> BO2;
    BitField<9, 10> BO3;
};

union XLForm_1 {
    BitField<6, 11> BT;
    BitField<11, 16> BA;
    BitField<16, 21> BB;
    BitField<21, 31> XO;
};

union XLForm_2 {
    BO_t BO;
    BI_t BI;;
    BitField<19, 21> BH;
    BitField<31, 32> LK;
    
    BitField<6, 7> BO0;
    BitField<7, 8> BO1;
    BitField<8, 9> BO2;
    BitField<9, 10> BO3;
};

union XLForm_3 {
    BF_t BF;
    BFA_t BFA;
};

union XForm_1 {
    RT_t RT;
    RA_t RA;
    RB_t RB;
    BitField<21, 31> XO;
};

union XForm_6 {
    RS_t RS;
    RA_t RA;
    RB_t RB;
    Rc_t Rc;
};

union XForm_8 {
    RS_t RS;
    RA_t RA;
    RB_t RB;
};

union XForm_10 {
    RS_t RS;
    RA_t RA;
    BitField<16, 21> SH;
    Rc_t Rc;
};

union XForm_11 {
    RS_t RS;
    RA_t RA;
    Rc_t Rc;
};

union XForm_16 {
    BF_t BF;
    BitField<10, 11> L;
    RA_t RA;
    RB_t RB;
};

union XForm_17 {
    BF_t BF;
    FRA_t FRA;
    FRB_t FRB;
};

union XForm_26 {
    FRT_t FRT;
    RA_t RA;
    RB_t RB;
};

union XForm_27 {
    FRT_t FRT;
    FRB_t FRB;
    Rc_t Rc;
};

union XForm_28 {
    FRT_t FRT;
    Rc_t Rc;
};

union XForm_29 {
    FRT_t FRS;
    RA_t RA;
    RB_t RB;
};

union XForm_31 {
    RA_t RA;
    RB_t RB;
};

union XFLForm {
    FLM_t FLM;
    FRB_t FRB;
    Rc_t Rc;
};

union DForm_1 {
    RT_t RT;
    RA_t RA;
    D_t D;
};

union DForm_2 {
    RT_t RT;
    RA_t RA;
    SI_t SI;
};

union DForm_3 {
    RS_t RS;
    RA_t RA;
    D_t D;
};

union DForm_4 {
    RS_t RS;
    RA_t RA;
    UI_t UI;
};

union DForm_5 {
    BF_t BF;
    BitField<10, 11> L;
    RA_t RA;
    SI_t SI;
};

union DForm_6 {
    BF_t BF;
    BitField<10, 11> L;
    RA_t RA;
    UI_t UI;
};

union DSForm_1 {
    RT_t RT;
    RA_t RA;
    DS_t DS;
    BitField<30, 32> XO;
};

union DSForm_2 {
    RS_t RS;
    RA_t RA;
    DS_t DS;
    BitField<30, 32> XO;
};

union DForm_8 {
    FRT_t FRT;
    RA_t RA;
    D_t D;
};

union DForm_9 {
    FRS_t FRS;
    RA_t RA;
    D_t D;
};

union XOForm_1 {
    RT_t RT;
    RA_t RA;
    RB_t RB;
    BitField<21, 22> OE;
    BitField<22, 31> XO;
    Rc_t Rc;
};

union XOForm_3 {
    RT_t RT;
    RA_t RA;
    BitField<21, 22> OE;
    Rc_t Rc;
};


union XFXForm_1 {
    RT_t RT;
    BitField<11, 21> spr;
    BitField<21, 31> XO;
};

union XFXForm_3 {
    RT_t RT;
    BitField<11, 12> _0;
};

union XFXForm_5 {
    RS_t RS;
    FXM_t FXM;
};

union XFXForm_7 {
    RS_t RS;
    BitField<11, 21> spr;
};

union XFXForm_6 {
    RS_t RS;
    BitField<11, 12> _1;
    FXM_t FXM;
};

union MDForm_1 {
    RS_t RS;
    RA_t RA;
    BitField<16, 21> sh04;
    BitField<21, 26> mb04;
    BitField<26, 27> mb5;
    BitField<27, 30> XO;
    BitField<30, 31> sh5;
    Rc_t Rc;
};

union MDForm_2 {
    RS_t RS;
    RA_t RA;
    BitField<16, 21> sh04;
    BitField<21, 26> me04;
    BitField<26, 27> me5;
    BitField<30, 31> sh5;
    Rc_t Rc;
};

union MDSForm_1 {
    RS_t RS;
    RA_t RA;
    RB_t RB;
    BitField<21, 27> mb;
    BitField<27, 31> XO;
    Rc_t Rc;
};

union MDSForm_2 {
    RS_t RS;
    RA_t RA;
    RB_t RB;
    BitField<21, 27> me;
    BitField<27, 31> XO;
    Rc_t Rc;
};

union MForm_1 {
    RS_t RS;
    RA_t RA;
    RB_t RB;
    BitField<21, 26> mb;
    BitField<26, 31> me;
    Rc_t Rc;
};

union MForm_2 {
    RS_t RS;
    RA_t RA;
    BitField<16, 21> SH;
    BitField<21, 26> mb;
    BitField<26, 31> me;
    Rc_t Rc;
};

union SCForm {
    BitField<20, 27> LEV;
    BitField<30, 31> _1;
};

union NCallForm {
    BitField<6, 32> idx;
};

union XSForm {
    RS_t RS;
    RA_t RA;
    BitField<16, 21> sh04;
    BitField<21, 30> XO;
    BitField<30, 31> sh5;
    Rc_t Rc;
};

union AForm_1 {
    FRT_t FRT;
    FRA_t FRA;
    FRB_t FRB;
    FRC_t FRC;
    BitField<26, 31> XO;
    Rc_t Rc;
};

union AForm_2 {
    FRT_t FRT;
    FRA_t FRA;
    FRB_t FRB;
    Rc_t Rc;
};

union AForm_3 {
    FRT_t FRT;
    FRA_t FRA;
    FRC_t FRC;
    Rc_t Rc;
};

union AForm_4 {
    FRT_t FRT;
    FRB_t FRB;
    Rc_t Rc;
};