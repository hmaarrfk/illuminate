/*
   Copyright (c) 2018, Zachary Phillips (UC Berkeley)
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
      Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
      Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
      Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL ZACHARY PHILLIPS (UC BERKELEY) BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "illuminate.h"
#ifdef USE_SCI_WING_ARRAY
#include "ledarrayinterface.h"
#include "src/TLC5955/TLC5955.h"

// Pin definitions (used internally)
#define GSCLK 6 // 10 on Arduino Mega
#define LAT 3   // 44 on Arduino Mega
#define SPI_MOSI 11
#define SPI_CLK 13
#define TRIGGER_OUTPUT_PIN_0 23
#define TRIGGER_OUTPUT_PIN_1 22
#define TRIGGER_INPUT_PIN_0 20
#define TRIGGER_INPUT_PIN_1 19
#define TRIGGER_OUTPUT_COUNT 2
#define TRIGGER_INPUT_COUNT 2

// Device and Software Descriptors
const char * LedArrayInterface::device_name = "sci-wing";
const char * LedArrayInterface::device_hardware_revision = "1.0";
const float LedArrayInterface::max_na = 1.0;
const int16_t LedArrayInterface::led_count = 793;
const uint16_t LedArrayInterface::center_led = 0;
const int LedArrayInterface::trigger_output_count = 2;
const int LedArrayInterface::trigger_input_count = 2;
const int LedArrayInterface::color_channel_count = 3;
const char LedArrayInterface::color_channel_names[] = {'r', 'g', 'b'};
const float LedArrayInterface::color_channel_center_wavelengths[] = {0.48, 0.525, 0.625};
const int LedArrayInterface::bit_depth = 16;
const int16_t LedArrayInterface::tlc_chip_count = 52;
const bool LedArrayInterface::supports_fast_sequence = false;
const float LedArrayInterface::led_array_distance_z_default = 50.0;

const int LedArrayInterface::trigger_output_pin_list[] = {TRIGGER_OUTPUT_PIN_0, TRIGGER_OUTPUT_PIN_1};
const int LedArrayInterface::trigger_input_pin_list[] = {TRIGGER_INPUT_PIN_0, TRIGGER_INPUT_PIN_1};
bool LedArrayInterface::trigger_input_state[] = {false, false};

const uint8_t TLC5955::_tlc_count = 52;    // Change to reflect number of TLC chips
float TLC5955::max_current_amps = 8.0;      // Maximum current output, amps
bool TLC5955::enforce_max_current = true;   // Whether to enforce max current limit

// Define dot correction, pin rgb order, and grayscale data arrays in program memory
uint8_t TLC5955::_dc_data[TLC5955::_tlc_count][TLC5955::LEDS_PER_CHIP][TLC5955::COLOR_CHANNEL_COUNT];
uint8_t TLC5955::_rgb_order[TLC5955::_tlc_count][TLC5955::LEDS_PER_CHIP][TLC5955::COLOR_CHANNEL_COUNT];
uint16_t TLC5955::_grayscale_data[TLC5955::_tlc_count][TLC5955::LEDS_PER_CHIP][TLC5955::COLOR_CHANNEL_COUNT];

int LedArrayInterface::debug = 0;

/**** Device-specific variables ****/
TLC5955 tlc;                            // TLC5955 object
uint32_t gsclk_frequency = 5000000;     // Grayscale clock speed

