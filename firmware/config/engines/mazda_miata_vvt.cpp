/*
 * @file	mazda_miata_vvt.cpp
 *
 * Miata NB2, also known as MX-5 Mk2.5
 *
 * Frankenso MAZDA_MIATA_2003
 * set engine_type 47
 *
 * coil1/4          (p1 +5 VP)    Gpio::E14
 * coil2/2          (p1 +5 VP)    Gpio::C9
 * tachometer +5 VP (p3 +12 VP)   Gpio::E8
 * alternator +5 VP (p3 +12 VP)   Gpio::E10
 * ETB PWM                        Gpio::E6 inverted low-side with pull-up
 * ETB dir1                       Gpio::E12
 * ETB dir2                       Gpio::C7
 *
 * COP ion #1                     Gpio::D8
 * COP ion #3                     Gpio::D9
 *
 * @date Oct 4, 2016
 * @author Andrey Belomutskiy, (c) 2012-2020
 * http://rusefi.com/forum/viewtopic.php?f=3&t=1095
 *
 *
 * See also trigger_type_e::TT_MAZDA_MIATA_VVT_TEST for trigger simulation
 *
 * Based on http://rusefi.com/wiki/index.php?title=Manual:Hardware_Frankenso_board#Default_Pinout
 *
 * board #70 - red car, hunchback compatible
 * set engine_type 55
 *
 * Crank   primary trigger        PA5 (3E in Miata board)       white
 * Cam     vvt input              PC6 (3G in Miata board)       blue
 * Wideband input                 PA3 (3J in Miata board)
 *
 * coil1/4          (p1 +5 VP)    Gpio::E14
 * coil2/2          (p1 +5 VP)    Gpio::C7
 *
 * tachometer +5 VP (p3 +12 VP)   Gpio::E8
 * alternator +5 VP (p3 +12 VP)   Gpio::E10
 *
 * VVT solenoid on aux PID#1      Gpio::E3
 * warning light                  Gpio::E6
 *
 *
 * idle solenoid                  PC13 on middle harness plug. diodes seem to be in the harness
 */

#include "pch.h"

#include "mazda_miata_vvt.h"
#include "custom_engine.h"
#include "mazda_miata_base_maps.h"


#if HW_PROTEUS
#include "proteus_meta.h"
#endif

#include "mre_meta.h"

static const uint16_t vvt18fsioRpmBins[] = { 950, 1800, 2500, 3300, 4200, 5000, 6000, 7200 };
static const uint8_t vvt18fsioLoadBins[] = { 30, 40, 50, 60, 70, 75, 82, 100 };

static const uint8_t SCRIPT_TABLE_vvt_target[8][8] = {
	{ 0,	0,	0,	0,	0,	0,	0,	0 },
	{ 0,	0,	15,	15,	15,	15,	0,	0 },
	{ 0,	0,	15,	20,	20,	15,	0,	0 },
	{ 8,	15,	18,	20,	20,	18,	15,	8 },
	{ 8,	15,	20,	20,	20,	18,	15,	8 },
	{ 8,	15,	25,	25,	23,	20,	12,	8 },
	{ 12,	19,	29,	29,	27,	20,	4,	0 },
	{ 12,	19,	29,	29,	27,	20,	5,	1 },
};

const float mazda_miata_nb2_RpmBins[FUEL_RPM_COUNT] = {700.0, 820.0, 950.0, 1100.0,
		1300.0, 1550.0, 1800.0, 2150.0,
		2500.0, 3000.0, 3500.0, 4150.0,
		4900.0, 5800.0, 6800.0, 8000.0}
;

const float mazda_miata_nb2_LoadBins[FUEL_LOAD_COUNT] = {20.0, 25.0, 30.0, 35.0,
		40.0, 46.0, 54.0, 63.0,
		73.0, 85.0, 99.0, 116.0,
		135.0, 158.0, 185.0, 220.0}
;

static const  float ignition18vvtRpmBins[FUEL_RPM_COUNT] = {
		700.0, 		         850.0 ,		         943.0 ,
		         1112.0 ,		         1310.0 ,		         1545.0 ,
		         1821.0, 		         2146.0, 		         2530.0,
		         2982.0, 		         3515.0 ,		         4144.0 ,
		         4884.0 ,		         5757.0 ,		         6787.0, 		         8000.0};

static const float ignition18vvtLoadBins[FUEL_LOAD_COUNT] = {
		25.0 ,		         29.10009765625 ,		         34.0 ,		         39.60009765625 ,
		         46.2001953125 ,		         53.89990234375 ,		         62.7998046875 ,
				 73.2001953125 ,		         85.400390625 ,		         99.5 ,		         116.0 ,
		         135.30078125 ,		         157.69921875 ,		         183.900390625 ,		         214.400390625 ,
		         250.0};

