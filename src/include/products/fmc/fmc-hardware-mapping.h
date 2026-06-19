#ifndef FMC_HARDWARE_MAPPING_H
#define FMC_HARDWARE_MAPPING_H

#include <string>
#include <variant>
#include <vector>

enum class FMCHardwareType : unsigned char {
    HARDWARE_MCDU = 1,
    HARDWARE_PFP3N,
    HARDWARE_PFP4,
    HARDWARE_PFP7,
};

enum class FMCDeviceVariant : unsigned char {
    VARIANT_CAPTAIN = 0x00,
    VARIANT_FIRSTOFFICER = 0x02,
    VARIANT_OBSERVER = 0x01
};

enum class FMCKey : unsigned char {
    INVALID_UNKNOWN = 0,
    LSK1L = 1,
    LSK1R,
    LSK2L,
    LSK2R,
    LSK3L,
    LSK3R,
    LSK4L,
    LSK4R,
    LSK5L,
    LSK5R,
    LSK6L,
    LSK6R,

    BRIGHTNESS_DOWN,
    BRIGHTNESS_UP,
    PAGE_NEXT,
    PAGE_PREV,
    MENU,
    PROG,

    SLASH,
    PERIOD,
    PLUSMINUS,
    SPACE,
    CLR,
    KEY0,
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY5,
    KEY6,
    KEY7,
    KEY8,
    KEY9,
    KEYA,
    KEYB,
    KEYC,
    KEYD,
    KEYE,
    KEYF,
    KEYG,
    KEYH,
    KEYI,
    KEYJ,
    KEYK,
    KEYL,
    KEYM,
    KEYN,
    KEYO,
    KEYP,
    KEYQ,
    KEYR,
    KEYS,
    KEYT,
    KEYU,
    KEYV,
    KEYW,
    KEYX,
    KEYY,
    KEYZ,

    PFP_INIT_REF,
    PFP_ROUTE,
    PFP_LEGS,
    PFP_DEP_ARR,
    PFP_HOLD,
    PFP_FIX,
    PFP_EXEC,
    PFP_DEL,

    PFP3_CLB,
    PFP3_CRZ,
    PFP3_DES,
    PFP3_N1_LIMIT,

    PFP4_ATC,
    PFP4_VNAV,
    PFP4_FMC_COMM,
    PFP4_NAV_RAD,

    PFP7_ALTN,
    PFP7_VNAV,
    PFP7_FMC_COMM,
    PFP7_NAV_RAD,

    MCDU_DIR,
    MCDU_PERF,
    MCDU_INIT,
    MCDU_DATA,
    MCDU_EMPTY_TOP_RIGHT,
    MCDU_FPLN,
    MCDU_RAD_NAV,
    MCDU_FUEL_PRED,
    MCDU_SEC_FPLN,
    MCDU_ATC_COMM,
    MCDU_AIRPORT,
    MCDU_EMPTY_BOTTOM_LEFT,
    MCDU_PAGE_UP,
    MCDU_PAGE_DOWN,
    MCDU_OVERFLY,
};

enum class FMCDatarefType : unsigned char {
    EXECUTE_CMD_ONCE = 1,
    EXECUTE_MULTIPLE_CMD_ONCE,
    EXECUTE_CMD_PHASED,
    SET_VALUE,
    SET_VALUE_PHASED,
    ADJUST_VALUE
};

struct FMCButtonDef {
        std::variant<FMCKey, std::vector<FMCKey>> key;
        std::string dataref;
        FMCDatarefType datarefType = FMCDatarefType::EXECUTE_CMD_PHASED;
        double value = 0.0;
};

// SimAppPro "Screen Layout Settings" per device: Character Size (width x height of
// each character) and Screen Position (top-left x/y). The FMC applies the entry for
// the connected hardware whenever a font is loaded, so the 14 display rows line up
// with the physical LSK keys.
struct FMCScreenLayout {
    unsigned char characterHeight;
    unsigned char characterWidth;
    unsigned char x;
    unsigned char y;
};

class FMCHardwareMapping {
    public:
        static FMCScreenLayout ScreenLayoutForHardware(FMCHardwareType hardwareType) {
            switch (hardwareType) {
                case FMCHardwareType::HARDWARE_MCDU:  return {29, 23, 16, 17};
                case FMCHardwareType::HARDWARE_PFP3N: return {32, 23, 14, 12};
                case FMCHardwareType::HARDWARE_PFP4:  return {32, 23, 14, 12};
                case FMCHardwareType::HARDWARE_PFP7:  return {32, 23, 14, 12};
                default:                              return {29, 23, 16, 17};
            }
        }