// FORMAT: hole number, channel, 100*x, 100*y, 100*z
PROGMEM const int16_t LedArrayInterface::led_positions[793][5] = {
    {0, 90, 0, 0, 6500},
    {1, 150, 0, 417, 6500},
    {2, 108, 417, 0, 6500},
    {3, 94, -417, 0, 6500},
    {4, 91, 0, -417, 6500},
    {5, 104, 417, -417, 6500},
    {6, 95, -417, -417, 6500},
    {7, 160, 417, 417, 6500},
    {8, 146, -417, 417, 6500},
    {9, 151, 0, 835, 6500},
    {10, 109, 835, 0, 6500},
    {11, 87, 0, -835, 6500},
    {12, 93, -835, 0, 6500},
    {13, 147, -417, 835, 6500},
    {14, 161, 835, 417, 6500},
    {15, 145, -835, 417, 6500},
    {16, 164, 417, 835, 6500},
    {17, 100, 417, -835, 6500},
    {18, 83, -417, -835, 6500},
    {19, 89, -835, -417, 6500},
    {20, 105, 835, -417, 6500},
    {21, 85, -835, -835, 6500},
    {22, 101, 835, -835, 6500},
    {23, 165, 835, 835, 6500},
    {24, 149, -835, 835, 6500},
    {25, 86, 0, -1252, 6500},
    {26, 92, -1252, 0, 6500},
    {27, 110, 1252, 0, 6500},
    {28, 155, 0, 1252, 6500},
    {29, 162, 1252, 417, 6500},
    {30, 168, 417, 1252, 6500},
    {31, 111, 1252, -417, 6500},
    {32, 88, -1252, -417, 6500},
    {33, 82, -417, -1252, 6500},
    {34, 144, -1252, 417, 6500},
    {35, 159, -417, 1252, 6500},
    {36, 96, 417, -1252, 6500},
    {37, 99, 1252, -835, 6500},
    {38, 81, -835, -1252, 6500},
    {39, 153, -835, 1252, 6500},
    {40, 163, 1252, 835, 6500},
    {41, 169, 835, 1252, 6500},
    {42, 84, -1252, -835, 6500},
    {43, 97, 835, -1252, 6500},
    {44, 148, -1252, 835, 6500},
    {45, 74, -1669, 0, 6500},
    {46, 154, 0, 1669, 6500},
    {47, 26, 0, -1669, 6500},
    {48, 106, 1669, 0, 6500},
    {49, 158, -417, 1669, 6500},
    {50, 172, 417, 1669, 6500},
    {51, 107, 1669, -417, 6500},
    {52, 134, -1669, 417, 6500},
    {53, 44, 417, -1669, 6500},
    {54, 30, -417, -1669, 6500},
    {55, 166, 1669, 417, 6500},
    {56, 75, -1669, -417, 6500},
    {57, 152, -1252, 1252, 6500},
    {58, 98, 1252, -1252, 6500},
    {59, 80, -1252, -1252, 6500},
    {60, 175, 1252, 1252, 6500},
    {61, 157, -835, 1669, 6500},
    {62, 71, -1669, -835, 6500},
    {63, 173, 835, 1669, 6500},
    {64, 135, -1669, 835, 6500},
    {65, 167, 1669, 835, 6500},
    {66, 45, 835, -1669, 6500},
    {67, 103, 1669, -835, 6500},
    {68, 29, -835, -1669, 6500},
    {69, 214, 0, 2087, 6500},
    {70, 124, 2087, 0, 6500},
    {71, 78, -2087, 0, 6500},
    {72, 27, 0, -2087, 6500},
    {73, 139, -1669, 1252, 6500},
    {74, 28, -1252, -1669, 6500},
    {75, 171, 1669, 1252, 6500},
    {76, 156, -1252, 1669, 6500},
    {77, 102, 1669, -1252, 6500},
    {78, 174, 1252, 1669, 6500},
    {79, 46, 1252, -1669, 6500},
    {80, 70, -1669, -1252, 6500},
    {81, 130, -2087, 417, 6500},
    {82, 40, 417, -2087, 6500},
    {83, 120, 2087, -417, 6500},
    {84, 210, -417, 2087, 6500},
    {85, 176, 2087, 417, 6500},
    {86, 224, 417, 2087, 6500},
    {87, 79, -2087, -417, 6500},
    {88, 31, -417, -2087, 6500},
    {89, 209, -835, 2087, 6500},
    {90, 116, 2087, -835, 6500},
    {91, 67, -2087, -835, 6500},
    {92, 41, 835, -2087, 6500},
    {93, 225, 835, 2087, 6500},
    {94, 180, 2087, 835, 6500},
    {95, 131, -2087, 835, 6500},
    {96, 25, -835, -2087, 6500},
    {97, 10, -1669, -1669, 6500},
    {98, 170, 1669, 1669, 6500},
    {99, 138, -1669, 1669, 6500},
    {100, 42, 1669, -1669, 6500},
    {101, 226, 1252, 2087, 6500},
    {102, 112, 2087, -1252, 6500},
    {103, 47, 1252, -2087, 6500},
    {104, 24, -1252, -2087, 6500},
    {105, 184, 2087, 1252, 6500},
    {106, 208, -1252, 2087, 6500},
    {107, 143, -2087, 1252, 6500},
    {108, 66, -2087, -1252, 6500},
    {109, 23, 0, -2504, 6500},
    {110, 77, -2504, 0, 6500},
    {111, 125, 2504, 0, 6500},
    {112, 215, 0, 2504, 6500},
    {113, 228, 417, 2504, 6500},
    {114, 129, -2504, 417, 6500},
    {115, 19, -417, -2504, 6500},
    {116, 177, 2504, 417, 6500},
    {117, 73, -2504, -417, 6500},
    {118, 121, 2504, -417, 6500},
    {119, 211, -417, 2504, 6500},
    {120, 36, 417, -2504, 6500},
    {121, 181, 2504, 835, 6500},
    {122, 37, 835, -2504, 6500},
    {123, 213, -835, 2504, 6500},
    {124, 229, 835, 2504, 6500},
    {125, 21, -835, -2504, 6500},
    {126, 133, -2504, 835, 6500},
    {127, 69, -2504, -835, 6500},
    {128, 117, 2504, -835, 6500},
    {129, 14, -2087, -1669, 6500},
    {130, 230, 1669, 2087, 6500},
    {131, 11, -1669, -2087, 6500},
    {132, 60, 2087, -1669, 6500},
    {133, 188, 2087, 1669, 6500},
    {134, 198, -1669, 2087, 6500},
    {135, 142, -2087, 1669, 6500},
    {136, 43, 1669, -2087, 6500},
    {137, 227, 1252, 2504, 6500},
    {138, 212, -1252, 2504, 6500},
    {139, 35, 1252, -2504, 6500},
    {140, 65, -2504, -1252, 6500},
    {141, 20, -1252, -2504, 6500},
    {142, 137, -2504, 1252, 6500},
    {143, 185, 2504, 1252, 6500},
    {144, 113, 2504, -1252, 6500},
    {145, 219, 0, 2921, 6500},
    {146, 22, 0, -2921, 6500},
    {147, 126, 2921, 0, 6500},
    {148, 76, -2921, 0, 6500},
    {149, 194, -2087, 2087, 6500},
    {150, 240, 2087, 2087, 6500},
    {151, 56, 2087, -2087, 6500},
    {152, 15, -2087, -2087, 6500},
    {153, 223, -417, 2921, 6500},
    {154, 232, 417, 2921, 6500},
    {155, 18, -417, -2921, 6500},
    {156, 72, -2921, -417, 6500},
    {157, 178, 2921, 417, 6500},
    {158, 32, 417, -2921, 6500},
    {159, 127, 2921, -417, 6500},
    {160, 128, -2921, 417, 6500},
    {161, 199, -1669, 2504, 6500},
    {162, 13, -2504, -1669, 6500},
    {163, 39, 1669, -2504, 6500},
    {164, 189, 2504, 1669, 6500},
    {165, 231, 1669, 2504, 6500},
    {166, 7, -1669, -2504, 6500},
    {167, 141, -2504, 1669, 6500},
    {168, 61, 2504, -1669, 6500},
    {169, 179, 2921, 835, 6500},
    {170, 132, -2921, 835, 6500},
    {171, 115, 2921, -835, 6500},
    {172, 233, 835, 2921, 6500},
    {173, 33, 835, -2921, 6500},
    {174, 68, -2921, -835, 6500},
    {175, 217, -835, 2921, 6500},
    {176, 17, -835, -2921, 6500},
    {177, 761, 3051, 217, 6353},
    {178, 766, 3051, -218, 6353},
    {179, 329, -218, 3051, 6353},
    {180, 622, -218, -3051, 6353},
    {181, 473, -3051, -217, 6353},
    {182, 617, 217, -3051, 6353},
    {183, 334, 218, 3051, 6353},
    {184, 478, -3051, 218, 6353},
    {185, 191, 2921, 1252, 6500},
    {186, 216, -1252, 2921, 6500},
    {187, 136, -2921, 1252, 6500},
    {188, 34, 1252, -2921, 6500},
    {189, 64, -2921, -1252, 6500},
    {190, 114, 2921, -1252, 6500},
    {191, 239, 1252, 2921, 6500},
    {192, 16, -1252, -2921, 6500},
    {193, 330, 652, 3051, 6353},
    {194, 476, -3051, -652, 6353},
    {195, 474, -3051, 653, 6353},
    {196, 764, 3051, 652, 6353},
    {197, 332, -652, 3051, 6353},
    {198, 620, 652, -3051, 6353},
    {199, 762, 3051, -653, 6353},
    {200, 618, -653, -3051, 6353},
    {201, 193, -2504, 2087, 6500},
    {202, 52, 2087, -2504, 6500},
    {203, 195, -2087, 2504, 6500},
    {204, 9, -2504, -2087, 6500},
    {205, 57, 2504, -2087, 6500},
    {206, 241, 2504, 2087, 6500},
    {207, 3, -2087, -2504, 6500},
    {208, 244, 2087, 2504, 6500},
    {209, 314, -1088, 3051, 6353},
    {210, 746, 3051, 1087, 6353},
    {211, 458, -3051, -1087, 6353},
    {212, 348, 1088, 3051, 6353},
    {213, 780, 3051, -1088, 6353},
    {214, 602, 1087, -3051, 6353},
    {215, 636, -1088, -3051, 6353},
    {216, 492, -3051, 1088, 6353},
    {217, 62, 2921, -1669, 6500},
    {218, 203, -1669, 2921, 6500},
    {219, 12, -2921, -1669, 6500},
    {220, 6, -1669, -2921, 6500},
    {221, 38, 1669, -2921, 6500},
    {222, 190, 2921, 1669, 6500},
    {223, 235, 1669, 2921, 6500},
    {224, 140, -2921, 1669, 6500},
    {225, 633, -1523, -3051, 6353},
    {226, 462, -3051, -1522, 6353},
    {227, 777, 3051, -1523, 6353},
    {228, 606, 1522, -3051, 6353},
    {229, 750, 3051, 1522, 6353},
    {230, 318, -1523, 3051, 6353},
    {231, 345, 1523, 3051, 6353},
    {232, 489, -3051, 1523, 6353},
    {233, 477, -3268, 0, 6045},
    {234, 765, 3268, 0, 6045},
    {235, 621, 0, -3268, 6045},
    {236, 333, 0, 3268, 6045},
    {237, 5, -2504, -2504, 6500},
    {238, 197, -2504, 2504, 6500},
    {239, 245, 2504, 2504, 6500},
    {240, 53, 2504, -2504, 6500},
    {241, 623, -435, -3268, 6045},
    {242, 335, 435, 3268, 6045},
    {243, 472, -3268, -435, 6045},
    {244, 760, 3268, 435, 6045},
    {245, 479, -3268, 435, 6045},
    {246, 328, -435, 3268, 6045},
    {247, 616, 435, -3268, 6045},
    {248, 767, 3268, -435, 6045},
    {249, 248, 2087, 2921, 6500},
    {250, 242, 2921, 2087, 6500},
    {251, 192, -2921, 2087, 6500},
    {252, 63, 2921, -2087, 6500},
    {253, 48, 2087, -2921, 6500},
    {254, 8, -2921, -2087, 6500},
    {255, 2, -2087, -2921, 6500},
    {256, 207, -2087, 2921, 6500},
    {257, 315, -870, 3268, 6045},
    {258, 747, 3268, 870, 6045},
    {259, 331, 870, 3268, 6045},
    {260, 459, -3268, -870, 6045},
    {261, 603, 870, -3268, 6045},
    {262, 475, -3268, 870, 6045},
    {263, 763, 3268, -870, 6045},
    {264, 619, -870, -3268, 6045},
    {265, 457, -3051, -1958, 6353},
    {266, 745, 3051, 1957, 6353},
    {267, 313, -1958, 3051, 6353},
    {268, 601, 1958, -3051, 6353},
    {269, 782, 3051, -1958, 6353},
    {270, 638, -1958, -3051, 6353},
    {271, 350, 1958, 3051, 6353},
    {272, 494, -3051, 1958, 6353},
    {273, 751, 3268, 1305, 6045},
    {274, 463, -3268, -1305, 6045},
    {275, 344, 1305, 3268, 6045},
    {276, 488, -3268, 1305, 6045},
    {277, 607, 1305, -3268, 6045},
    {278, 319, -1305, 3268, 6045},
    {279, 776, 3268, -1305, 6045},
    {280, 632, -1305, -3268, 6045},
    {281, 201, -2504, 2921, 6500},
    {282, 196, -2921, 2504, 6500},
    {283, 4, -2921, -2504, 6500},
    {284, 243, 2921, 2504, 6500},
    {285, 51, 2921, -2504, 6500},
    {286, 249, 2504, 2921, 6500},
    {287, 1, -2504, -2921, 6500},
    {288, 49, 2504, -2921, 6500},
    {289, 614, -218, -3486, 5738},
    {290, 608, 217, -3486, 5738},
    {291, 752, 3486, 217, 5738},
    {292, 758, 3486, -218, 5738},
    {293, 464, -3486, -217, 5738},
    {294, 470, -3486, 218, 5738},
    {295, 326, 218, 3486, 5738},
    {296, 320, -218, 3486, 5738},
    {297, 460, -3051, -2392, 6353},
    {298, 604, 2392, -3051, 6353},
    {299, 778, 3051, -2393, 6353},
    {300, 748, 3051, 2392, 6353},
    {301, 346, 2393, 3051, 6353},
    {302, 316, -2393, 3051, 6353},
    {303, 490, -3051, 2393, 6353},
    {304, 634, -2393, -3051, 6353},
    {305, 749, 3268, 1740, 6045},
    {306, 605, 1740, -3268, 6045},
    {307, 461, -3268, -1740, 6045},
    {308, 781, 3268, -1740, 6045},
    {309, 637, -1740, -3268, 6045},
    {310, 349, 1740, 3268, 6045},
    {311, 493, -3268, 1740, 6045},
    {312, 317, -1740, 3268, 6045},
    {313, 468, -3486, -652, 5738},
    {314, 324, -652, 3486, 5738},
    {315, 615, -653, -3486, 5738},
    {316, 612, 652, -3486, 5738},
    {317, 759, 3486, -653, 5738},
    {318, 756, 3486, 652, 5738},
    {319, 327, 652, 3486, 5738},
    {320, 471, -3486, 653, 5738},
    {321, 255, 2921, 2921, 6500},
    {322, 200, -2921, 2921, 6500},
    {323, 50, 2921, -2921, 6500},
    {324, 0, -2921, -2921, 6500},
    {325, 743, 3486, 1087, 5738},
    {326, 599, 1087, -3486, 5738},
    {327, 772, 3486, -1088, 5738},
    {328, 311, -1088, 3486, 5738},
    {329, 484, -3486, 1088, 5738},
    {330, 340, 1088, 3486, 5738},
    {331, 455, -3486, -1087, 5738},
    {332, 628, -1088, -3486, 5738},
    {333, 783, 3268, -2175, 6045},
    {334, 600, 2175, -3268, 6045},
    {335, 456, -3268, -2175, 6045},
    {336, 639, -2175, -3268, 6045},
    {337, 351, 2175, 3268, 6045},
    {338, 495, -3268, 2175, 6045},
    {339, 744, 3268, 2175, 6045},
    {340, 312, -2175, 3268, 6045},
    {341, 510, -3051, -2828, 6353},
    {342, 798, 3051, 2828, 6353},
    {343, 396, 2828, 3051, 6353},
    {344, 366, -2828, 3051, 6353},
    {345, 540, -3051, 2828, 6353},
    {346, 828, 3051, -2828, 6353},
    {347, 654, 2828, -3051, 6353},
    {348, 684, -2828, -3051, 6353},
    {349, 742, 3486, 1522, 5738},
    {350, 454, -3486, -1522, 5738},
    {351, 310, -1523, 3486, 5738},
    {352, 768, 3486, -1523, 5738},
    {353, 480, -3486, 1523, 5738},
    {354, 336, 1523, 3486, 5738},
    {355, 624, -1523, -3486, 5738},
    {356, 598, 1522, -3486, 5738},
    {357, 465, -3703, 0, 5430},
    {358, 609, 0, -3703, 5430},
    {359, 753, 3703, 0, 5430},
    {360, 321, 0, 3703, 5430},
    {361, 469, -3703, -435, 5430},
    {362, 754, 3703, -435, 5430},
    {363, 322, 435, 3703, 5430},
    {364, 757, 3703, 435, 5430},
    {365, 610, -435, -3703, 5430},
    {366, 466, -3703, 435, 5430},
    {367, 325, -435, 3703, 5430},
    {368, 613, 435, -3703, 5430},
    {369, 651, 2610, -3268, 6045},
    {370, 507, -3268, -2610, 6045},
    {371, 347, 2610, 3268, 6045},
    {372, 363, -2610, 3268, 6045},
    {373, 491, -3268, 2610, 6045},
    {374, 635, -2610, -3268, 6045},
    {375, 779, 3268, -2610, 6045},
    {376, 795, 3268, 2610, 6045},
    {377, 448, -3486, -1958, 5738},
    {378, 592, 1958, -3486, 5738},
    {379, 486, -3486, 1958, 5738},
    {380, 736, 3486, 1957, 5738},
    {381, 342, 1958, 3486, 5738},
    {382, 630, -1958, -3486, 5738},
    {383, 774, 3486, -1958, 5738},
    {384, 304, -1958, 3486, 5738},
    {385, 323, 870, 3703, 5430},
    {386, 467, -3703, 870, 5430},
    {387, 755, 3703, -870, 5430},
    {388, 451, -3703, -870, 5430},
    {389, 307, -870, 3703, 5430},
    {390, 595, 870, -3703, 5430},
    {391, 739, 3703, 870, 5430},
    {392, 611, -870, -3703, 5430},
    {393, 485, -3703, 1305, 5430},
    {394, 306, -1305, 3703, 5430},
    {395, 629, -1305, -3703, 5430},
    {396, 341, 1305, 3703, 5430},
    {397, 450, -3703, -1305, 5430},
    {398, 738, 3703, 1305, 5430},
    {399, 594, 1305, -3703, 5430},
    {400, 773, 3703, -1305, 5430},
    {401, 775, 3486, -2393, 5738},
    {402, 343, 2393, 3486, 5738},
    {403, 596, 2392, -3486, 5738},
    {404, 308, -2393, 3486, 5738},
    {405, 452, -3486, -2392, 5738},
    {406, 740, 3486, 2392, 5738},
    {407, 631, -2393, -3486, 5738},
    {408, 487, -3486, 2393, 5738},
    {409, 537, -3268, 3045, 6045},
    {410, 825, 3268, -3045, 6045},
    {411, 393, 3045, 3268, 6045},
    {412, 681, -3045, -3268, 6045},
    {413, 797, 3268, 3045, 6045},
    {414, 509, -3268, -3045, 6045},
    {415, 653, 3045, -3268, 6045},
    {416, 365, -3045, 3268, 6045},
    {417, 449, -3703, -1740, 5430},
    {418, 305, -1740, 3703, 5430},
    {419, 625, -1740, -3703, 5430},
    {420, 737, 3703, 1740, 5430},
    {421, 593, 1740, -3703, 5430},
    {422, 481, -3703, 1740, 5430},
    {423, 337, 1740, 3703, 5430},
    {424, 769, 3703, -1740, 5430},
    {425, 286, 218, 3921, 5123},
    {426, 713, 3921, 217, 5123},
    {427, 430, -3921, 218, 5123},
    {428, 281, -218, 3921, 5123},
    {429, 569, 217, -3921, 5123},
    {430, 425, -3921, -217, 5123},
    {431, 718, 3921, -218, 5123},
    {432, 574, -218, -3921, 5123},
    {433, 572, 652, -3921, 5123},
    {434, 714, 3921, -653, 5123},
    {435, 428, -3921, -652, 5123},
    {436, 282, 652, 3921, 5123},
    {437, 570, -653, -3921, 5123},
    {438, 716, 3921, 652, 5123},
    {439, 284, -652, 3921, 5123},
    {440, 426, -3921, 653, 5123},
    {441, 676, -2828, -3486, 5738},
    {442, 820, 3486, -2828, 5738},
    {443, 367, -2828, 3486, 5738},
    {444, 388, 2828, 3486, 5738},
    {445, 655, 2828, -3486, 5738},
    {446, 511, -3486, -2828, 5738},
    {447, 532, -3486, 2828, 5738},
    {448, 799, 3486, 2828, 5738},
    {449, 309, -2175, 3703, 5430},
    {450, 626, -2175, -3703, 5430},
    {451, 338, 2175, 3703, 5430},
    {452, 482, -3703, 2175, 5430},
    {453, 597, 2175, -3703, 5430},
    {454, 770, 3703, -2175, 5430},
    {455, 453, -3703, -2175, 5430},
    {456, 741, 3703, 2175, 5430},
    {457, 554, 1087, -3921, 5123},
    {458, 698, 3921, 1087, 5123},
    {459, 300, 1088, 3921, 5123},
    {460, 410, -3921, -1087, 5123},
    {461, 266, -1088, 3921, 5123},
    {462, 588, -1088, -3921, 5123},
    {463, 732, 3921, -1088, 5123},
    {464, 444, -3921, 1088, 5123},
    {465, 585, -1523, -3921, 5123},
    {466, 729, 3921, -1523, 5123},
    {467, 441, -3921, 1523, 5123},
    {468, 270, -1523, 3921, 5123},
    {469, 558, 1522, -3921, 5123},
    {470, 414, -3921, -1522, 5123},
    {471, 297, 1523, 3921, 5123},
    {472, 702, 3921, 1522, 5123},
    {473, 686, -3263, -3486, 5738},
    {474, 364, -3263, 3486, 5738},
    {475, 398, 3263, 3486, 5738},
    {476, 830, 3486, -3263, 5738},
    {477, 542, -3486, 3263, 5738},
    {478, 508, -3486, -3263, 5738},
    {479, 796, 3486, 3262, 5738},
    {480, 652, 3262, -3486, 5738},
    {481, 627, -2610, -3703, 5430},
    {482, 483, -3703, 2610, 5430},
    {483, 498, -3703, -2610, 5430},
    {484, 354, -2610, 3703, 5430},
    {485, 642, 2610, -3703, 5430},
    {486, 339, 2610, 3703, 5430},
    {487, 786, 3703, 2610, 5430},
    {488, 771, 3703, -2610, 5430},
    {489, 409, -3921, -1958, 5123},
    {490, 590, -1958, -3921, 5123},
    {491, 302, 1958, 3921, 5123},
    {492, 553, 1958, -3921, 5123},
    {493, 265, -1958, 3921, 5123},
    {494, 697, 3921, 1957, 5123},
    {495, 446, -3921, 1958, 5123},
    {496, 734, 3921, -1958, 5123},
    {497, 717, 4138, 0, 4815},
    {498, 285, 0, 4138, 4815},
    {499, 429, -4138, 0, 4815},
    {500, 573, 0, -4138, 4815},
    {501, 280, -435, 4138, 4815},
    {502, 712, 4138, 435, 4815},
    {503, 719, 4138, -435, 4815},
    {504, 568, 435, -4138, 4815},
    {505, 575, -435, -4138, 4815},
    {506, 287, 435, 4138, 4815},
    {507, 424, -4138, -435, 4815},
    {508, 431, -4138, 435, 4815},
    {509, 699, 4138, 870, 4815},
    {510, 571, -870, -4138, 4815},
    {511, 267, -870, 4138, 4815},
    {512, 283, 870, 4138, 4815},
    {513, 555, 870, -4138, 4815},
    {514, 411, -4138, -870, 4815},
    {515, 715, 4138, -870, 4815},
    {516, 427, -4138, 870, 4815},
    {517, 392, 3045, 3703, 5430},
    {518, 536, -3703, 3045, 5430},
    {519, 506, -3703, -3045, 5430},
    {520, 362, -3045, 3703, 5430},
    {521, 650, 3045, -3703, 5430},
    {522, 680, -3045, -3703, 5430},
    {523, 794, 3703, 3045, 5430},
    {524, 824, 3703, -3045, 5430},
    {525, 268, -2393, 3921, 5123},
    {526, 442, -3921, 2393, 5123},
    {527, 298, 2393, 3921, 5123},
    {528, 586, -2393, -3921, 5123},
    {529, 730, 3921, -2393, 5123},
    {530, 412, -3921, -2392, 5123},
    {531, 700, 3921, 2392, 5123},
    {532, 556, 2392, -3921, 5123},
    {533, 703, 4138, 1305, 4815},
    {534, 415, -4138, -1305, 4815},
    {535, 296, 1305, 4138, 4815},
    {536, 440, -4138, 1305, 4815},
    {537, 584, -1305, -4138, 4815},
    {538, 559, 1305, -4138, 4815},
    {539, 271, -1305, 4138, 4815},
    {540, 728, 4138, -1305, 4815},
    {541, 301, 1740, 4138, 4815},
    {542, 733, 4138, -1740, 4815},
    {543, 589, -1740, -4138, 4815},
    {544, 445, -4138, 1740, 4815},
    {545, 269, -1740, 4138, 4815},
    {546, 557, 1740, -4138, 4815},
    {547, 413, -4138, -1740, 4815},
    {548, 701, 4138, 1740, 4815},
    {549, 399, 3480, 3703, 5430},
    {550, 356, -3480, 3703, 5430},
    {551, 644, 3480, -3703, 5430},
    {552, 543, -3703, 3480, 5430},
    {553, 500, -3703, -3480, 5430},
    {554, 788, 3703, 3480, 5430},
    {555, 831, 3703, -3480, 5430},
    {556, 687, -3480, -3703, 5430},
    {557, 502, -3921, -2828, 5123},
    {558, 790, 3921, 2828, 5123},
    {559, 646, 2828, -3921, 5123},
    {560, 817, 3921, -2828, 5123},
    {561, 673, -2828, -3921, 5123},
    {562, 385, 2828, 3921, 5123},
    {563, 529, -3921, 2828, 5123},
    {564, 358, -2828, 3921, 5123},
    {565, 710, 4356, -218, 4508},
    {566, 422, -4356, 218, 4508},
    {567, 278, 218, 4356, 4508},
    {568, 566, -218, -4356, 4508},
    {569, 272, -218, 4356, 4508},
    {570, 416, -4356, -217, 4508},
    {571, 560, 217, -4356, 4508},
    {572, 704, 4356, 217, 4508},
    {573, 264, -2175, 4138, 4815},
    {574, 552, 2175, -4138, 4815},
    {575, 408, -4138, -2175, 4815},
    {576, 303, 2175, 4138, 4815},
    {577, 447, -4138, 2175, 4815},
    {578, 591, -2175, -4138, 4815},
    {579, 696, 4138, 2175, 4815},
    {580, 735, 4138, -2175, 4815},
    {581, 279, 652, 4356, 4508},
    {582, 276, -652, 4356, 4508},
    {583, 711, 4356, -653, 4508},
    {584, 423, -4356, 653, 4508},
    {585, 420, -4356, -652, 4508},
    {586, 567, -653, -4356, 4508},
    {587, 708, 4356, 652, 4508},
    {588, 564, 652, -4356, 4508},
    {589, 685, -3263, -3921, 5123},
    {590, 829, 3921, -3263, 5123},
    {591, 649, 3262, -3921, 5123},
    {592, 793, 3921, 3262, 5123},
    {593, 505, -3921, -3263, 5123},
    {594, 397, 3263, 3921, 5123},
    {595, 361, -3263, 3921, 5123},
    {596, 541, -3921, 3263, 5123},
    {597, 695, 4356, 1087, 4508},
    {598, 551, 1087, -4356, 4508},
    {599, 580, -1088, -4356, 4508},
    {600, 263, -1088, 4356, 4508},
    {601, 292, 1088, 4356, 4508},
    {602, 724, 4356, -1088, 4508},
    {603, 436, -4356, 1088, 4508},
    {604, 407, -4356, -1087, 4508},
    {605, 809, 4138, 2610, 4815},
    {606, 587, -2610, -4138, 4815},
    {607, 731, 4138, -2610, 4815},
    {608, 443, -4138, 2610, 4815},
    {609, 377, -2610, 4138, 4815},
    {610, 299, 2610, 4138, 4815},
    {611, 665, 2610, -4138, 4815},
    {612, 521, -4138, -2610, 4815},
    {613, 576, -1523, -4356, 4508},
    {614, 550, 1522, -4356, 4508},
    {615, 406, -4356, -1522, 4508},
    {616, 288, 1523, 4356, 4508},
    {617, 262, -1523, 4356, 4508},
    {618, 694, 4356, 1522, 4508},
    {619, 720, 4356, -1523, 4508},
    {620, 432, -4356, 1523, 4508},
    {621, 784, 3921, 3698, 5123},
    {622, 827, 3921, -3698, 5123},
    {623, 395, 3698, 3921, 5123},
    {624, 539, -3921, 3698, 5123},
    {625, 496, -3921, -3698, 5123},
    {626, 683, -3698, -3921, 5123},
    {627, 640, 3698, -3921, 5123},
    {628, 352, -3698, 3921, 5123},
    {629, 256, -1958, 4356, 4508},
    {630, 400, -4356, -1958, 4508},
    {631, 726, 4356, -1958, 4508},
    {632, 544, 1957, -4356, 4508},
    {633, 688, 4356, 1957, 4508},
    {634, 294, 1958, 4356, 4508},
    {635, 582, -1958, -4356, 4508},
    {636, 438, -4356, 1958, 4508},
    {637, 533, -4138, 3045, 4815},
    {638, 499, -4138, -3045, 4815},
    {639, 677, -3045, -4138, 4815},
    {640, 389, 3045, 4138, 4815},
    {641, 355, -3045, 4138, 4815},
    {642, 821, 4138, -3045, 4815},
    {643, 787, 4138, 3045, 4815},
    {644, 643, 3045, -4138, 4815},
    {645, 561, 0, -4573, 4200},
    {646, 417, -4573, 0, 4200},
    {647, 705, 4573, 0, 4200},
    {648, 273, 0, 4573, 4200},
    {649, 565, 435, -4573, 4200},
    {650, 418, -4573, 435, 4200},
    {651, 421, -4573, -435, 4200},
    {652, 277, -435, 4573, 4200},
    {653, 562, -435, -4573, 4200},
    {654, 274, 435, 4573, 4200},
    {655, 706, 4573, -435, 4200},
    {656, 709, 4573, 435, 4200},
    {657, 548, 2392, -4356, 4508},
    {658, 692, 4356, 2392, 4508},
    {659, 727, 4356, -2393, 4508},
    {660, 439, -4356, 2393, 4508},
    {661, 295, 2393, 4356, 4508},
    {662, 404, -4356, -2392, 4508},
    {663, 260, -2393, 4356, 4508},
    {664, 583, -2393, -4356, 4508},
    {665, 707, 4573, -870, 4200},
    {666, 419, -4573, 870, 4200},
    {667, 259, -870, 4573, 4200},
    {668, 275, 870, 4573, 4200},
    {669, 403, -4573, -870, 4200},
    {670, 563, -870, -4573, 4200},
    {671, 547, 870, -4573, 4200},
    {672, 691, 4573, 870, 4200},
    {673, 360, -3480, 4138, 4815},
    {674, 792, 4138, 3480, 4815},
    {675, 826, 4138, -3480, 4815},
    {676, 394, 3480, 4138, 4815},
    {677, 538, -4138, 3480, 4815},
    {678, 504, -4138, -3480, 4815},
    {679, 682, -3480, -4138, 4815},
    {680, 648, 3480, -4138, 4815},
    {681, 690, 4573, 1305, 4200},
    {682, 258, -1305, 4573, 4200},
    {683, 293, 1305, 4573, 4200},
    {684, 546, 1305, -4573, 4200},
    {685, 437, -4573, 1305, 4200},
    {686, 402, -4573, -1305, 4200},
    {687, 581, -1305, -4573, 4200},
    {688, 725, 4573, -1305, 4200},
    {689, 376, -2828, 4356, 4508},
    {690, 375, 2828, 4356, 4508},
    {691, 520, -4356, -2828, 4508},
    {692, 519, -4356, 2828, 4508},
    {693, 807, 4356, -2828, 4508},
    {694, 663, -2828, -4356, 4508},
    {695, 664, 2828, -4356, 4508},
    {696, 808, 4356, 2828, 4508},
    {697, 401, -4573, -1740, 4200},
    {698, 721, 4573, -1740, 4200},
    {699, 433, -4573, 1740, 4200},
    {700, 577, -1740, -4573, 4200},
    {701, 545, 1740, -4573, 4200},
    {702, 689, 4573, 1740, 4200},
    {703, 289, 1740, 4573, 4200},
    {704, 257, -1740, 4573, 4200},
    {705, 531, -4138, 3915, 4815},
    {706, 353, -3915, 4138, 4815},
    {707, 387, 3915, 4138, 4815},
    {708, 819, 4138, -3915, 4815},
    {709, 641, 3915, -4138, 4815},
    {710, 675, -3915, -4138, 4815},
    {711, 785, 4138, 3915, 4815},
    {712, 497, -4138, -3915, 4815},
    {713, 722, 4573, -2175, 4200},
    {714, 290, 2175, 4573, 4200},
    {715, 261, -2175, 4573, 4200},
    {716, 405, -4573, -2175, 4200},
    {717, 434, -4573, 2175, 4200},
    {718, 693, 4573, 2175, 4200},
    {719, 578, -2175, -4573, 4200},
    {720, 549, 2175, -4573, 4200},
    {721, 359, -3263, 4356, 4508},
    {722, 528, -4356, 3263, 4508},
    {723, 384, 3263, 4356, 4508},
    {724, 503, -4356, -3263, 4508},
    {725, 791, 4356, 3262, 4508},
    {726, 816, 4356, -3263, 4508},
    {727, 647, 3262, -4356, 4508},
    {728, 672, -3263, -4356, 4508},
    {729, 666, -218, -4791, 3892},
    {730, 378, 218, 4791, 3892},
    {731, 670, 217, -4791, 3892},
    {732, 810, 4791, -218, 3892},
    {733, 382, -218, 4791, 3892},
    {734, 526, -4791, -217, 3892},
    {735, 814, 4791, 217, 3892},
    {736, 522, -4791, 218, 3892},
    {737, 525, -4791, -652, 3892},
    {738, 383, 652, 4791, 3892},
    {739, 669, 652, -4791, 3892},
    {740, 381, -652, 4791, 3892},
    {741, 813, 4791, 652, 3892},
    {742, 527, -4791, 653, 3892},
    {743, 671, -653, -4791, 3892},
    {744, 815, 4791, -653, 3892},
    {745, 800, 4573, 2610, 4200},
    {746, 656, 2610, -4573, 4200},
    {747, 291, 2610, 4573, 4200},
    {748, 723, 4573, -2610, 4200},
    {749, 579, -2610, -4573, 4200},
    {750, 368, -2610, 4573, 4200},
    {751, 512, -4573, -2610, 4200},
    {752, 435, -4573, 2610, 4200},
    {753, 668, 1087, -4791, 3892},
    {754, 380, -1088, 4791, 3892},
    {755, 812, 4791, 1087, 3892},
    {756, 524, -4791, -1087, 3892},
    {757, 379, 1088, 4791, 3892},
    {758, 523, -4791, 1088, 3892},
    {759, 667, -1088, -4791, 3892},
    {760, 811, 4791, -1088, 3892},
    {761, 789, 4356, 3698, 4508},
    {762, 645, 3698, -4356, 4508},
    {763, 535, -4356, 3698, 4508},
    {764, 501, -4356, -3698, 4508},
    {765, 679, -3698, -4356, 4508},
    {766, 391, 3698, 4356, 4508},
    {767, 357, -3698, 4356, 4508},
    {768, 823, 4356, -3698, 4508},
    {769, 804, 4791, 1522, 3892},
    {770, 803, 4791, -1523, 3892},
    {771, 372, -1523, 4791, 3892},
    {772, 515, -4791, 1523, 3892},
    {773, 659, -1523, -4791, 3892},
    {774, 371, 1523, 4791, 3892},
    {775, 516, -4791, -1522, 3892},
    {776, 660, 1522, -4791, 3892},
    {777, 802, 4573, -3045, 4200},
    {778, 661, 3045, -4573, 4200},
    {779, 370, 3045, 4573, 4200},
    {780, 658, -3045, -4573, 4200},
    {781, 517, -4573, -3045, 4200},
    {782, 805, 4573, 3045, 4200},
    {783, 373, -3045, 4573, 4200},
    {784, 514, -4573, 3045, 4200},
    {785, 513, -4791, -1958, 3892},
    {786, 657, 1957, -4791, 3892},
    {787, 369, -1958, 4791, 3892},
    {788, 374, 1958, 4791, 3892},
    {789, 806, 4791, -1958, 3892},
    {790, 518, -4791, 1958, 3892},
    {791, 662, -1958, -4791, 3892},
    {792, 801, 4791, 1957, 3892},
};