static const int8_t mapBased18vvtVeTable_NB_fuel_rail[16][16] = {
		/* Generated by TS2C on Tue Apr 18 21:46:03 EDT 2017*/
		{/* 0 20	*//* 0 700.0*/35,	/* 1 820.0*/36,	/* 2 950.0*/37,	/* 3 1100.0*/35,	/* 4 1300.0*/36,	/* 5 1550.0*/37,	/* 6 1800.0*/33,	/* 7 2150.0*/31,	/* 8 2500.0*/25,	/* 9 3000.0*/24,	/* 10 3500.0*/24,	/* 11 4150.0*/25,	/* 12 4900.0*/26,	/* 13 5800.0*/29,	/* 14 6800.0*/33,	/* 15 8000.0*/36,	},
		{/* 1 25	*//* 0 700.0*/35,	/* 1 820.0*/37,	/* 2 950.0*/38,	/* 3 1100.0*/37,	/* 4 1300.0*/36,	/* 5 1550.0*/37,	/* 6 1800.0*/41,	/* 7 2150.0*/39,	/* 8 2500.0*/40,	/* 9 3000.0*/37,	/* 10 3500.0*/35,	/* 11 4150.0*/36,	/* 12 4900.0*/37,	/* 13 5800.0*/35,	/* 14 6800.0*/38,	/* 15 8000.0*/40,	},
		{/* 2 30	*//* 0 700.0*/37,	/* 1 820.0*/40,	/* 2 950.0*/39,	/* 3 1100.0*/37,	/* 4 1300.0*/38,	/* 5 1550.0*/41,	/* 6 1800.0*/45,	/* 7 2150.0*/47,	/* 8 2500.0*/54,	/* 9 3000.0*/48,	/* 10 3500.0*/47,	/* 11 4150.0*/55,	/* 12 4900.0*/55,	/* 13 5800.0*/49,	/* 14 6800.0*/50,	/* 15 8000.0*/51,	},
		{/* 3 35	*//* 0 700.0*/39,	/* 1 820.0*/44,	/* 2 950.0*/42,	/* 3 1100.0*/40,	/* 4 1300.0*/45,	/* 5 1550.0*/48,	/* 6 1800.0*/48,	/* 7 2150.0*/52,	/* 8 2500.0*/56,	/* 9 3000.0*/53,	/* 10 3500.0*/52,	/* 11 4150.0*/58,	/* 12 4900.0*/62,	/* 13 5800.0*/57,	/* 14 6800.0*/58,	/* 15 8000.0*/58,	},
		{/* 4 40	*//* 0 700.0*/45,	/* 1 820.0*/56,	/* 2 950.0*/49,	/* 3 1100.0*/45,	/* 4 1300.0*/54,	/* 5 1550.0*/53,	/* 6 1800.0*/55,	/* 7 2150.0*/54,	/* 8 2500.0*/57,	/* 9 3000.0*/55,	/* 10 3500.0*/57,	/* 11 4150.0*/59,	/* 12 4900.0*/62,	/* 13 5800.0*/59,	/* 14 6800.0*/63,	/* 15 8000.0*/62,	},
		{/* 5 46	*//* 0 700.0*/54,	/* 1 820.0*/61,	/* 2 950.0*/56,	/* 3 1100.0*/52,	/* 4 1300.0*/53,	/* 5 1550.0*/58,	/* 6 1800.0*/57,	/* 7 2150.0*/59,	/* 8 2500.0*/58,	/* 9 3000.0*/58,	/* 10 3500.0*/60,	/* 11 4150.0*/64,	/* 12 4900.0*/66,	/* 13 5800.0*/64,	/* 14 6800.0*/65,	/* 15 8000.0*/63,	},
		{/* 6 54	*//* 0 700.0*/60,	/* 1 820.0*/67,	/* 2 950.0*/66,	/* 3 1100.0*/60,	/* 4 1300.0*/59,	/* 5 1550.0*/59,	/* 6 1800.0*/61,	/* 7 2150.0*/63,	/* 8 2500.0*/63,	/* 9 3000.0*/60,	/* 10 3500.0*/62,	/* 11 4150.0*/69,	/* 12 4900.0*/71,	/* 13 5800.0*/67,	/* 14 6800.0*/65,	/* 15 8000.0*/63,	},
		{/* 7 63	*//* 0 700.0*/65,	/* 1 820.0*/70,	/* 2 950.0*/71,	/* 3 1100.0*/67,	/* 4 1300.0*/62,	/* 5 1550.0*/61,	/* 6 1800.0*/65,	/* 7 2150.0*/63,	/* 8 2500.0*/63,	/* 9 3000.0*/64,	/* 10 3500.0*/66,	/* 11 4150.0*/69,	/* 12 4900.0*/73,	/* 13 5800.0*/71,	/* 14 6800.0*/67,	/* 15 8000.0*/65,	},
		{/* 8 73	*//* 0 700.0*/70,	/* 1 820.0*/74,	/* 2 950.0*/73,	/* 3 1100.0*/75,	/* 4 1300.0*/71,	/* 5 1550.0*/66,	/* 6 1800.0*/66,	/* 7 2150.0*/65,	/* 8 2500.0*/67,	/* 9 3000.0*/69,	/* 10 3500.0*/68,	/* 11 4150.0*/72,	/* 12 4900.0*/76,	/* 13 5800.0*/75,	/* 14 6800.0*/66,	/* 15 8000.0*/65,	},
		{/* 9 85	*//* 0 700.0*/71,	/* 1 820.0*/75,	/* 2 950.0*/76,	/* 3 1100.0*/74,	/* 4 1300.0*/73,	/* 5 1550.0*/72,	/* 6 1800.0*/71,	/* 7 2150.0*/70,	/* 8 2500.0*/72,	/* 9 3000.0*/72,	/* 10 3500.0*/74,	/* 11 4150.0*/76,	/* 12 4900.0*/78,	/* 13 5800.0*/76,	/* 14 6800.0*/68,	/* 15 8000.0*/64,	},
		{/* 10 99	*//* 0 700.0*/75,	/* 1 820.0*/76,	/* 2 950.0*/78,	/* 3 1100.0*/76,	/* 4 1300.0*/73,	/* 5 1550.0*/74,	/* 6 1800.0*/74,	/* 7 2150.0*/74,	/* 8 2500.0*/77,	/* 9 3000.0*/76,	/* 10 3500.0*/77,	/* 11 4150.0*/76,	/* 12 4900.0*/77,	/* 13 5800.0*/76,	/* 14 6800.0*/69,	/* 15 8000.0*/65,	},
		{/* 11 116	*//* 0 700.0*/80,	/* 1 820.0*/80,	/* 2 950.0*/80,	/* 3 1100.0*/80,	/* 4 1300.0*/80,	/* 5 1550.0*/80,	/* 6 1800.0*/80,	/* 7 2150.0*/80,	/* 8 2500.0*/80,	/* 9 3000.0*/80,	/* 10 3500.0*/80,	/* 11 4150.0*/80,	/* 12 4900.0*/80,	/* 13 5800.0*/80,	/* 14 6800.0*/80,	/* 15 8000.0*/80,	},
		{/* 12 135	*//* 0 700.0*/80,	/* 1 820.0*/80,	/* 2 950.0*/80,	/* 3 1100.0*/80,	/* 4 1300.0*/80,	/* 5 1550.0*/80,	/* 6 1800.0*/80,	/* 7 2150.0*/80,	/* 8 2500.0*/80,	/* 9 3000.0*/80,	/* 10 3500.0*/80,	/* 11 4150.0*/80,	/* 12 4900.0*/80,	/* 13 5800.0*/80,	/* 14 6800.0*/80,	/* 15 8000.0*/80,	},
		{/* 13 158	*//* 0 700.0*/80,	/* 1 820.0*/80,	/* 2 950.0*/80,	/* 3 1100.0*/80,	/* 4 1300.0*/80,	/* 5 1550.0*/80,	/* 6 1800.0*/80,	/* 7 2150.0*/80,	/* 8 2500.0*/80,	/* 9 3000.0*/80,	/* 10 3500.0*/80,	/* 11 4150.0*/80,	/* 12 4900.0*/80,	/* 13 5800.0*/80,	/* 14 6800.0*/80,	/* 15 8000.0*/80,	},
		{/* 14 185	*//* 0 700.0*/80,	/* 1 820.0*/80,	/* 2 950.0*/80,	/* 3 1100.0*/80,	/* 4 1300.0*/80,	/* 5 1550.0*/80,	/* 6 1800.0*/80,	/* 7 2150.0*/80,	/* 8 2500.0*/80,	/* 9 3000.0*/80,	/* 10 3500.0*/80,	/* 11 4150.0*/80,	/* 12 4900.0*/80,	/* 13 5800.0*/80,	/* 14 6800.0*/80,	/* 15 8000.0*/80,	},
		{/* 15 220	*//* 0 700.0*/80,	/* 1 820.0*/80,	/* 2 950.0*/80,	/* 3 1100.0*/80,	/* 4 1300.0*/80,	/* 5 1550.0*/80,	/* 6 1800.0*/80,	/* 7 2150.0*/80,	/* 8 2500.0*/80,	/* 9 3000.0*/80,	/* 10 3500.0*/80,	/* 11 4150.0*/80,	/* 12 4900.0*/80,	/* 13 5800.0*/80,	/* 14 6800.0*/80,	/* 15 8000.0*/80,	},
};