        static FMCKey ButtonIdentifierForIndex(FMCHardwareType hardwareType, int index) {
            switch (hardwareType) {
                case FMCHardwareType::HARDWARE_MCDU:
                    return getMCDUButtonForIndex(index);
                case FMCHardwareType::HARDWARE_PFP3N:
                    return getPFP3NButtonForIndex(index);
                case FMCHardwareType::HARDWARE_PFP4:
                    return getPFP4ButtonForIndex(index);
                case FMCHardwareType::HARDWARE_PFP7:
                    return getPFP7ButtonForIndex(index);
                default:
                    return FMCKey::INVALID_UNKNOWN;
            }
        }

    private:
        static FMCKey getMCDUButtonForIndex(int index) {
            switch (index) {
                case 0:
                    return FMCKey::LSK1L;
                case 1:
                    return FMCKey::LSK2L;
                case 2:
                    return FMCKey::LSK3L;
                case 3:
                    return FMCKey::LSK4L;
                case 4:
                    return FMCKey::LSK5L;
                case 5:
                    return FMCKey::LSK6L;
                case 6:
                    return FMCKey::LSK1R;
                case 7:
                    return FMCKey::LSK2R;
                case 8:
                    return FMCKey::LSK3R;
                case 9:
                    return FMCKey::LSK4R;
                case 10:
                    return FMCKey::LSK5R;
                case 11:
                    return FMCKey::LSK6R;
                case 12:
                    return FMCKey::MCDU_DIR;
                case 13:
                    return FMCKey::PROG;
                case 14:
                    return FMCKey::MCDU_PERF;
                case 15:
                    return FMCKey::MCDU_INIT;
                case 16:
                    return FMCKey::MCDU_DATA;
                case 17:
                    return FMCKey::MCDU_EMPTY_TOP_RIGHT;
                case 18:
                    return FMCKey::BRIGHTNESS_UP;
                case 19:
                    return FMCKey::MCDU_FPLN;
                case 20:
                    return FMCKey::MCDU_RAD_NAV;
                case 21:
                    return FMCKey::MCDU_FUEL_PRED;
                case 22:
                    return FMCKey::MCDU_SEC_FPLN;
                case 23:
                    return FMCKey::MCDU_ATC_COMM;
                case 24:
                    return FMCKey::MENU;
                case 25:
                    return FMCKey::BRIGHTNESS_DOWN;
                case 26:
                    return FMCKey::MCDU_AIRPORT;
                case 27:
                    return FMCKey::MCDU_EMPTY_BOTTOM_LEFT;
                case 28:
                    return FMCKey::PAGE_PREV;
                case 29:
                    return FMCKey::MCDU_PAGE_UP;
                case 30:
                    return FMCKey::PAGE_NEXT;
                case 31:
                    return FMCKey::MCDU_PAGE_DOWN;
                case 32:
                    return FMCKey::KEY1;
                case 33:
                    return FMCKey::KEY2;
                case 34:
                    return FMCKey::KEY3;
                case 35:
                    return FMCKey::KEY4;
                case 36:
                    return FMCKey::KEY5;
                case 37:
                    return FMCKey::KEY6;
                case 38:
                    return FMCKey::KEY7;
                case 39:
                    return FMCKey::KEY8;
                case 40:
                    return FMCKey::KEY9;
                case 41:
                    return FMCKey::PERIOD;
                case 42:
                    return FMCKey::KEY0;
                case 43:
                    return FMCKey::PLUSMINUS;
                case 44:
                    return FMCKey::KEYA;
                case 45:
                    return FMCKey::KEYB;
                case 46:
                    return FMCKey::KEYC;
                case 47:
                    return FMCKey::KEYD;
                case 48:
                    return FMCKey::KEYE;
                case 49:
                    return FMCKey::KEYF;
                case 50:
                    return FMCKey::KEYG;
                case 51:
                    return FMCKey::KEYH;
                case 52:
                    return FMCKey::KEYI;
                case 53:
                    return FMCKey::KEYJ;
                case 54:
                    return FMCKey::KEYK;
                case 55:
                    return FMCKey::KEYL;
                case 56:
                    return FMCKey::KEYM;
                case 57:
                    return FMCKey::KEYN;
                case 58:
                    return FMCKey::KEYO;
                case 59:
                    return FMCKey::KEYP;
                case 60:
                    return FMCKey::KEYQ;
                case 61:
                    return FMCKey::KEYR;
                case 62:
                    return FMCKey::KEYS;
                case 63:
                    return FMCKey::KEYT;
                case 64:
                    return FMCKey::KEYU;
                case 65:
                    return FMCKey::KEYV;
                case 66:
                    return FMCKey::KEYW;
                case 67:
                    return FMCKey::KEYX;
                case 68:
                    return FMCKey::KEYY;
                case 69:
                    return FMCKey::KEYZ;
                case 70:
                    return FMCKey::SLASH;
                case 71:
                    return FMCKey::SPACE;
                case 72:
                    return FMCKey::MCDU_OVERFLY;
                case 73:
                    return FMCKey::CLR;
            }

            return FMCKey::INVALID_UNKNOWN;
        }