void LedArrayInterface::setPinOrder(int16_t led_number, int16_t color_channel_index, uint8_t position)
{
  tlc.setPinOrderSingle(led_number, color_channel_index, position);
}

void LedArrayInterface::notImplemented(const char * command_name)
{
  Serial.print(F("Command "));
  Serial.print(command_name);
  Serial.printf(F(" is not implemented for this device.%s"), SERIAL_LINE_ENDING);
}

uint16_t LedArrayInterface::getLedValue(uint16_t led_number, int color_channel_index)
{
  int16_t channel_number = (int16_t)pgm_read_word(&(led_positions[led_number][1]));
  if (channel_number >= 0)
    return tlc.getChannelValue(channel_number, color_channel_index);
  else
  {
    Serial.print(F("ERROR (LedArrayInterface::getLedValue) - invalid LED number ("));
    Serial.print(led_number);
    Serial.printf(F(")%s"), SERIAL_LINE_ENDING);
    return 0;
  }
}

void LedArrayInterface::setLedFast(int16_t led_number, int color_channel_index, bool value)
{
  notImplemented("setLedFast");
}

// Debug Variables
bool LedArrayInterface::getDebug()
{
  return (LedArrayInterface::debug);
}

void LedArrayInterface::setDebug(int state)
{
  LedArrayInterface::debug = state;
  Serial.printf(F("(LedArrayInterface::setDebug): Set debug level to %d \n"), debug);
}

