#include "fps748-tcas-profile.h"

#include "dataref.h"
#include "product-tcas.h"

FPS748TCASProfile::FPS748TCASProfile(ProductTCAS *product) : TCASAircraftProfile(product) {
    bool isSSG = IsSSGVersion();
    std::string altPrefix = isSSG ? "ssg" : "FPS";

    Dataref::getInstance()->monitorExistingDataref<float>((altPrefix + "/LGT/glaresheld_sw").c_str(), [product, altPrefix](float brightness) {
        bool hasPower = Dataref::getInstance()->getCached<bool>((altPrefix + "/Elec/bus_1_powered").c_str());
        uint8_t backlight = hasPower ? static_cast<uint8_t>(brightness * 255) : 0;
        product->setLedBrightness(TCASLed::BACKLIGHT, backlight);
        product->setLedBrightness(TCASLed::LCD_BRIGHTNESS, hasPower ? 255 : 0);
        product->setLedBrightness(TCASLed::OVERALL_LEDS_BRIGHTNESS, hasPower ? 255 : 0);
    }, this);

    Dataref::getInstance()->monitorExistingDataref<bool>((altPrefix + "/Elec/bus_1_powered").c_str(), [altPrefix](bool) {
        Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/LGT/glaresheld_sw").c_str());
    }, this);

    Dataref::getInstance()->executeChangedCallbacksForDataref((altPrefix + "/Elec/bus_1_powered").c_str());
}

bool FPS748TCASProfile::IsSSGVersion() {
    return Dataref::getInstance()->exists("SSG/748/simtime");
}

bool FPS748TCASProfile::IsEligible() {
    return Dataref::getInstance()->exists("FPS/748/simtime") || Dataref::getInstance()->exists("SSG/748/simtime");
}

const std::vector<std::string> &FPS748TCASProfile::displayDatarefs() const {
    static const std::vector<std::string> datarefs = {
        "FPS/Radio/transp_da",
        "FPS/Radio/transp_C1",
        "FPS/Radio/transp_C2",
        "FPS/Radio/transp_C3",
        "FPS/Radio/transp_C4",
    };

    return datarefs;
}

const std::unordered_map<uint16_t, TCASButtonDef> &FPS748TCASProfile::buttonDefs() const {
    static const std::unordered_map<uint16_t, TCASButtonDef> buttons = {
        // Keypad digits (transponder codes use octal digits 0-7)
        {0, {"Keypad 1", "FPS/Radio/transp_Key_1", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {1, {"Keypad 2", "FPS/Radio/transp_Key_2", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {2, {"Keypad 3", "FPS/Radio/transp_Key_3", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {3, {"Keypad 4", "FPS/Radio/transp_Key_4", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {4, {"Keypad 5", "FPS/Radio/transp_Key_5", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {5, {"Keypad 6", "FPS/Radio/transp_Key_6", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {6, {"Keypad 7", "FPS/Radio/transp_Key_7", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {7, {"Keypad 0", "FPS/Radio/transp_Key_0", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {8, {"Keypad CLR", "FPS/Radio/transp_Key_clr", TCASDatarefType::SET_VALUE_PHASED, 1}},
        {9, {"Ident", "FPS/Radio/transp_id_sw", TCASDatarefType::SET_VALUE_PHASED, 1}},

        // Transponder mode selector
        {10, {"XPDR STBY", "FPS/Radio/transp_mode_sw", TCASDatarefType::SET_VALUE, 0}},
        {12, {"XPDR ON", "FPS/Radio/transp_mode_sw", TCASDatarefType::SET_VALUE, 1}},
        {15, {"ALT RPTG OFF", "FPS/Radio/transp_mode_sw", TCASDatarefType::SET_VALUE, 5}},
        {22, {"TCAS TA", "FPS/Radio/transp_mode_sw", TCASDatarefType::SET_VALUE, 2}},
        {23, {"TCAS TA/RA", "FPS/Radio/transp_mode_sw", TCASDatarefType::SET_VALUE, 3}},
    };
    return buttons;
}

void FPS748TCASProfile::buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto dm = Dataref::getInstance();

    if (button->datarefType == TCASDatarefType::SET_VALUE_PHASED) {
        dm->set<int>(button->dataref.c_str(), phase == xplm_CommandBegin ? static_cast<int>(button->value) : 0);
    } else if (button->datarefType == TCASDatarefType::SET_VALUE) {
        if (phase == xplm_CommandBegin) {
            dm->set<int>(button->dataref.c_str(), static_cast<int>(button->value));
        }
    } else {
        dm->executeCommand(button->dataref.c_str(), phase);
    }
}

void FPS748TCASProfile::updateDisplays() {
    if (!product) {
        return;
    }

    auto dm = Dataref::getInstance();
    int da = dm->getCached<int>("FPS/Radio/transp_da");

    if (da == 0) {
        product->setLCDText("0");
        return;
    }

    std::string code;
    code.reserve(4);

    int digits[4] = {
        static_cast<int>(dm->getCached<float>("FPS/Radio/transp_C1")) / 1000,
        static_cast<int>(dm->getCached<float>("FPS/Radio/transp_C2")) / 100,
        static_cast<int>(dm->getCached<float>("FPS/Radio/transp_C3")) / 10,
        static_cast<int>(dm->getCached<float>("FPS/Radio/transp_C4")),
    };

    int count = da < 4 ? da : 4;
    for (int i = 0; i < count; i++) {
        code += static_cast<char>('0' + digits[i]);
    }

    product->setLCDText(code);
}
