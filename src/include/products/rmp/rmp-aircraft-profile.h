#ifndef RMP_AIRCRAFT_PROFILE_H
#define RMP_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <XPLMUtilities.h>

class ProductRMP;

enum class RMPDatarefType : unsigned char {
    EXECUTE_CMD_PHASED = 1,
    SET_VALUE,
};

struct RMPButtonDef {
        std::string name;
        std::string dataref;
        RMPDatarefType datarefType = RMPDatarefType::EXECUTE_CMD_PHASED;
        double value = 0.0;
};

class RMPAircraftProfile {
    protected:
        ProductRMP *product;

    public:
        RMPAircraftProfile(ProductRMP *product) : product(product) {};
        virtual ~RMPAircraftProfile() {
            cleanupProfile(this);
        }

        virtual const std::vector<std::string> &displayDatarefs() const = 0;
        virtual const std::unordered_map<uint16_t, RMPButtonDef> &buttonDefs() const = 0;
        virtual void buttonPressed(const RMPButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual void updateDisplays() = 0;
};

#endif