int LedArrayInterface::setTriggerState(int trigger_index, bool state)
{
  // Get trigger pin
  int trigger_pin = trigger_output_pin_list[trigger_index];
  if (trigger_pin > 0)
  {
    if (state)
      digitalWriteFast(trigger_pin, HIGH);
    else
      digitalWriteFast(trigger_pin, LOW);
    return (1);
  } else {
    return (-1);
  }
}

int LedArrayInterface::getInputTriggerState(int input_trigger_index)
{
  // Get trigger pin
  int trigger_pin = trigger_input_pin_list[input_trigger_index];
  if (trigger_pin > 0)
    return (trigger_input_state[trigger_pin]);
  else
    return (-1);
}

int LedArrayInterface::sendTriggerPulse(int trigger_index, uint16_t delay_us, bool inverse_polarity)
{
  // Get trigger pin
  int trigger_pin = trigger_output_pin_list[trigger_index];

  if (trigger_pin > 0)
  {
    // Write active state
    if (inverse_polarity)
      digitalWriteFast(trigger_pin, LOW);
    else
      digitalWriteFast(trigger_pin, HIGH);

    // Delay if desired
    if (delay_us > 0)
      delayMicroseconds(delay_us);

    // Write normal state
    if (inverse_polarity)
      digitalWriteFast(trigger_pin, HIGH);
    else
      digitalWriteFast(trigger_pin, LOW);
    return (1);
  } else {
    return (-1);
  }
}
void LedArrayInterface::update()
{
  tlc.updateLeds();
}