        static FMCKey getPFP3NButtonForIndex(int index) {
            switch (index) {
                case 0:
                    return FMCKey::LSK1L;
                case 1:
                    return FMCKey::LSK2L;
                case 2:
                    return FMCKey::LSK3L;
                case 3:
                    return FMCKey::LSK4L;
                case 4:
                    return FMCKey::LSK5L;
                case 5:
                    return FMCKey::LSK6L;
                case 6:
                    return FMCKey::LSK1R;
                case 7:
                    return FMCKey::LSK2R;
                case 8:
                    return FMCKey::LSK3R;
                case 9:
                    return FMCKey::LSK4R;
                case 10:
                    return FMCKey::LSK5R;
                case 11:
                    return FMCKey::LSK6R;
                case 12:
                    return FMCKey::PFP_INIT_REF;
                case 13:
                    return FMCKey::PFP_ROUTE;
                case 14:
                    return FMCKey::PFP3_CLB;
                case 15:
                    return FMCKey::PFP3_CRZ;
                case 16:
                    return FMCKey::PFP3_DES;
                case 17:
                    return FMCKey::BRIGHTNESS_DOWN;
                case 18:
                    return FMCKey::BRIGHTNESS_UP;
                case 19:
                    return FMCKey::MENU;
                case 20:
                    return FMCKey::PFP_LEGS;
                case 21:
                    return FMCKey::PFP_DEP_ARR;
                case 22:
                    return FMCKey::PFP_HOLD;
                case 23:
                    return FMCKey::PROG;
                case 24:
                    return FMCKey::PFP_EXEC;
                case 25:
                    return FMCKey::PFP3_N1_LIMIT;
                case 26:
                    return FMCKey::PFP_FIX;
                case 27:
                    return FMCKey::PAGE_PREV;
                case 28:
                    return FMCKey::PAGE_NEXT;
                case 29:
                    return FMCKey::KEY1;
                case 30:
                    return FMCKey::KEY2;
                case 31:
                    return FMCKey::KEY3;
                case 32:
                    return FMCKey::KEY4;
                case 33:
                    return FMCKey::KEY5;
                case 34:
                    return FMCKey::KEY6;
                case 35:
                    return FMCKey::KEY7;
                case 36:
                    return FMCKey::KEY8;
                case 37:
                    return FMCKey::KEY9;
                case 38:
                    return FMCKey::PERIOD;
                case 39:
                    return FMCKey::KEY0;
                case 40:
                    return FMCKey::PLUSMINUS;
                case 41:
                    return FMCKey::KEYA;
                case 42:
                    return FMCKey::KEYB;
                case 43:
                    return FMCKey::KEYC;
                case 44:
                    return FMCKey::KEYD;
                case 45:
                    return FMCKey::KEYE;
                case 46:
                    return FMCKey::KEYF;
                case 47:
                    return FMCKey::KEYG;
                case 48:
                    return FMCKey::KEYH;
                case 49:
                    return FMCKey::KEYI;
                case 50:
                    return FMCKey::KEYJ;
                case 51:
                    return FMCKey::KEYK;
                case 52:
                    return FMCKey::KEYL;
                case 53:
                    return FMCKey::KEYM;
                case 54:
                    return FMCKey::KEYN;
                case 55:
                    return FMCKey::KEYO;
                case 56:
                    return FMCKey::KEYP;
                case 57:
                    return FMCKey::KEYQ;
                case 58:
                    return FMCKey::KEYR;
                case 59:
                    return FMCKey::KEYS;
                case 60:
                    return FMCKey::KEYT;
                case 61:
                    return FMCKey::KEYU;
                case 62:
                    return FMCKey::KEYV;
                case 63:
                    return FMCKey::KEYW;
                case 64:
                    return FMCKey::KEYX;
                case 65:
                    return FMCKey::KEYY;
                case 66:
                    return FMCKey::KEYZ;
                case 67:
                    return FMCKey::SPACE;
                case 68:
                    return FMCKey::PFP_DEL;
                case 69:
                    return FMCKey::SLASH;
                case 70:
                    return FMCKey::CLR;
            }

            return FMCKey::INVALID_UNKNOWN;
        }

