#ifndef ECAM_AIRCRAFT_PROFILE_H
#define ECAM_AIRCRAFT_PROFILE_H

#include "profile-cleanup.h"

#include <string>
#include <unordered_map>
#include <XPLMUtilities.h>

class ProductECAM;

struct ECAMButtonDef {
        std::string name;
        std::string dataref;
        double value = 0.0;
};

class ECAMAircraftProfile {
    protected:
        ProductECAM *product;

    public:
        ECAMAircraftProfile(ProductECAM *product) : product(product) {};
        virtual ~ECAMAircraftProfile() {
            cleanupProfile(this);
        }

        virtual const std::unordered_map<uint16_t, ECAMButtonDef> &buttonDefs() const = 0;
        virtual void buttonPressed(const ECAMButtonDef *button, XPLMCommandPhase phase) = 0;
};

#endif
