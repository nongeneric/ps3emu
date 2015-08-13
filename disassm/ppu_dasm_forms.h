#pragma once

#include "BitField.h"

union IForm {
    BitField<0, 6> OPCD;
    BitField<6, 30> LI;
    BitField<30, 31> AA;
    BitField<31, 32> LK;
};

union BForm {
    BitField<6, 11> BO;
    BitField<11, 16> BI;
    BitField<16, 30> BD;
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
    BitField<6, 11> BO;
    BitField<11, 16> BI;
    BitField<19, 21> BH;
    BitField<31, 32> LK;
    
    BitField<6, 7> BO0;
    BitField<7, 8> BO1;
    BitField<8, 9> BO2;
    BitField<9, 10> BO3;
};

union XLForm_3 {
    BitField<6, 9> BF;
    BitField<11, 14> BFA;
};

union XForm_1 {
    BitField<6, 11> RT;
    BitField<11, 16> RA;
    BitField<16, 21> RB;
    BitField<21, 31> XO;
};

union XForm_6 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 21> RB;
    BitField<31, 32> Rc;
};

union XForm_8 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 21> RB;
};

union XForm_11 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<31, 32> Rc;
};

union DForm_1 {
    BitField<6, 11> RT;
    BitField<11, 16> RA;
    BitField<16, 32> D;
};

union DForm_2 {
    BitField<6, 11> RT;
    BitField<11, 16> RA;
    BitField<16, 32> SI;
};

union DForm_3 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 32> D;
};

union DForm_4 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 32> UI;
};

union DSForm_1 {
    BitField<6, 11> RT;
    BitField<11, 16> RA;
    BitField<16, 30> DS;
    BitField<30, 32> XO;
};

union DSForm_2 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 30> DS;
    BitField<30, 32> XO;
};

union XOForm_1 {
    BitField<6, 11> RT;
    BitField<11, 16> RA;
    BitField<16, 21> RB;
    BitField<21, 22> OE;
    BitField<22, 31> XO;
    BitField<31, 32> Rc;
};

union XFXForm_1 {
    BitField<6, 11> RT;
    BitField<11, 21> spr;
    BitField<21, 31> XO;
};

union XFXForm_5 {
    BitField<6, 11> RS;
    BitField<12, 19> FXM;
};

union XFXForm_7 {
    BitField<6, 11> RS;
    BitField<11, 21> spr;
};

union MDForm_1 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 21> sh04;
    BitField<21, 26> mb04;
    BitField<26, 27> mb5;
    BitField<27, 30> XO;
    BitField<30, 31> sh5;
    BitField<31, 32> Rc;
};

union MDForm_2 {
    BitField<6, 11> RS;
    BitField<11, 16> RA;
    BitField<16, 21> sh04;
    BitField<21, 26> me04;
    BitField<26, 27> me5;
    BitField<30, 31> sh5;
    BitField<31, 32> Rc;
};