        static FMCKey getPFP4ButtonForIndex(int index) {
            switch (index) {
                case 0:
                    return FMCKey::LSK1L;
                case 1:
                    return FMCKey::LSK2L;
                case 2:
                    return FMCKey::LSK3L;
                case 3:
                    return FMCKey::LSK4L;
                case 4:
                    return FMCKey::LSK5L;
                case 5:
                    return FMCKey::LSK6L;
                case 6:
                    return FMCKey::LSK1R;
                case 7:
                    return FMCKey::LSK2R;
                case 8:
                    return FMCKey::LSK3R;
                case 9:
                    return FMCKey::LSK4R;
                case 10:
                    return FMCKey::LSK5R;
                case 11:
                    return FMCKey::LSK6R;
                case 12:
                    return FMCKey::PFP_INIT_REF;
                case 13:
                    return FMCKey::PFP_ROUTE;
                case 14:
                    return FMCKey::PFP_DEP_ARR;
                case 15:
                    return FMCKey::PFP4_ATC;
                case 16:
                    return FMCKey::PFP4_VNAV;
                case 17:
                    return FMCKey::BRIGHTNESS_DOWN;
                case 18:
                    return FMCKey::BRIGHTNESS_UP;
                case 19:
                    return FMCKey::PFP_FIX;
                case 20:
                    return FMCKey::PFP_LEGS;
                case 21:
                    return FMCKey::PFP_HOLD;
                case 22:
                    return FMCKey::PFP4_FMC_COMM;
                case 23:
                    return FMCKey::PROG;
                case 24:
                    return FMCKey::PFP_EXEC;
                case 25:
                    return FMCKey::MENU;
                case 26:
                    return FMCKey::PFP4_NAV_RAD;
                case 27:
                    return FMCKey::PAGE_PREV;
                case 28:
                    return FMCKey::PAGE_NEXT;
                case 29:
                    return FMCKey::KEY1;
                case 30:
                    return FMCKey::KEY2;
                case 31:
                    return FMCKey::KEY3;
                case 32:
                    return FMCKey::KEY4;
                case 33:
                    return FMCKey::KEY5;
                case 34:
                    return FMCKey::KEY6;
                case 35:
                    return FMCKey::KEY7;
                case 36:
                    return FMCKey::KEY8;
                case 37:
                    return FMCKey::KEY9;
                case 38:
                    return FMCKey::PERIOD;
                case 39:
                    return FMCKey::KEY0;
                case 40:
                    return FMCKey::PLUSMINUS;
                case 41:
                    return FMCKey::KEYA;
                case 42:
                    return FMCKey::KEYB;
                case 43:
                    return FMCKey::KEYC;
                case 44:
                    return FMCKey::KEYD;
                case 45:
                    return FMCKey::KEYE;
                case 46:
                    return FMCKey::KEYF;
                case 47:
                    return FMCKey::KEYG;
                case 48:
                    return FMCKey::KEYH;
                case 49:
                    return FMCKey::KEYI;
                case 50:
                    return FMCKey::KEYJ;
                case 51:
                    return FMCKey::KEYK;
                case 52:
                    return FMCKey::KEYL;
                case 53:
                    return FMCKey::KEYM;
                case 54:
                    return FMCKey::KEYN;
                case 55:
                    return FMCKey::KEYO;
                case 56:
                    return FMCKey::KEYP;
                case 57:
                    return FMCKey::KEYQ;
                case 58:
                    return FMCKey::KEYR;
                case 59:
                    return FMCKey::KEYS;
                case 60:
                    return FMCKey::KEYT;
                case 61:
                    return FMCKey::KEYU;
                case 62:
                    return FMCKey::KEYV;
                case 63:
                    return FMCKey::KEYW;
                case 64:
                    return FMCKey::KEYX;
                case 65:
                    return FMCKey::KEYY;
                case 66:
                    return FMCKey::KEYZ;
                case 67:
                    return FMCKey::SPACE;
                case 68:
                    return FMCKey::PFP_DEL;
                case 69:
                    return FMCKey::SLASH;
                case 70:
                    return FMCKey::CLR;
            }

            return FMCKey::INVALID_UNKNOWN;
        }