void LedArrayInterface::clear()
{
  tlc.setAllLed(0);
  tlc.updateLeds();
}

void LedArrayInterface::setChannel(int16_t channel_number, int16_t color_channel_number, uint16_t value)
{
  if (debug >= 2)
  {
    Serial.print(F("Drawing channel #"));
    Serial.print(channel_number);
    Serial.print(F(", color_channel #"));
    Serial.print(color_channel_number);
    Serial.print(F(" to value "));
    Serial.print(value);
    Serial.print(SERIAL_LINE_ENDING);
  }

  if (channel_number >= 0)
  {
    if (color_channel_number < 0)
      tlc.setLed(channel_number, value);
    else if (color_channel_number == 0)
      tlc.setLed(channel_number, value, tlc.getChannelValue(channel_number, 1), tlc.getChannelValue(channel_number, 2));
    else if (color_channel_number == 1)
      tlc.setLed(channel_number, tlc.getChannelValue(channel_number, 0), value, tlc.getChannelValue(channel_number, 2));
    else if (color_channel_number == 2)
      tlc.setLed(channel_number, tlc.getChannelValue(channel_number, 0), tlc.getChannelValue(channel_number, 1), value);
  }
  else
  {
    Serial.print(F("Error (LedArrayInterface::setChannel): Invalid channel ("));
    Serial.print(channel_number);
    Serial.printf(F(")%s"), SERIAL_LINE_ENDING);
  }
}

