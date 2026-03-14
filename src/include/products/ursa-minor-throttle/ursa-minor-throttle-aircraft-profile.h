#ifndef URSA_MINOR_THROTTLE_AIRCRAFT_PROFILE_H
#define URSA_MINOR_THROTTLE_AIRCRAFT_PROFILE_H

#include <string>
#include <unordered_map>
#include <XPLMUtilities.h>

class ProductUrsaMinorThrottle;

enum class UrsaMinorThrottleDatarefType : unsigned char {
    EXECUTE_CMD_ONCE = 1,
    EXECUTE_CMD_ONCE_IF_CHANGED,
    EXECUTE_MULTIPLE_CMD_ONCE,
    EXECUTE_CMD_PHASED,
    SET_VALUE,
    SET_VALUE_USING_COMMANDS,
    TOLISS_SPEEDBRAKE
};

struct UrsaMinorThrottleButtonDef {
        std::string name;
        std::string dataref;
        UrsaMinorThrottleDatarefType datarefType = UrsaMinorThrottleDatarefType::EXECUTE_CMD_ONCE;
        double value = 0.0;
};

class UrsaMinorThrottleAircraftProfile {
    protected:
        ProductUrsaMinorThrottle *product;
        std::unordered_map<std::string, int> datarefChangedCache = {};

    public:
        UrsaMinorThrottleAircraftProfile(ProductUrsaMinorThrottle *product) : product(product) {};
        virtual ~UrsaMinorThrottleAircraftProfile() = default;

        virtual void update() {}

        virtual const std::unordered_map<uint16_t, UrsaMinorThrottleButtonDef> &buttonDefs() const = 0;
        virtual void buttonPressed(const UrsaMinorThrottleButtonDef *button, XPLMCommandPhase phase) = 0;

        virtual void updateDisplays() = 0;
};

#endif