#if IGN_LOAD_COUNT == DEFAULT_IGN_LOAD_COUNT
static const uint8_t mapBased18vvtTimingTable[16][16] = {
		/* Generated by TS2C on Tue Apr 18 21:43:57 EDT 2017*/
		{/* 0 25	*//* 0 700.0*/14,	/* 1 850.0*/13,	/* 2 943.0*/13,	/* 3 1112.0*/16,	/* 4 1310.0*/21,	/* 5 1545.0*/25,	/* 6 1821.0*/28,	/* 7 2146.0*/31,	/* 8 2530.0*/34,	/* 9 2982.0*/36,	/* 10 3515.0*/38,	/* 11 4144.0*/39,	/* 12 4884.0*/40,	/* 13 5757.0*/40,	/* 14 6787.0*/40,	/* 15 8000.0*/41,	},
		{/* 1 29.100	*//* 0 700.0*/14,	/* 1 850.0*/13,	/* 2 943.0*/13,	/* 3 1112.0*/16,	/* 4 1310.0*/21,	/* 5 1545.0*/25,	/* 6 1821.0*/28,	/* 7 2146.0*/31,	/* 8 2530.0*/34,	/* 9 2982.0*/36,	/* 10 3515.0*/38,	/* 11 4144.0*/39,	/* 12 4884.0*/40,	/* 13 5757.0*/40,	/* 14 6787.0*/40,	/* 15 8000.0*/40,	},
		{/* 2 34	*//* 0 700.0*/14,	/* 1 850.0*/13,	/* 2 943.0*/13,	/* 3 1112.0*/16,	/* 4 1310.0*/21,	/* 5 1545.0*/24,	/* 6 1821.0*/27,	/* 7 2146.0*/30,	/* 8 2530.0*/33,	/* 9 2982.0*/35,	/* 10 3515.0*/37,	/* 11 4144.0*/38,	/* 12 4884.0*/39,	/* 13 5757.0*/40,	/* 14 6787.0*/40,	/* 15 8000.0*/40,	},
		{/* 3 39.600	*//* 0 700.0*/15,	/* 1 850.0*/13,	/* 2 943.0*/13,	/* 3 1112.0*/17,	/* 4 1310.0*/21,	/* 5 1545.0*/24,	/* 6 1821.0*/27,	/* 7 2146.0*/30,	/* 8 2530.0*/33,	/* 9 2982.0*/35,	/* 10 3515.0*/36,	/* 11 4144.0*/38,	/* 12 4884.0*/38,	/* 13 5757.0*/39,	/* 14 6787.0*/39,	/* 15 8000.0*/39,	},
		{/* 4 46.200	*//* 0 700.0*/15,	/* 1 850.0*/13,	/* 2 943.0*/13,	/* 3 1112.0*/18,	/* 4 1310.0*/21,	/* 5 1545.0*/24,	/* 6 1821.0*/26,	/* 7 2146.0*/29,	/* 8 2530.0*/32,	/* 9 2982.0*/33,	/* 10 3515.0*/36,	/* 11 4144.0*/37,	/* 12 4884.0*/38,	/* 13 5757.0*/38,	/* 14 6787.0*/38,	/* 15 8000.0*/39,	},
		{/* 5 53.900	*//* 0 700.0*/15,	/* 1 850.0*/14,	/* 2 943.0*/14,	/* 3 1112.0*/18,	/* 4 1310.0*/21,	/* 5 1545.0*/24,	/* 6 1821.0*/26,	/* 7 2146.0*/28,	/* 8 2530.0*/30,	/* 9 2982.0*/32,	/* 10 3515.0*/34,	/* 11 4144.0*/36,	/* 12 4884.0*/37,	/* 13 5757.0*/37,	/* 14 6787.0*/38,	/* 15 8000.0*/38,	},
		{/* 6 62.800	*//* 0 700.0*/15,	/* 1 850.0*/15,	/* 2 943.0*/14,	/* 3 1112.0*/19,	/* 4 1310.0*/21,	/* 5 1545.0*/23,	/* 6 1821.0*/25,	/* 7 2146.0*/27,	/* 8 2530.0*/29,	/* 9 2982.0*/31,	/* 10 3515.0*/33,	/* 11 4144.0*/34,	/* 12 4884.0*/35,	/* 13 5757.0*/36,	/* 14 6787.0*/36,	/* 15 8000.0*/37,	},
		{/* 7 73.200	*//* 0 700.0*/16,	/* 1 850.0*/16,	/* 2 943.0*/15,	/* 3 1112.0*/19,	/* 4 1310.0*/21,	/* 5 1545.0*/23,	/* 6 1821.0*/24,	/* 7 2146.0*/26,	/* 8 2530.0*/28,	/* 9 2982.0*/30,	/* 10 3515.0*/31,	/* 11 4144.0*/32,	/* 12 4884.0*/33,	/* 13 5757.0*/34,	/* 14 6787.0*/34,	/* 15 8000.0*/35,	},
		{/* 8 85.400	*//* 0 700.0*/16,	/* 1 850.0*/17,	/* 2 943.0*/16,	/* 3 1112.0*/19,	/* 4 1310.0*/20,	/* 5 1545.0*/22,	/* 6 1821.0*/23,	/* 7 2146.0*/24,	/* 8 2530.0*/26,	/* 9 2982.0*/28,	/* 10 3515.0*/29,	/* 11 4144.0*/31,	/* 12 4884.0*/31,	/* 13 5757.0*/32,	/* 14 6787.0*/33,	/* 15 8000.0*/33,	},
		{/* 9 99.500	*//* 0 700.0*/16,	/* 1 850.0*/16,	/* 2 943.0*/17,	/* 3 1112.0*/18,	/* 4 1310.0*/19,	/* 5 1545.0*/20,	/* 6 1821.0*/21,	/* 7 2146.0*/22,	/* 8 2530.0*/23,	/* 9 2982.0*/25,	/* 10 3515.0*/26,	/* 11 4144.0*/28,	/* 12 4884.0*/28,	/* 13 5757.0*/29,	/* 14 6787.0*/30,	/* 15 8000.0*/31,	},
		{/* 10 116	*//* 0 700.0*/15,	/* 1 850.0*/15,	/* 2 943.0*/16,	/* 3 1112.0*/16,	/* 4 1310.0*/17,	/* 5 1545.0*/18,	/* 6 1821.0*/19,	/* 7 2146.0*/20,	/* 8 2530.0*/21,	/* 9 2982.0*/23,	/* 10 3515.0*/24,	/* 11 4144.0*/25,	/* 12 4884.0*/26,	/* 13 5757.0*/27,	/* 14 6787.0*/28,	/* 15 8000.0*/29,	},
		{/* 11 135.301	*//* 0 700.0*/13,	/* 1 850.0*/13,	/* 2 943.0*/14,	/* 3 1112.0*/14,	/* 4 1310.0*/15,	/* 5 1545.0*/15,	/* 6 1821.0*/17,	/* 7 2146.0*/17,	/* 8 2530.0*/19,	/* 9 2982.0*/20,	/* 10 3515.0*/22,	/* 11 4144.0*/23,	/* 12 4884.0*/24,	/* 13 5757.0*/25,	/* 14 6787.0*/26,	/* 15 8000.0*/27,	},
		{/* 12 157.699	*//* 0 700.0*/11,	/* 1 850.0*/11,	/* 2 943.0*/11,	/* 3 1112.0*/12,	/* 4 1310.0*/12,	/* 5 1545.0*/13,	/* 6 1821.0*/14,	/* 7 2146.0*/15,	/* 8 2530.0*/16,	/* 9 2982.0*/17,	/* 10 3515.0*/19,	/* 11 4144.0*/20,	/* 12 4884.0*/21,	/* 13 5757.0*/22,	/* 14 6787.0*/24,	/* 15 8000.0*/25,	},
		{/* 13 183.900	*//* 0 700.0*/8,	/* 1 850.0*/8,	/* 2 943.0*/9,	/* 3 1112.0*/9,	/* 4 1310.0*/9,	/* 5 1545.0*/10,	/* 6 1821.0*/11,	/* 7 2146.0*/12,	/* 8 2530.0*/13,	/* 9 2982.0*/14,	/* 10 3515.0*/16,	/* 11 4144.0*/17,	/* 12 4884.0*/18,	/* 13 5757.0*/19,	/* 14 6787.0*/21,	/* 15 8000.0*/22,	},
		{/* 14 214.400	*//* 0 700.0*/5,	/* 1 850.0*/5,	/* 2 943.0*/5,	/* 3 1112.0*/5,	/* 4 1310.0*/6,	/* 5 1545.0*/7,	/* 6 1821.0*/7,	/* 7 2146.0*/8,	/* 8 2530.0*/9,	/* 9 2982.0*/10,	/* 10 3515.0*/12,	/* 11 4144.0*/13,	/* 12 4884.0*/14,	/* 13 5757.0*/16,	/* 14 6787.0*/17,	/* 15 8000.0*/18,	},
		{/* 15 250	*//* 0 700.0*/1,	/* 1 850.0*/1,	/* 2 943.0*/1,	/* 3 1112.0*/2,	/* 4 1310.0*/2,	/* 5 1545.0*/3,	/* 6 1821.0*/3,	/* 7 2146.0*/4,	/* 8 2530.0*/5,	/* 9 2982.0*/6,	/* 10 3515.0*/7,	/* 11 4144.0*/9,	/* 12 4884.0*/10,	/* 13 5757.0*/12,	/* 14 6787.0*/13,	/* 15 8000.0*/14,	},
};
#endif


