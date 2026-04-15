#ifndef TCAS_AIRCRAFT_PROFILE_H
#define TCAS_AIRCRAFT_PROFILE_H

#include <string>
#include <unordered_map>
#include <XPLMUtilities.h>

class ProductTCAS;

enum class TCASDatarefType : unsigned char {
    EXECUTE_CMD_PHASED = 1,
    SET_VALUE
};

struct TCASButtonDef {
        std::string name;
        std::string dataref;
        TCASDatarefType datarefType = TCASDatarefType::EXECUTE_CMD_PHASED;
        double value = 0.0;
};

class TCASAircraftProfile {
    protected:
        ProductTCAS *product;

    public:
        TCASAircraftProfile(ProductTCAS *product) : product(product) {};
        virtual ~TCASAircraftProfile() = default;

        virtual const std::unordered_map<uint16_t, TCASButtonDef> &buttonDefs() const = 0;
        virtual void buttonPressed(const TCASButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual void updateDisplays() = 0;
};

#endif