void LedArrayInterface::setChannel(int16_t channel_number, int16_t color_channel_number, uint8_t value)
{
  setChannel(channel_number, color_channel_number, (uint16_t) (value * UINT16_MAX / UINT8_MAX));
}

void LedArrayInterface::setChannel(int16_t channel_number, int16_t color_channel_number, bool value)
{
  setChannel(channel_number, color_channel_number, (uint16_t) (value > 0 * UINT16_MAX));
}

void LedArrayInterface::setLed(int16_t led_number, int16_t color_channel_number, uint16_t value)
{
  if (debug >= 2)
  {
    Serial.print("U16 Setting led #");
    Serial.print(led_number);
    Serial.print(", color channel #");
    Serial.print(color_channel_number);
    Serial.print(" to value ");
    Serial.print(value);
    Serial.print(SERIAL_LINE_ENDING);
  }
  if (led_number < 0)
  {
    for (uint16_t led_index = 0; led_index < led_count; led_index++)
    {
      int16_t channel_number = (int16_t)pgm_read_word(&(led_positions[led_index][1]));
      setChannel(channel_number, color_channel_number, value);
    }
  }
  else
  {
    int16_t channel_number = (int16_t)pgm_read_word(&(led_positions[led_number][1]));
    setChannel(channel_number, color_channel_number, value);
  }
}