/*
#define MAF_TRANSFER_SIZE 8

static const float mafTransferVolts[MAF_TRANSFER_SIZE] = {1.365,
		1.569,
		2.028,
		2.35,
		2.611,
		2.959,
		3.499,
		4.011,
};


according to internet this should be the Miata NB transfer function but in reality it seems off
this could be related to us not using proper signal conditioning hardware
static const float mafTransferKgH[MAF_TRANSFER_SIZE] = {
		0,
		3.9456,
		18.7308,
		45.4788,
		82.278,
		154.4328,
		329.8104,
		594.2772
};


*/

#define MAF_TRANSFER_SIZE 10

// this transfer function somehow works with 1K pull-down
static const float mafTransferVolts[MAF_TRANSFER_SIZE] = {
		0.50,
		0.87,
		1.07,
		1.53,
		1.85,
		2.11,
		2.46,
		3.00,
		3.51,
		4.50
};

static const float mafTransferKgH[MAF_TRANSFER_SIZE] = {
		0.00,
		0.00,
		1.00,
		3.00,
		8.00,
		19.00,
		45.00,
		100.00,
		175.00,
		350.00
};

template <typename TSrc, typename TDst, size_t NSrc, size_t NDest>
constexpr void copyArrayPartial(TDst (&dest)[NDest], const TSrc (&src)[NSrc]) {
	static_assert(NDest >= NSrc, "Source array must be larger than destination.");

	for (size_t i = 0; i < NSrc; i++) {
		dest[i] = src[i];
	}
}

static void setMAFTransferFunction() {
	copyArrayPartial(config->mafDecoding, mafTransferKgH);
	copyArrayPartial(config->mafDecodingBins, mafTransferVolts);
	for (int i = MAF_TRANSFER_SIZE;i<MAF_DECODING_COUNT;i++) {
		config->mafDecodingBins[i] = config->mafDecodingBins[MAF_TRANSFER_SIZE - 1] + i * 0.01;
		config->mafDecoding[i] = config->mafDecoding[MAF_TRANSFER_SIZE - 1];
	}
}

static void setMazdaMiataNbInjectorLag() {
	static const float deadtimeBins[] = { 8, 9.6, 11.2, 12.8, 13.2, 14.4, 15.3, 16 };
	static const float deadtimeValues[] = { 1.97, 1.52, 1.23, 1.04, 0.99, 0.9, 0.85, 0.73 };
	copyArray(engineConfiguration->injector.battLagCorrBins, deadtimeBins);
	copyArray(engineConfiguration->injector.battLagCorr, deadtimeValues);
}

/**
 * stuff common between NB1 and NB2
 */
