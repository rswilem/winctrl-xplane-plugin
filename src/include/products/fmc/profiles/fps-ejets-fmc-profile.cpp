#include "fps-ejets-fmc-profile.h"

#include "appstate.h"
#include "dataref.h"
#include "product-fmc.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>

FPSEjetsFMCProfile::FPSEjetsFMCProfile(ProductFMC *product) : FMCAircraftProfile(product) {
    const std::string prefix = UfmcPrefix();
    datarefRegex = std::regex(prefix + "/UFMC/LINE_([0-9]+)");
    for (int line = 1; line <= 14; line++) {
        lineDatarefs.push_back(prefix + "/UFMC/LINE_" + std::to_string(line));
    }

    const std::string cdu = product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "SSG/CDU/cdu1" : "SSG/CDU/cdu2";
    const std::vector<std::pair<std::variant<FMCKey, std::vector<FMCKey>>, std::string>> keys = {
        {FMCKey::LSK1L, "lk1"},
        {FMCKey::LSK2L, "lk2"},
        {FMCKey::LSK3L, "lk3"},
        {FMCKey::LSK4L, "lk4"},
        {FMCKey::LSK5L, "lk5"},
        {FMCKey::LSK6L, "lk6"},
        {FMCKey::LSK1R, "rk1"},
        {FMCKey::LSK2R, "rk2"},
        {FMCKey::LSK3R, "rk3"},
        {FMCKey::LSK4R, "rk4"},
        {FMCKey::LSK5R, "rk5"},
        {FMCKey::LSK6R, "rk6"},
        {std::vector<FMCKey>{FMCKey::PFP_INIT_REF, FMCKey::MCDU_SEC_FPLN}, "nav"},
        {std::vector<FMCKey>{FMCKey::PFP_ROUTE, FMCKey::MCDU_INIT}, "rte"},
        {std::vector<FMCKey>{FMCKey::PFP_LEGS, FMCKey::MCDU_FPLN}, "fpl"},
        {std::vector<FMCKey>{FMCKey::MCDU_PERF, FMCKey::PFP3_CLB, FMCKey::PFP3_CRZ, FMCKey::PFP3_DES}, "perf"},
        {std::vector<FMCKey>{FMCKey::MCDU_FUEL_PRED, FMCKey::PFP3_N1_LIMIT}, "trs"},
        {std::vector<FMCKey>{FMCKey::MCDU_DATA, FMCKey::MCDU_ATC_COMM, FMCKey::PFP4_FMC_COMM, FMCKey::PFP7_FMC_COMM}, "dlk"},
        {std::vector<FMCKey>{FMCKey::MCDU_RAD_NAV, FMCKey::PFP4_NAV_RAD, FMCKey::PFP7_NAV_RAD}, "radio"},
        {FMCKey::PROG, "prog"},
        {FMCKey::MENU, "menu"},
        {FMCKey::PAGE_PREV, "prev"},
        {FMCKey::PAGE_NEXT, "next"},
        {FMCKey::PERIOD, "dot"},
        {FMCKey::PLUSMINUS, "dash"},
        {FMCKey::SPACE, "sp"},
        {FMCKey::SLASH, "slash"},
        {std::vector<FMCKey>{FMCKey::PFP_DEL, FMCKey::MCDU_OVERFLY}, "del"},
        {FMCKey::CLR, "clr"},
    };
    for (const auto &[key, name] : keys) {
        buttons.push_back({key, cdu + "_" + name + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }
    buttons.push_back({FMCKey::BRIGHTNESS_DOWN, cdu + "_brt_sw", FMCDatarefType::SET_VALUE_PHASED, -1});
    buttons.push_back({FMCKey::BRIGHTNESS_UP, cdu + "_brt_sw", FMCDatarefType::SET_VALUE_PHASED, 1});

    const FMCKey digitKeys[] = {FMCKey::KEY0, FMCKey::KEY1, FMCKey::KEY2, FMCKey::KEY3, FMCKey::KEY4, FMCKey::KEY5, FMCKey::KEY6, FMCKey::KEY7, FMCKey::KEY8, FMCKey::KEY9};
    for (int i = 0; i < 10; i++) {
        buttons.push_back({digitKeys[i], cdu + "_" + std::to_string(i) + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }

    const FMCKey letterKeys[] = {FMCKey::KEYA, FMCKey::KEYB, FMCKey::KEYC, FMCKey::KEYD, FMCKey::KEYE, FMCKey::KEYF, FMCKey::KEYG, FMCKey::KEYH, FMCKey::KEYI, FMCKey::KEYJ, FMCKey::KEYK, FMCKey::KEYL, FMCKey::KEYM, FMCKey::KEYN, FMCKey::KEYO, FMCKey::KEYP, FMCKey::KEYQ, FMCKey::KEYR, FMCKey::KEYS, FMCKey::KEYT, FMCKey::KEYU, FMCKey::KEYV, FMCKey::KEYW, FMCKey::KEYX, FMCKey::KEYY, FMCKey::KEYZ};
    for (int i = 0; i < 26; i++) {
        buttons.push_back({letterKeys[i], cdu + "_" + std::string(1, static_cast<char>('a' + i)) + "_sw", FMCDatarefType::SET_VALUE_PHASED});
    }

    for (const auto &button : buttons) {
        std::visit([&](auto &&k) {
            using T = std::decay_t<decltype(k)>;
            if constexpr (std::is_same_v<T, FMCKey>) {
                keyMap[k] = &button;
            } else {
                for (const auto &key : k) {
                    keyMap[key] = &button;
                }
            }
        },
            button.key);
    }

    product->setAllLedsEnabled(false);
    product->setFont(FontVariant::Font737);

    Dataref::getInstance()->monitorExistingDataref<float>(BrightnessDataref().c_str(), [this](float) {
        updateBacklight();
    },
        this);

    Dataref::getInstance()->monitorExistingDataref<float>("ssg/Elec/ac_alive", [this](float) {
        updateBacklight();
    },
        this);

    updateBacklight();
}

FPSEjetsFMCProfile::~FPSEjetsFMCProfile() {
    if (radioLinesVisible) {
        drawRadioLines(false);
    }
}

std::string FPSEjetsFMCProfile::UfmcPrefix() {
    return Dataref::getInstance()->exists("SSG/UFMC/LINE_1") ? "SSG" : "FJCC";
}

std::string FPSEjetsFMCProfile::BrightnessDataref() const {
    return product->deviceVariant == FMCDeviceVariant::VARIANT_CAPTAIN ? "SSG/EJET/LIGHTS/mcdu_p_brt_rt" : "SSG/EJET/LIGHTS/mcdu_cp_brt_rt";
}

void FPSEjetsFMCProfile::updateBacklight() {
    auto datarefManager = Dataref::getInstance();
    const std::string brightnessRef = BrightnessDataref();
    bool powered = !datarefManager->exists("ssg/Elec/ac_alive") || datarefManager->get<float>("ssg/Elec/ac_alive") > 0.5f;
    float brightness = datarefManager->exists(brightnessRef.c_str()) ? datarefManager->get<float>(brightnessRef.c_str()) : 1.0f;
    uint8_t target = powered ? static_cast<uint8_t>(std::clamp(brightness, 0.0f, 1.0f) * 255) : 0;

    product->setLedBrightness(FMCLed::BACKLIGHT, target);
    product->setLedBrightness(FMCLed::SCREEN_BACKLIGHT, target);
}

bool FPSEjetsFMCProfile::IsEligible() {
    std::string icao = Dataref::getInstance()->get<std::string>("sim/aircraft/view/acf_ICAO");
    return (icao == "E170" || icao == "E175" || icao == "E190" || icao == "E195") && Dataref::getInstance()->exists("SSG/CDU/cdu1_menu_sw");
}

const std::vector<std::string> &FPSEjetsFMCProfile::displayDatarefs() const {
    return lineDatarefs;
}

const std::vector<FMCButtonDef> &FPSEjetsFMCProfile::buttonDefs() const {
    return buttons;
}

const std::unordered_map<FMCKey, const FMCButtonDef *> &FPSEjetsFMCProfile::buttonKeyMap() const {
    return keyMap;
}

const std::map<char, FMCTextColor> &FPSEjetsFMCProfile::colorMap() const {
    static const std::map<char, FMCTextColor> colMap = {
        {'c', FMCTextColor::COLOR_CYAN},
        {'g', FMCTextColor::COLOR_GREEN},
        {'p', FMCTextColor::COLOR_MAGENTA},
        {'w', FMCTextColor::COLOR_WHITE},
        {'y', FMCTextColor::COLOR_YELLOW},
        {'l', FMCTextColor::COLOR_RED},
        {'s', FMCTextColor::COLOR_GREY},
        {'x', FMCTextColor::COLOR_RED},
        {'i', FMCTextColor::withBackgroundColor(FMCTextColor::COLOR_WHITE, FMCTextColor::COLOR_GREY)},
    };

    return colMap;
}

void FPSEjetsFMCProfile::mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) {
    switch (character) {
        case '#':
            buffer->insert(buffer->end(), FMCSpecialCharacter::OUTLINED_SQUARE.begin(), FMCSpecialCharacter::OUTLINED_SQUARE.end());
            break;

        case '=':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DEGREES.begin(), FMCSpecialCharacter::DEGREES.end());
            break;

        case ',':
            buffer->insert(buffer->end(), FMCSpecialCharacter::DIAMOND.begin(), FMCSpecialCharacter::DIAMOND.end());
            break;

        case '{':
            buffer->push_back('[');
            break;

        case '}':
            buffer->push_back(']');
            break;

        case '<':
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_LEFT.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_LEFT.end());
            break;

        case '>':
            buffer->insert(buffer->end(), FMCSpecialCharacter::FILLED_TRIANGLE_RIGHT.begin(), FMCSpecialCharacter::FILLED_TRIANGLE_RIGHT.end());
            break;

        default:
            buffer->push_back(character);
            break;
    }
}

namespace {
    struct FPSEjetsCell {
            char character = ' ';
            std::string codes;
    };

    std::vector<FPSEjetsCell> parseCells(const std::string &text) {
        std::vector<FPSEjetsCell> cells;
        std::string pending;
        for (size_t i = 0; i < text.size(); ++i) {
            char c = text[i];
            if (c == 0x00) {
                break;
            }
            if (c == ';' && i + 1 < text.size() && text[i + 1] == ';') {
                continue;
            }
            if (c == ';' && i + 1 < text.size()) {
                pending += text[++i];
                continue;
            }
            if (c == '[' && i + 1 < text.size() && text[i + 1] == ']') {
                cells.push_back({'#', pending});
                pending.clear();
                ++i;
                continue;
            }
            cells.push_back({c, pending});
            pending.clear();
        }
        return cells;
    }

    void eraseCell(std::vector<FPSEjetsCell> &cells, size_t index) {
        if (index + 1 < cells.size()) {
            cells[index + 1].codes = cells[index].codes + cells[index + 1].codes;
        }
        cells.erase(cells.begin() + index);
    }

    size_t leadingSpaces(const std::vector<FPSEjetsCell> &cells) {
        size_t count = 0;
        while (count < cells.size() && cells[count].character == ' ') {
            count++;
        }
        return count;
    }

    struct FPSEjetsGap {
            size_t start = 0;
            size_t length = 0;
    };

    std::vector<FPSEjetsGap> interiorGaps(const std::vector<FPSEjetsCell> &cells) {
        std::vector<FPSEjetsGap> gaps;
        size_t i = leadingSpaces(cells);
        while (i < cells.size()) {
            if (cells[i].character != ' ') {
                i++;
                continue;
            }
            size_t start = i;
            while (i < cells.size() && cells[i].character == ' ') {
                i++;
            }
            if (i < cells.size()) {
                gaps.push_back({start, i - start});
            }
        }
        return gaps;
    }
}

void FPSEjetsFMCProfile::updatePage(std::vector<std::vector<char>> &page) {
    page = std::vector<std::vector<char>>(ProductFMC::PageLines, std::vector<char>(ProductFMC::PageCharsPerLine * ProductFMC::PageBytesPerChar, ' '));

    const size_t width = ProductFMC::PageCharsPerLine;
    bool onRadioPage = false;
    auto datarefManager = Dataref::getInstance();
    for (const auto &ref : displayDatarefs()) {
        std::smatch match;
        if (!std::regex_match(ref, match, datarefRegex)) {
            continue;
        }

        int lineIndex = std::stoi(match[1]) - 1;
        if (lineIndex < 0 || lineIndex >= ProductFMC::PageLines) {
            continue;
        }

        std::vector<FPSEjetsCell> cells = parseCells(datarefManager->getCached<std::string>(ref.c_str()));
        while (!cells.empty() && cells.back().character == ' ') {
            cells.pop_back();
        }
        if (cells.empty()) {
            continue;
        }

        size_t leading = leadingSpaces(cells);
        if (leading > 0 && leading < cells.size() && cells[leading].character == '<') {
            for (size_t i = 0; i < leading; ++i) {
                eraseCell(cells, 0);
            }
        }

        while (cells.size() > width) {
            auto gaps = interiorGaps(cells);
            auto widest = std::max_element(gaps.begin(), gaps.end(), [](const FPSEjetsGap &a, const FPSEjetsGap &b) {
                return a.length < b.length;
            });
            if (widest != gaps.end() && widest->length >= 2) {
                eraseCell(cells, widest->start);
                continue;
            }
            if (leadingSpaces(cells) > 0) {
                eraseCell(cells, 0);
                continue;
            }
            if (!gaps.empty()) {
                size_t middle = cells.size() / 2;
                auto nearest = std::min_element(gaps.begin(), gaps.end(), [middle](const FPSEjetsGap &a, const FPSEjetsGap &b) {
                    return std::abs(static_cast<long>(a.start) - static_cast<long>(middle)) < std::abs(static_cast<long>(b.start) - static_cast<long>(middle));
                });
                eraseCell(cells, nearest->start);
                continue;
            }
            cells.resize(width);
        }

        for (size_t i = 1; i < cells.size() && cells.size() < width; ++i) {
            bool colorChange = std::any_of(cells[i].codes.begin(), cells[i].codes.end(), [](char code) {
                return std::isalpha(static_cast<unsigned char>(code));
            });
            if (colorChange && cells[i].character != ' ' && cells[i - 1].character != ' ') {
                cells.insert(cells.begin() + i, FPSEjetsCell{});
                i++;
            }
        }

        size_t splitIndex = cells.size();
        size_t insertAt = cells.size();
        auto gaps = interiorGaps(cells);
        auto lastWideGap = std::find_if(gaps.rbegin(), gaps.rend(), [](const FPSEjetsGap &gap) {
            return gap.length >= 2;
        });
        size_t lastFieldMarker = 0;
        size_t lead = leadingSpaces(cells);
        if (lineIndex > 0 && lead < 3 && lead + 2 < cells.size() && cells[lead].character != ' ' && cells[lead + 1].character == ' ' && cells[lead + 2].character == ' ') {
            while (lead + 2 < cells.size() && cells[lead + 2].character == ' ') {
                eraseCell(cells, lead + 1);
            }
            for (size_t i = cells.size() - 1; i > lead + 2; --i) {
                if (cells[i].character != ' ' && cells[i - 1].character == ' ' && !cells[i].codes.empty()) {
                    lastFieldMarker = i;
                    break;
                }
            }
            gaps = interiorGaps(cells);
            lastWideGap = std::find_if(gaps.rbegin(), gaps.rend(), [](const FPSEjetsGap &gap) {
                return gap.length >= 2;
            });
        }

        size_t pageNumberStart = cells.size();
        if (lineIndex == 0) {
            while (pageNumberStart > 0 && (std::isdigit(static_cast<unsigned char>(cells[pageNumberStart - 1].character)) || cells[pageNumberStart - 1].character == '/')) {
                pageNumberStart--;
            }
            bool hasSlash = std::any_of(cells.begin() + pageNumberStart, cells.end(), [](const FPSEjetsCell &cell) {
                return cell.character == '/';
            });
            if (!hasSlash || pageNumberStart == 0 || cells[pageNumberStart].character == '/') {
                pageNumberStart = cells.size();
            }
        }

        if (pageNumberStart < cells.size()) {
            size_t titleStart = leadingSpaces(cells);
            size_t titleEnd = pageNumberStart;
            while (titleEnd > titleStart && cells[titleEnd - 1].character == ' ') {
                titleEnd--;
            }
            size_t titleLength = titleEnd - titleStart;
            size_t pageNumberLength = cells.size() - pageNumberStart;
            size_t centered = (width - titleLength + 1) / 2;
            if (titleLength > 0 && centered + titleLength + 1 + pageNumberLength <= width) {
                std::vector<FPSEjetsCell> title(cells.begin() + titleStart, cells.begin() + titleEnd);
                std::vector<FPSEjetsCell> pageNumber(cells.begin() + pageNumberStart, cells.end());
                std::string leadingCodes;
                for (size_t i = 0; i < titleStart; ++i) {
                    leadingCodes += cells[i].codes;
                }
                title.front().codes = leadingCodes + title.front().codes;
                cells.assign(centered, FPSEjetsCell{});
                cells.insert(cells.end(), title.begin(), title.end());
                cells.push_back(FPSEjetsCell{});
                pageNumberStart = cells.size();
                cells.insert(cells.end(), pageNumber.begin(), pageNumber.end());
            }
            insertAt = pageNumberStart;
            splitIndex = pageNumberStart;
        } else if (lastFieldMarker > 0 && lastWideGap == gaps.rend()) {
            insertAt = lastFieldMarker;
            splitIndex = lastFieldMarker;
        } else if (lastWideGap != gaps.rend()) {
            insertAt = lastWideGap->start;
            splitIndex = lastWideGap->start + lastWideGap->length;
        } else if (lineIndex > 0 && leadingSpaces(cells) >= 3) {
            insertAt = 0;
            splitIndex = 0;
        } else if (lineIndex > 0 && cells.size() >= 3 && cells.back().character == '>') {
            size_t start = cells.size() - 1;
            if (cells[start - 1].character == ' ') {
                start--;
            }
            while (start > 0 && cells[start - 1].character != ' ') {
                start--;
            }
            size_t letters = std::count_if(cells.begin() + start, cells.end(), [](const FPSEjetsCell &cell) {
                return std::isalpha(static_cast<unsigned char>(cell.character));
            });
            if (cells[leadingSpaces(cells)].character == '<') {
                for (size_t i = cells.size() - 1; i > leadingSpaces(cells); --i) {
                    if (cells[i].character != ' ' && cells[i - 1].character == ' ' && !cells[i].codes.empty()) {
                        insertAt = i;
                        splitIndex = i;
                        break;
                    }
                }
            } else if (letters >= 2 && start > leadingSpaces(cells)) {
                splitIndex = start;
                insertAt = start;
            }
        }

        if (insertAt < cells.size() && cells.size() < width) {
            size_t padding = width - cells.size();
            cells.insert(cells.begin() + insertAt, padding, FPSEjetsCell{});
            splitIndex += padding;
        }

        const bool lineFontSmall = lineIndex % 2 == 1;
        char currentColor = 'W';
        bool fontSmall = lineFontSmall;
        std::string visibleText;
        for (size_t i = 0; i < cells.size() && i < width; ++i) {
            if (i == splitIndex) {
                currentColor = 'W';
                fontSmall = lineFontSmall;
            }
            for (char code : cells[i].codes) {
                if (code == '2') {
                    fontSmall = true;
                } else if (code == '4') {
                    fontSmall = false;
                } else if (!std::isdigit(static_cast<unsigned char>(code))) {
                    currentColor = code;
                    fontSmall = lineFontSmall;
                }
            }

            char c = static_cast<char>(toupper(static_cast<unsigned char>(cells[i].character)));
            visibleText += c;
            if (c != ' ') {
                product->writeLineToPage(page, lineIndex, static_cast<int>(i), std::string(1, c), currentColor, fontSmall);
            }
        }

        if (lineIndex == 0 && visibleText.find("RADIO") != std::string::npos) {
            onRadioPage = true;
        }
    }

    if (onRadioPage || radioLinesVisible) {
        drawRadioLines(onRadioPage);
    }
}

void FPSEjetsFMCProfile::drawRadioLines(bool visible) {
    FMCScreenLayout layout = FMCHardwareMapping::ScreenLayoutForHardware(product->hardwareType);
    const uint16_t left = 36 + layout.x;
    const uint16_t top = 20 + layout.y;
    const uint16_t width = ProductFMC::PageCharsPerLine * layout.characterWidth;
    const uint16_t comBottom = top + 5 * layout.characterHeight;
    const uint16_t navBottom = top + 9 * layout.characterHeight;
    const uint16_t thickness = 4;

    radioLinesVisible = visible;
    if (!visible) {
        product->fillRect(0, 0, 1, 1, 0x00, 0x00, 0x00);
        return;
    }

    product->fillRect(left, comBottom - 1, width, thickness, 0xff, 0xff, 0xff);
    product->fillRect(left, navBottom - 1, width, thickness, 0xff, 0xff, 0xff);
    product->fillRect(left + width / 2 + layout.characterWidth / 2 - thickness / 2, top + layout.characterHeight, thickness, navBottom - top - layout.characterHeight, 0xff, 0xff, 0xff);
}

void FPSEjetsFMCProfile::buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) {
    if (!button || button->dataref.empty() || phase == xplm_CommandContinue) {
        return;
    }

    auto datarefManager = Dataref::getInstance();
    if (button->datarefType == FMCDatarefType::SET_VALUE || button->datarefType == FMCDatarefType::SET_VALUE_PHASED) {
        double value = std::fabs(button->value) < std::numeric_limits<double>::epsilon() ? 1.0 : button->value;
        if (button->datarefType == FMCDatarefType::SET_VALUE && phase != xplm_CommandBegin) {
            return;
        }

        double phasedValue = phase == xplm_CommandBegin ? value : 0.0;
        datarefManager->set<double>(button->dataref.c_str(), phasedValue);

        if (button->dataref.ends_with("_brt_sw")) {
            return;
        }

        std::string sibling = button->dataref;
        size_t pos = sibling.find("cdu1");
        if (pos != std::string::npos) {
            sibling.replace(pos, 4, "cdu2");
        } else if ((pos = sibling.find("cdu2")) != std::string::npos) {
            sibling.replace(pos, 4, "cdu1");
        } else {
            return;
        }
        datarefManager->set<double>(sibling.c_str(), phasedValue);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::ADJUST_VALUE) {
        double currentValue = datarefManager->get<double>(button->dataref.c_str());
        datarefManager->set<double>(button->dataref.c_str(), currentValue + button->value);
    } else if (phase == xplm_CommandBegin && button->datarefType == FMCDatarefType::EXECUTE_CMD_ONCE) {
        datarefManager->executeCommand(button->dataref.c_str());
    } else {
        datarefManager->executeCommand(button->dataref.c_str(), phase);
    }
}