void LedArrayInterface::setLed(int16_t led_number, int16_t color_channel_number, uint8_t value)
{
  if (debug >= 2)
  {
    Serial.print("U8 Setting led #");
    Serial.print(led_number);
    Serial.print(", color channel #");
    Serial.print(color_channel_number);
    Serial.print(SERIAL_LINE_ENDING);
  }
  setLed(led_number, color_channel_number, (uint16_t) (value * UINT16_MAX / UINT8_MAX));
}

void LedArrayInterface::setLed(int16_t led_number, int16_t color_channel_number, bool value)
{
  if (debug >= 2)
  {
    Serial.print("B Setting led #");
    Serial.print(led_number);
    Serial.print(", color channel #");
    Serial.print(color_channel_number);
    Serial.print(SERIAL_LINE_ENDING);
  }
  setLed(led_number, color_channel_number, (uint16_t) (value * UINT16_MAX));
}

void LedArrayInterface::deviceReset()
{
  deviceSetup();
}

void LedArrayInterface::deviceSetup()
{
  // Now set the GSCK to an output and a 50% PWM duty-cycle
  // For simplicity all three grayscale clocks are tied to the same pin
  pinMode(GSCLK, OUTPUT);
  pinMode(LAT, OUTPUT);

  // Adjust PWM timer for maximum GSCLK frequency
  analogWriteFrequency(GSCLK, gsclk_frequency);
  analogWriteResolution(1);
  analogWrite(GSCLK, 1);

  // The library does not ininiate SPI for you, so as to prevent issues with other SPI libraries
  SPI.setMOSI(SPI_MOSI);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);

  tlc.init(LAT, SPI_MOSI, SPI_CLK);

  // We must set dot correction values, so set them all to the brightest adjustment
  tlc.setAllDcData(127);

  // Set Max Current Values (see TLC5955 datasheet)
  tlc.setMaxCurrent(3, 3, 3); // Go up to 7

  // Set Function Control Data Latch values. See the TLC5955 Datasheet for the purpose of this latch.
  // DSPRPT, TMGRST, RFRESH, ESPWM, LSDVLT
  tlc.setFunctionData(true, true, false, true, true); // WORKS with fast update

  // Set all LED current levels to max (127)
  int currentR = 127;
  int currentB = 127;
  int currentG = 127;
  tlc.setBrightnessCurrent(currentR, currentB, currentG);

  // Update vontrol register
  tlc.updateControl();

  // Set RGB pin order
  tlc.setRgbPinOrder(0, 1, 2);

  // Update the GS register
  clear();

  // Output trigger Pins
  for (int trigger_index = 0; trigger_index < trigger_output_count; trigger_index++)
  {
    pinMode(trigger_output_pin_list[trigger_index], OUTPUT);
    digitalWriteFast(trigger_output_pin_list[trigger_index], LOW);
  }

  // Input trigger Pins
  for (int trigger_index = 0; trigger_index < trigger_input_count; trigger_index++)
    pinMode(trigger_input_pin_list[trigger_index], INPUT);

}
#endif