static void setCommonMazdaNB() {
	// Base engine
	engineConfiguration->displacement = 1.839;
	engineConfiguration->cylindersCount = 4;
	engineConfiguration->firingOrder = FO_1_3_4_2;

	engineConfiguration->rpmHardLimit = 7200;

	engineConfiguration->cylinderBore = 83;
	strcpy(engineConfiguration->engineMake, ENGINE_MAKE_MAZDA);
	engineConfiguration->vehicleWeight = 950;

	/**
	 * http://miataturbo.wikidot.com/fuel-injectors
	 * 01-05 (purple) - #195500-4060
	 */
	engineConfiguration->injector.flow = 265;
	engineConfiguration->fuelReferencePressure = 400; // 400 kPa, 58 psi
	engineConfiguration->injectorCompensationMode = ICM_FixedRailPressure;


	engineConfiguration->injectionMode = IM_SEQUENTIAL;
	engineConfiguration->ignitionMode = IM_WASTED_SPARK;

	static const float cltFuelMultValues[] = { 1.25, 1.23, 1.2, 1.18, 1.16, 1.14, 1.11, 1.09, 1.07, 1.05, 1.02, 1, 1, 1, 1, 1 };
	copyArray(config->cltFuelCorr, cltFuelMultValues);

	// Trigger
	engineConfiguration->trigger.type = trigger_type_e::TT_MIATA_VVT;
	engineConfiguration->globalTriggerAngleOffset = 10;
	engineConfiguration->vvtMode[0] = VVT_MIATA_NB;

	// Cranking
	engineConfiguration->ignitionDwellForCrankingMs = 4;
	engineConfiguration->cranking.baseFuel = 27.5; // this value for return-less NB miata fuel system, higher pressure
	engineConfiguration->cranking.rpm = 400;
	engineConfiguration->crankingIACposition = 60;
	engineConfiguration->afterCrankingIACtaperDuration = 250;

	engineConfiguration->primingDelay = 1;

	config->crankingFuelCoef[0] = 3.36;
	config->crankingFuelBins[0] = -20;
	config->crankingFuelCoef[1] = 2.97;
	config->crankingFuelBins[1] = -10;
	config->crankingFuelCoef[2] = 2.69;
	config->crankingFuelBins[2] = 5;
	config->crankingFuelCoef[3] = 2.15;
	config->crankingFuelBins[3] = 30;
	config->crankingFuelCoef[4] = 1.78;
	config->crankingFuelBins[4] = 40;
	config->crankingFuelCoef[5] = 1.47;
	config->crankingFuelBins[5] = 50;
	config->crankingFuelCoef[6] = 1.0;
	config->crankingFuelBins[6] = 65;
	config->crankingFuelCoef[7] = 1.0;
	config->crankingFuelBins[7] = 90;

	// Idle
	engineConfiguration->idle.solenoidFrequency = 500;
	engineConfiguration->idleMode = IM_AUTO;
	engineConfiguration->manIdlePosition = 20;
	engineConfiguration->iacByTpsTaper = 6;
	engineConfiguration->acIdleExtraOffset = 15;

	engineConfiguration->useIdleTimingPidControl = true;
	engineConfiguration->idlePidRpmUpperLimit = 350;
	engineConfiguration->idlePidRpmDeadZone = 100;

	engineConfiguration->idleRpmPid.pFactor = 0.0065;
	engineConfiguration->idleRpmPid.iFactor = 0.3;
	engineConfiguration->idle_derivativeFilterLoss = 0.08;
	engineConfiguration->idle_antiwindupFreq = 0.03;
	engineConfiguration->idleRpmPid.dFactor = 0.002;
	engineConfiguration->idleRpmPid.minValue = -8;
	engineConfiguration->idleRpmPid.maxValue = 10;
	engineConfiguration->idlerpmpid_iTermMin = -15;
	engineConfiguration->idlerpmpid_iTermMax =  30;

	// Idle timing
	static const uint16_t idleAdvanceBins[] = { 500, 650, 800, 950, 1050, 1200, 1350, 1500 };
	copyArray(config->idleAdvanceBins, idleAdvanceBins);
	setArrayValues(config->idleAdvance, 15);

	// Fan
	engineConfiguration->enableFan1WithAc = true;

	// Alternator
	engineConfiguration->isAlternatorControlEnabled = true;
	engineConfiguration->targetVBatt = 14.0f;
	engineConfiguration->alternatorControl.offset = 20;
	engineConfiguration->alternatorControl.pFactor = 20;
	engineConfiguration->alternatorControl.iFactor = 8;
	engineConfiguration->alternatorControl.dFactor = 0.1;
	engineConfiguration->alternatorControl.periodMs = 10;

	// AC
	engineConfiguration->acDelay = 0.5;
	engineConfiguration->maxAcClt = 105;
	engineConfiguration->acIdleExtraOffset = 8;
	engineConfiguration->acIdleRpmBump = 150;

	// Tach
	engineConfiguration->tachPulsePerRev = 2;

	// Tables
	copyArray(config->veRpmBins, mazda_miata_nb2_RpmBins);
	copyArray(config->veLoadBins, mazda_miata_nb2_LoadBins);
	copyTable(config->veTable, mapBased18vvtVeTable_NB_fuel_rail);

	copyArray(config->ignitionRpmBins, ignition18vvtRpmBins);
	copyArray(config->ignitionLoadBins, ignition18vvtLoadBins);
#if IGN_LOAD_COUNT == DEFAULT_IGN_LOAD_COUNT
	copyTable(config->ignitionTable, mapBased18vvtTimingTable);
#endif

	setMazdaMiataNbInjectorLag();

	// Sensors

	// TPS
	// set tps_min 90
	engineConfiguration->tpsMin = 100; // convert 12to10 bit (ADC/4)
	// set tps_max 540
	engineConfiguration->tpsMax = 650; // convert 12to10 bit (ADC/4)

	// CLT/IAT
	engineConfiguration->clt.config = { -10, 20, 80, 9000, 2500, 320, 0 };
	engineConfiguration->iat.config = { -20, 40, 80, 16150, 1150, 330, 0 };

	// MAF (todo: do we use this?)
	setMAFTransferFunction();

	// second harmonic (aka double) is usually quieter background noise
	engineConfiguration->knockBandCustom = 13.8;

	engineConfiguration->wwaeTau = 0.1;

	miataNA_setCltIdleCorrBins();
	miataNA_setCltIdleRpmBins();
	miataNA_setIacCoastingBins();

	// All factory miata setups end up with 1.12 speed sensor turns
	// per wheel turn, by matching the speedo sensor gear to the
	// diff ratio

	// - 6 teeth on transmission output shaft
	// - 23 teeth on speedometer sensor
	// - 3.909 rear axle ratio
	// 3.909 * 6 / 21 ~= 1.12
	engineConfiguration->vssGearRatio = 3.909 * 6 / 21;
	engineConfiguration->vssToothCount = 4;
}

static void setMazdaMiataEngineNB1Defaults() {
	setCommonMazdaNB();

	strcpy(engineConfiguration->engineCode, "NB1");
	engineConfiguration->compressionRatio = 9.5;

	// Vehicle speed/gears
	engineConfiguration->totalGearsCount = 5;
	engineConfiguration->gearRatio[0] = 3.136;
	engineConfiguration->gearRatio[1] = 1.888;
	engineConfiguration->gearRatio[2] = 1.330;
	engineConfiguration->gearRatio[3] = 1.000;
	engineConfiguration->gearRatio[4] = 0.814;

	// These may need to change based on your real car
	engineConfiguration->driveWheelRevPerKm = 551;
	engineConfiguration->finalGearRatio = 4.3;
}