        static FMCKey getPFP7ButtonForIndex(int index) {
            switch (index) {
                case 0:
                    return FMCKey::LSK1L;
                case 1:
                    return FMCKey::LSK2L;
                case 2:
                    return FMCKey::LSK3L;
                case 3:
                    return FMCKey::LSK4L;
                case 4:
                    return FMCKey::LSK5L;
                case 5:
                    return FMCKey::LSK6L;
                case 6:
                    return FMCKey::LSK1R;
                case 7:
                    return FMCKey::LSK2R;
                case 8:
                    return FMCKey::LSK3R;
                case 9:
                    return FMCKey::LSK4R;
                case 10:
                    return FMCKey::LSK5R;
                case 11:
                    return FMCKey::LSK6R;
                case 12:
                    return FMCKey::PFP_INIT_REF;
                case 13:
                    return FMCKey::PFP_ROUTE;
                case 14:
                    return FMCKey::PFP_DEP_ARR;
                case 15:
                    return FMCKey::PFP7_ALTN;
                case 16:
                    return FMCKey::PFP7_VNAV;
                case 17:
                    return FMCKey::BRIGHTNESS_DOWN;
                case 18:
                    return FMCKey::BRIGHTNESS_UP;
                case 19:
                    return FMCKey::PFP_FIX;
                case 20:
                    return FMCKey::PFP_LEGS;
                case 21:
                    return FMCKey::PFP_HOLD;
                case 22:
                    return FMCKey::PFP7_FMC_COMM;
                case 23:
                    return FMCKey::PROG;
                case 24:
                    return FMCKey::PFP_EXEC;
                case 25:
                    return FMCKey::MENU;
                case 26:
                    return FMCKey::PFP7_NAV_RAD;
                case 27:
                    return FMCKey::PAGE_PREV;
                case 28:
                    return FMCKey::PAGE_NEXT;
                case 29:
                    return FMCKey::KEY1;
                case 30:
                    return FMCKey::KEY2;
                case 31:
                    return FMCKey::KEY3;
                case 32:
                    return FMCKey::KEY4;
                case 33:
                    return FMCKey::KEY5;
                case 34:
                    return FMCKey::KEY6;
                case 35:
                    return FMCKey::KEY7;
                case 36:
                    return FMCKey::KEY8;
                case 37:
                    return FMCKey::KEY9;
                case 38:
                    return FMCKey::PERIOD;
                case 39:
                    return FMCKey::KEY0;
                case 40:
                    return FMCKey::PLUSMINUS;
                case 41:
                    return FMCKey::KEYA;
                case 42:
                    return FMCKey::KEYB;
                case 43:
                    return FMCKey::KEYC;
                case 44:
                    return FMCKey::KEYD;
                case 45:
                    return FMCKey::KEYE;
                case 46:
                    return FMCKey::KEYF;
                case 47:
                    return FMCKey::KEYG;
                case 48:
                    return FMCKey::KEYH;
                case 49:
                    return FMCKey::KEYI;
                case 50:
                    return FMCKey::KEYJ;
                case 51:
                    return FMCKey::KEYK;
                case 52:
                    return FMCKey::KEYL;
                case 53:
                    return FMCKey::KEYM;
                case 54:
                    return FMCKey::KEYN;
                case 55:
                    return FMCKey::KEYO;
                case 56:
                    return FMCKey::KEYP;
                case 57:
                    return FMCKey::KEYQ;
                case 58:
                    return FMCKey::KEYR;
                case 59:
                    return FMCKey::KEYS;
                case 60:
                    return FMCKey::KEYT;
                case 61:
                    return FMCKey::KEYU;
                case 62:
                    return FMCKey::KEYV;
                case 63:
                    return FMCKey::KEYW;
                case 64:
                    return FMCKey::KEYX;
                case 65:
                    return FMCKey::KEYY;
                case 66:
                    return FMCKey::KEYZ;
                case 67:
                    return FMCKey::SPACE;
                case 68:
                    return FMCKey::PFP_DEL;
                case 69:
                    return FMCKey::SLASH;
                case 70:
                    return FMCKey::CLR;
            }

            return FMCKey::INVALID_UNKNOWN;
        }
};

#endif
