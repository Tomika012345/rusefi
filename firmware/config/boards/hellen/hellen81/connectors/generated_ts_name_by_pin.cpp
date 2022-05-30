//DO NOT EDIT MANUALLY, let automation work hard.

// auto-generated by PinoutLogic.java based on  config/boards/hellen/hellen81/connectors/main.yaml
#include "pch.h"

// see comments at declaration in pin_repository.h
const char * getBoardSpecificPinName(brain_pin_e brainPin) {
	switch(brainPin) {
		case Gpio::A6: return "79 - IN_CAM";
		case Gpio::A7: return "77 - IN_AUX4";
		case Gpio::A9: return "28 - OUT_O2H2";
		case Gpio::B0: return "74 - IN_AUX1";
		case Gpio::B1: return "15 - IN_CRANK (A24)";
		case Gpio::B8: return "23 - IGN_6";
		case Gpio::B9: return "22 - IGN_7";
		case Gpio::C4: return "75 - IN_AUX2";
		case Gpio::C5: return "76 - IN_AUX3";
		case Gpio::C6: return "67 - OUT_COIL_A1";
		case Gpio::C7: return "66 - OUT_COIL_A2";
		case Gpio::C8: return "65 - OUT_COIL_B1";
		case Gpio::C9: return "64 - OUT_COIL_B2";
		case Gpio::D10: return "47 - INJ_4";
		case Gpio::D11: return "7 - INJ_3";
		case Gpio::D12: return "68 - OUT_ECF";
		case Gpio::D13: return "60 - OUT_IO";
		case Gpio::D14: return "8 - OUT_TACH";
		case Gpio::D15: return "10 - OUT_FUEL";
		case Gpio::D9: return "30 - INJ_5";
		case Gpio::E2: return "24 - IGN_5";
		case Gpio::E3: return "4 - IGN_4";
		case Gpio::E4: return "2 - IGN_3";
		case Gpio::E5: return "1 - IGN_2";
		case Gpio::E6: return "21 - IGN_8";
		case Gpio::F11: return "59 - IN_VSS";
		case Gpio::F12: return "25 - INJ_6";
		case Gpio::F13: return "72 - INJ_7";
		case Gpio::F14: return "73 - INJ_8";
		case Gpio::F5: return "9 - IN_SENS3";
		case Gpio::G2: return "70 - OUT_PUMP";
		case Gpio::G3: return "46 - OUT_CANIST";
		case Gpio::G4: return "31 - OUT_CE";
		case Gpio::G7: return "27 - INJ_1";
		case Gpio::G8: return "6 - INJ_2";
		case Gpio::H13: return "11 - OUT_HIGH";
		case Gpio::H14: return "48 - OUT_O2H";
		case Gpio::H15: return "69 - OUT_AC";
		case Gpio::I0: return "50 - OUT_STARTER";
		case Gpio::I1: return "49 - OUT_HEATER";
		case Gpio::I2: return "14 - OUT_MAIN";
		case Gpio::I8: return "5 - IGN_1";
		default: return nullptr;
	}
	return nullptr;
}