static void setMazdaMiataEngineNB2Defaults() {
	setCommonMazdaNB();

	strcpy(engineConfiguration->engineCode, "NB2");
	engineConfiguration->compressionRatio = 10;

	copyArray(config->vvtTable1RpmBins, vvt18fsioRpmBins);
	copyArray(config->vvtTable1LoadBins, vvt18fsioLoadBins);
	copyTable(config->vvtTable1, SCRIPT_TABLE_vvt_target);

	engineConfiguration->vvtOffsets[0] = 95;

	// VVT closed loop
	engineConfiguration->auxPid[0].pFactor = 2;
	engineConfiguration->auxPid[0].iFactor = 0.005;
	engineConfiguration->auxPid[0].dFactor = 0.002;
	engineConfiguration->auxPid[0].offset = 33;
	engineConfiguration->auxPid[0].minValue = 10;
	engineConfiguration->auxPid[0].maxValue = 90;
	engineConfiguration->vvtControlMinRpm = 400;

	// Vehicle speed/gears
	engineConfiguration->totalGearsCount = 6;
	engineConfiguration->gearRatio[0] = 3.760;
	engineConfiguration->gearRatio[1] = 2.269;
	engineConfiguration->gearRatio[2] = 1.646;
	engineConfiguration->gearRatio[3] = 1.257;
	engineConfiguration->gearRatio[4] = 1.000;
	engineConfiguration->gearRatio[5] = 0.843;

	// These may need to change based on your real car
	engineConfiguration->driveWheelRevPerKm = 538;
	engineConfiguration->finalGearRatio = 3.909;
}

// MAZDA_MIATA_2003
void setMazdaMiata2003EngineConfiguration() {
	setFrankensoConfiguration();

	setMazdaMiataEngineNB2Defaults();

//	engineConfiguration->triggerInputPins[0] = Gpio::A8; // custom Frankenso wiring in order to use SPI1 for accelerometer
	engineConfiguration->triggerInputPins[0] = Gpio::A5; // board still not modified
	engineConfiguration->triggerInputPins[1] = Gpio::Unassigned;
	engineConfiguration->camInputs[0] = Gpio::C6;

//	engineConfiguration->is_enabled_spi_1 = true;

	engineConfiguration->alternatorControlPin = Gpio::E10;
	engineConfiguration->alternatorControlPinMode = OM_OPENDRAIN;

//	engineConfiguration->vehicleSpeedSensorInputPin = Gpio::A8;

	engineConfiguration->vvtPins[0] = Gpio::E3; // VVT solenoid control

	// high-side driver with +12v VP jumper
	engineConfiguration->tachOutputPin = Gpio::E8; // tachometer

	// enable trigger_details
	engineConfiguration->verboseTriggerSynchDetails = false;

	// set cranking_timing_angle 10
	engineConfiguration->crankingTimingAngle = 10;

/**
 * Saab attempt
 * Saab  coil on #1 PD8 extra blue wire
 * Miata coil on #2 PC9  - orange ECU wire "2&3"
 * Saab  coil on #3 PD9 extra white wire
 * Miata coil on #4 PE14 - white ECU wire "1&4"
 */

	engineConfiguration->ignitionPins[0] = Gpio::E14;
	engineConfiguration->ignitionPins[1] = Gpio::Unassigned;
	engineConfiguration->ignitionPins[2] = Gpio::C9;
	engineConfiguration->ignitionPins[3] = Gpio::Unassigned;



	engineConfiguration->malfunctionIndicatorPin = Gpio::D5;


//	engineConfiguration->malfunctionIndicatorPin = Gpio::D9;
//	engineConfiguration->malfunctionIndicatorPinMode = OM_INVERTED;

	// todo: blue jumper wire - what is it?!
	// Frankenso analog #6 pin 3R, W56 (5th lower row pin from the end) top <> W45 bottom jumper, not OEM


	// see setFrankensoConfiguration
	// map.sensor.hwChannel = EFI_ADC_0; W53

	/**
	 * PA4 Wideband O2 Sensor
	 */
	// todo: re-wire the board to use "Frankenso analog #7 pin 3J, W48 top <>W48 bottom jumper, not OEM"
	//engineConfiguration->afr.hwChannel = EFI_ADC_3; // PA3
	engineConfiguration->afr.hwChannel = EFI_ADC_4;

	//
	/**
	 * Combined RPM, CLT and VBATT warning light
	 *
	 * to test
	 * set_fsio_setting 2 1800
	 * set_fsio_setting 3 65
	 * set_fsio_setting 4 15
	 */
	engineConfiguration->scriptSetting[1] = 6500; // #2 RPM threshold
	engineConfiguration->scriptSetting[2] = 105; // #3 CLT threshold
	engineConfiguration->scriptSetting[3] = 12.0; // #4 voltage threshold

	// enable auto_idle
	// set idle_p 0.05
	// set idle_i 0
	// set idle_d 0
	// set debug_mode 3
	// set idle_rpm 1700
	// see setDefaultIdleParameters

	engineConfiguration->adcVcc = 3.3f;
	engineConfiguration->vbattDividerCoeff = 8.80f;

	// by the way NB2 MAF internal diameter is about 2.5 inches / 63mm
	// 1K pull-down to read current from this MAF
	engineConfiguration->mafAdcChannel = EFI_ADC_6; // PA6 W46 <> W46

	engineConfiguration->throttlePedalUpVoltage = 0.65f;


	// TLE7209 two-wire ETB control
	// PWM
	engineConfiguration->etb_use_two_wires = true;

	engineConfiguration->etbIo[0].controlPin = Gpio::Unassigned;

	//
	engineConfiguration->etbIo[0].directionPin1 = Gpio::E12; // orange
	//
	engineConfiguration->etbIo[0].directionPin2 = Gpio::C7; // white/blue

	// set_analog_input_pin tps PC3
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_13; // PC3 blue

	// set_analog_input_pin pps PA2
/* a step back - Frankenso does not use ETB
	engineConfiguration->throttlePedalPositionAdcChannel = EFI_ADC_2;
*/

	//set etb_p 12
	engineConfiguration->etb.pFactor = 12; // a bit lower p-factor seems to work better on TLE9201? MRE?
	engineConfiguration->etb.iFactor = 	0;
	engineConfiguration->etb.dFactor = 0;
	engineConfiguration->etb.offset = 40;
	engineConfiguration->etb.minValue = -60;
	engineConfiguration->etb.maxValue = 50;
}

/**
 * red car setting with default 1991/1995 miata harness
 * board #70 - closer to default miata NA6 harness
 *
 */
