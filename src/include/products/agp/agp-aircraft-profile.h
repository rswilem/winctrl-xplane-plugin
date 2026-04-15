#ifndef AGP_AIRCRAFT_PROFILE_H
#define AGP_AIRCRAFT_PROFILE_H

#include <string>
#include <unordered_map>
#include <XPLMUtilities.h>

class ProductAGP;

enum class AGPDatarefType : unsigned char {
    EXECUTE_CMD_PHASED = 1,
    SET_VALUE,
    TERRAIN_ON_ND,
    LANDING_GEAR
};

struct AGPButtonDef {
        std::string name;
        std::string dataref;
        AGPDatarefType datarefType = AGPDatarefType::EXECUTE_CMD_PHASED;
        double value = 0.0;
};

class AGPAircraftProfile {
    protected:
        ProductAGP *product;

    public:
        AGPAircraftProfile(ProductAGP *product) : product(product) {};
        virtual ~AGPAircraftProfile() = default;

        virtual const std::unordered_map<uint16_t, AGPButtonDef> &buttonDefs() const = 0;
        virtual void buttonPressed(const AGPButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual void updateDisplays() = 0;
};

#endif
