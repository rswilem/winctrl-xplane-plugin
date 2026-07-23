#ifndef UNS1_FMC_PROFILE_H
#define UNS1_FMC_PROFILE_H

#include "fmc-aircraft-profile.h"

#include <optional>

// Shared base for aircraft using a Universal Avionics UNS-1 style FMS (5 LSKs
// per side, 11-line CDU). The button table, screen decoding and button dispatch
// are identical across these airframes; only the dataref namespace differs, so
// leaf profiles supply it via displayPathPrefix()/commandPathPrefix() and handle
// their own eligibility check and backlight wiring.
class UNS1FMCProfile : public FMCAircraftProfile {
    protected:
        // Namespace prefix for the CDU display datarefs; this class appends
        // suffixes like "/text_line_0". Defaults to the stock UNS-1 module's
        // "uns1/cdu1"; leaves that re-namespace (e.g. Q4XP -> "FJS/Q4XP/cdu1")
        // override it.
        virtual std::string displayPathPrefix() const;

        // Namespace prefix for the FMS command datarefs; this class appends
        // suffixes like "/lsk_l1". Defaults to the stock UNS-1 module's
        // "uns1/fms1"; leaves that re-namespace (e.g. Q4XP -> "FJS/Q4XP/fms1")
        // override it.
        virtual std::string commandPathPrefix() const;

        // Number of CDU text/style lines exposed by this integration. Defaults
        // to 11 (Q4XP's value); override if a leaf aircraft exposes a
        // different count.
        virtual int screenLineCount() const {
            return 11;
        }

    public:
        UNS1FMCProfile(ProductFMC *product);

        const std::vector<std::string> &displayDatarefs() const override;
        const std::vector<FMCButtonDef> &buttonDefs() const override;
        const std::unordered_map<FMCKey, const FMCButtonDef *> &buttonKeyMap() const override;
        const std::map<char, FMCTextColor> &colorMap() const override;
        void mapCharacter(std::vector<uint8_t> *buffer, uint8_t character, bool isFontSmall) override;
        void updatePage(std::vector<std::vector<char>> &page) override;
        void buttonPressed(const FMCButtonDef *button, XPLMCommandPhase phase) override;

    private:
        // Per-instance (not static/shared) caches: displayPathPrefix() and
        // commandPathPrefix() are virtual, so a function-local static cache
        // here would be shared across every leaf aircraft subclass and could
        // return one aircraft's datarefs to another's profile instance.
        mutable std::optional<std::vector<std::string>> cachedDisplayDatarefs;
        mutable std::optional<std::vector<FMCButtonDef>> cachedButtonDefs;
        mutable std::optional<std::unordered_map<FMCKey, const FMCButtonDef *>> cachedButtonKeyMap;
};

#endif // UNS1_FMC_PROFILE_H