void setMazdaMiata2003EngineConfigurationBoardTest() {
	setMazdaMiata2003EngineConfiguration();

	engineConfiguration->ignitionPins[2] = Gpio::C7;

	// Frankenso analog #7 pin 3J, W48 top <>W48 bottom jumper, not OEM. Make sure 500K pull-down on Frankenso
	engineConfiguration->afr.hwChannel = EFI_ADC_3; // PA3

	engineConfiguration->mafAdcChannel = EFI_ADC_4; // PA4 - W47 top <>W47
}

static void setMiataNB2_MRE_common() {
	setMazdaMiataEngineNB2Defaults();
#if (BOARD_TLE8888_COUNT > 0)

	// MRE has a special main relay control low side pin - rusEfi firmware is totally not involved with main relay control
	//
	// fuelPumpPin output is inherited from boards/microrusefi/board_configuration.cpp
	// fanPin output is inherited from boards/microrusefi/board_configuration.cpp
	//
	// crank trigger input is inherited from boards/microrusefi/board_configuration.cpp
	// map.sensor.hwChannel input is inherited from boards/microrusefi/board_configuration.cpp
	// tps1_1AdcChannel input is inherited from boards/microrusefi/board_configuration.cpp
	// afr.hwChannel input is inherited from boards/microrusefi/board_configuration.cpp

	engineConfiguration->ignitionPins[1] = Gpio::Unassigned;
	engineConfiguration->ignitionPins[3] = Gpio::Unassigned;

	engineConfiguration->camInputs[0] = Gpio::A5;
	/**
	 * By default "auto detection mode for VR sensor signals" is used
	 * We know that for short & strange Hall (?) signals like Miata NB2 crank sensor this does not work well above certain RPM.
	 */
	engineConfiguration->tle8888mode = TL_MANUAL;

	// Gpio::D6: "13 - GP Out 6" - selected to +12v
	engineConfiguration->alternatorControlPin = Gpio::D6;
	// Gpio::D7: "14 - GP Out 5" - selected to +12v
	engineConfiguration->tachOutputPin = Gpio::D7; // tachometer
	engineConfiguration->tachPulsePerRev = 2;

	engineConfiguration->isSdCardEnabled = true;

	engineConfiguration->ignitionDwellForCrankingMs = 8;

	//   # TLE8888 high current low side: VVT1 IN10 / OUT6
	// Gpio::TLE8888_PIN_6:  "7 - Lowside 1"
	engineConfiguration->vvtPins[0] = Gpio::TLE8888_PIN_6; // VVT solenoid control

	// Gpio::TLE8888_PIN_23: "33 - GP Out 3"
	engineConfiguration->malfunctionIndicatorPin = Gpio::TLE8888_PIN_23;


	// todo: alternator warn
	// ?

	// todo: AC fan
	// Gpio::TLE8888_PIN_24: "43 - GP Out 4"

	// set_analog_input_pin pps PA7
	// EFI_ADC_7: "31 - AN volt 3" - PA7
// disabled for now since only allowed with ETB
//	engineConfiguration->throttlePedalPositionAdcChannel = EFI_ADC_7;

	// set tps_min 90
	engineConfiguration->tpsMin = 90;

	// set tps_max 540
	engineConfiguration->tpsMax = 870;

	// 0.3#4 has wrong R139? TODO: fix that custom board to match proper value!!!
	// set vbatt_divider 10.956
	// 56k high side/10k low side multiplied by analogInputDividerCoefficient
	// vbattDividerCoeff = 10.956 (66.0f / 10.0f) * engineConfiguration->analogInputDividerCoefficient;
#endif /* BOARD_TLE8888_COUNT */
}

/**
 * https://github.com/rusefi/rusefi/wiki/HOWTO-TCU-A42DE-on-Proteus
 */
#if HW_PROTEUS
void setMiataNB2_Proteus_TCU() {
	engineConfiguration->tcuEnabled = true;

	strcpy(engineConfiguration->engineCode, "NB2");
	strcpy(engineConfiguration->engineMake, ENGINE_MAKE_MAZDA);
	strcpy(engineConfiguration->vehicleName, "TCU test");

	engineConfiguration->trigger.type = trigger_type_e::TT_TOOTHED_WHEEL;
	engineConfiguration->trigger.customTotalToothCount = 10;
	engineConfiguration->trigger.customSkippedToothCount = 0;


	engineConfiguration->triggerInputPins[0] = Gpio::Unassigned;
	engineConfiguration->tcuInputSpeedSensorPin = PROTEUS_VR_1;

	engineConfiguration->vehicleSpeedSensorInputPin = PROTEUS_VR_2;

	engineConfiguration->driveWheelRevPerKm = 544;	// 205/50R15
	engineConfiguration->vssGearRatio = 4.3;
	engineConfiguration->vssToothCount = 22;

	// "Highside 2"
	engineConfiguration->tcu_solenoid[0] = Gpio::A8;
	// "Highside 1"
	engineConfiguration->tcu_solenoid[1] = Gpio::A9;

	// "Digital 1" green
	engineConfiguration->tcuUpshiftButtonPin = Gpio::C6;
	engineConfiguration->tcuUpshiftButtonPinMode = PI_PULLUP;
	// "Digital 6" white
	engineConfiguration->tcuDownshiftButtonPin = Gpio::E15;
	engineConfiguration->tcuDownshiftButtonPinMode = PI_PULLUP;

	// R
	config->tcuSolenoidTable[0][0] = 1;
	config->tcuSolenoidTable[0][1] = 0;
	// P/N
	config->tcuSolenoidTable[1][0] = 1;
	config->tcuSolenoidTable[1][1] = 0;
	// 1
	config->tcuSolenoidTable[2][0] = 1;
	config->tcuSolenoidTable[2][1] = 0;
	// 2
	config->tcuSolenoidTable[3][0] = 1;
	config->tcuSolenoidTable[3][1] = 1;
	// 3
	config->tcuSolenoidTable[4][0] = 0;
	config->tcuSolenoidTable[4][1] = 1;
	// 4
	config->tcuSolenoidTable[5][0] = 0;
	config->tcuSolenoidTable[5][1] = 0;

}

