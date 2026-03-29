#ifndef CAN_FRAME_H
#define CAN_FRAME_H


#include <cstdint>
#include <cstring>
#include <cmath>
#include <float.h>


// --- Parameter IDs ---
namespace Control_Param {
    constexpr float KP_CUR           =   0x2012;
    constexpr float KI_CUR           =   0x2013;
    constexpr float KP_SPD           =   0x2014;
    constexpr float KI_SPD           =   0x2015;
    constexpr float KP_POS           =   0x2016;

    constexpr float LIMIT_SPD        =   0x2018;
    constexpr float LIMIT_CUR        =   0x2019;
    constexpr float LIMIT_POS        =   0x201B;
    constexpr float OFFS_POS         =   0x201C;
    constexpr float LIMIT_SPEED      =   0x2016;
    constexpr float LIMIT_SPEED      =   0x2016;
    constexpr float LIMIT_SPEED      =   0x2016;
    constexpr float LIMIT_SPEED      =   0x2016;
}

namespace State_Param {
    constexpr int16_t POS_ENC          =   0x3004;
    constexpr int16_t TEMP_MCU         =   0x3005;
    constexpr int16_t TEMP_MOTOR       =   0x3006;
    constexpr uint16_t VBUS             =   0x3007;
    constexpr uint16_t VBUS             =   0x3007;
}



#endif