void setMiataNbPolygonusCommon() {
	engineConfiguration->triggerInputPins[0] = PROTEUS_VR_1;
	engineConfiguration->vehicleSpeedSensorInputPin = PROTEUS_DIGITAL_3;

	engineConfiguration->alternatorControlPin = PROTEUS_HS_1;

	// high-side driver with +12v VP jumper
	engineConfiguration->tachOutputPin = PROTEUS_LS_14; // tachometer
	engineConfiguration->tachPulsePerRev = 2;

	engineConfiguration->ignitionMode = IM_WASTED_SPARK;

	engineConfiguration->ignitionPins[0] = PROTEUS_IGN_1;
	engineConfiguration->ignitionPins[1] = PROTEUS_IGN_2;
	engineConfiguration->ignitionPins[2] = PROTEUS_IGN_3;
	engineConfiguration->ignitionPins[3] = PROTEUS_IGN_4;

	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_SEQUENTIAL;


	engineConfiguration->injectionPins[0] = PROTEUS_LS_1;  // BLU  # pin 3/black35
	engineConfiguration->injectionPins[1] = PROTEUS_LS_2;  // BLK
	engineConfiguration->injectionPins[2] = PROTEUS_LS_3; // GRN
	engineConfiguration->injectionPins[3] = PROTEUS_LS_4; // WHT

	engineConfiguration->enableSoftwareKnock = true;
	engineConfiguration->cylinderBore = 78;
	engineConfiguration->knockBandCustom = 6.8;

	engineConfiguration->malfunctionIndicatorPin = PROTEUS_LS_11;

	engineConfiguration->map.sensor.hwChannel = PROTEUS_IN_ANALOG_VOLT_1;
	engineConfiguration->map.sensor.type = MT_MPXH6400;


	engineConfiguration->tps1_1AdcChannel = PROTEUS_IN_ANALOG_VOLT_3;

	engineConfiguration->clt.adcChannel =  PROTEUS_IN_ANALOG_TEMP_3;
	engineConfiguration->iat.adcChannel = PROTEUS_IN_ANALOG_TEMP_2;

	engineConfiguration->fuelPumpPin = PROTEUS_LS_7;

	engineConfiguration->idle.solenoidPin = PROTEUS_LS_10;

	// Built in wideband controller on bus 2
	engineConfiguration->enableAemXSeries = true;
	engineConfiguration->widebandOnSecondBus = true;
	engineConfiguration->enableVerboseCanTx = true;

	engineConfiguration->fanPin = PROTEUS_LS_6;
	engineConfiguration->fanOnTemperature = 90;
	engineConfiguration->fanOffTemperature = 84;

	engineConfiguration->fan2Pin = PROTEUS_LS_5;
	engineConfiguration->enableFan2WithAc = true;

	engineConfiguration->clutchDownPin = PROTEUS_DIGITAL_4;
	engineConfiguration->clutchDownPinInverted = true;

#if EFI_PROD_CODE
	engineConfiguration->brakePedalPin = getAdcChannelBrainPin("", PROTEUS_IN_ANALOG_VOLT_5);
#endif // EFI_PROD_CODE

	engineConfiguration->acRelayPin = PROTEUS_LS_8;

	// Disable ETBs
	engineConfiguration->etbFunctions[0] = dc_function_e::DC_None;
	engineConfiguration->etbFunctions[1] = dc_function_e::DC_None;
}

void setMiataNB1_Polygonus() {
	setMazdaMiataEngineNB1Defaults();
	setMiataNbPolygonusCommon();

	engineConfiguration->camInputs[0] = PROTEUS_VR_2;

	engineConfiguration->tpsMin = 102;
	engineConfiguration->tpsMax = 727;

	engineConfiguration->acSwitch = PROTEUS_DIGITAL_6;

	// Fuel pressure
	engineConfiguration->lowPressureFuel.hwChannel = PROTEUS_IN_ANALOG_VOLT_6;
	engineConfiguration->lowPressureFuel.v1 = 0.5;
	engineConfiguration->lowPressureFuel.value1 = 0;
	engineConfiguration->lowPressureFuel.v2 = 4.5;
	engineConfiguration->lowPressureFuel.value2 = 689.5;

	// GPPWM1: VICS variable intake flap
	engineConfiguration->gppwm[0].pin = PROTEUS_LS_16;
	engineConfiguration->gppwm[0].pwmFrequency = 0;
	engineConfiguration->gppwm[0].loadAxis = GPPWM_Tps;
	engineConfiguration->gppwm[0].onAboveDuty = 60;
	engineConfiguration->gppwm[0].offBelowDuty = 40;
	strcpy(engineConfiguration->gpPwmNote[0], "VICS");
	copyArray(engineConfiguration->gppwm[0].rpmBins, { 0, 1000, 2000, 2500, 3500, 5500, 6500, 7000 });
	for (size_t i = 0; i < efi::size(engineConfiguration->gppwm[0].table); i++)
	{
		// Set the 3500 and 5500 rpm columns to 100
		engineConfiguration->gppwm[0].table[i][4] = 100;
		engineConfiguration->gppwm[0].table[i][5] = 100;
	}
}

// Both NB2 and MSM
void setMiataPolygonusNB2Common() {
	engineConfiguration->camInputs[0] = PROTEUS_DIGITAL_1;

	engineConfiguration->tpsMin = 92;
	engineConfiguration->tpsMax = 872;

	// NB2 has a main relay output, unlike NB1
	engineConfiguration->mainRelayPin = PROTEUS_LS_16;

#if EFI_PROD_CODE
	engineConfiguration->acSwitch = getAdcChannelBrainPin("", PROTEUS_IN_ANALOG_VOLT_6);
#endif // EFI_PROD_CODE
}

void setMiataNB2_Polygonus() {
	setMazdaMiataEngineNB2Defaults();
	setMiataNbPolygonusCommon();
	setMiataPolygonusNB2Common();

	// NB2 has VVT!
	engineConfiguration->vvtPins[0] = PROTEUS_LS_13;
}

void setMiataNB_MSM_Polygonus() {
	setMazdaMiataEngineNB1Defaults();
	setMiataNbPolygonusCommon();
	setMiataPolygonusNB2Common();

	strcpy(engineConfiguration->engineCode, "NB1");
	engineConfiguration->compressionRatio = 9.5;
	engineConfiguration->isForcedInduction = true;

	// MSM has boost control!
	engineConfiguration->boostControlPin = PROTEUS_HS_3;

	// MSM gets a different IAT from the other NBs
	engineConfiguration->iat.config = { 0, 40, 60, 112000, 17000, 8000, 0 };
}
#endif // HW_PROTEUS

#if HW_HELLEN
void setHellenNB1() {
	setMazdaMiataEngineNB1Defaults();

	engineConfiguration->injector.flow = 256;
}

void setMiataNB2_Hellen72() {
    setMazdaMiataEngineNB2Defaults();
	strcpy(engineConfiguration->vehicleName, "H72 test");


	// set tps_min 90
	engineConfiguration->tpsMin = 110; // convert 12to10 bit (ADC/4)

}

void setMiataNB2_Hellen72_36() {
	setMiataNB2_Hellen72();

	engineConfiguration->trigger.type = trigger_type_e::TT_TOOTHED_WHEEL_36_1;
	engineConfiguration->globalTriggerAngleOffset = 76;
}

#endif // HW_HELLEN
