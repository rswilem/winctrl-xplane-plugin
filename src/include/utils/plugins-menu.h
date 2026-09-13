#ifndef PLUGINS_MENU_H
#define PLUGINS_MENU_H

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <XPLMMenus.h>

struct MenuItem;

using MenuItemContent = std::variant<std::function<void(int)>, std::vector<MenuItem>>;

struct MenuItem {
        std::string name;
        bool checked = false;
        MenuItemContent content;
        // Set on the "Enabled" entry only. It marks the submenu as a device
        // submenu, so PluginsMenu can append the X-Plane assignment items to
        // it without every product repeating them.
        uint16_t deviceProductId = 0;

        static MenuItem Separator() {
            MenuItem item;
            item.name = "---";
            item.content = [](int) {};
            return item;
        }
};

class PluginsMenu {
    private:
        PluginsMenu();
        ~PluginsMenu();
        static PluginsMenu *instance;

        XPLMMenuID mainMenuId = nullptr;
        int mainMenuItemIndex = -1;
        int nextItemId = 0;
        std::map<int, std::pair<int, std::function<void(int)>>> menuCallbacks; // itemId -> (itemIndex, callback)
        std::map<int, std::string> itemNames;                                  // itemId -> name
        std::map<int, bool> persistentItems;                                   // itemId -> isPersistent
        std::map<int, std::pair<XPLMMenuID, std::vector<MenuItem>>> submenus;  // itemId -> (submenuId, items)
        std::map<int, XPLMMenuID> itemToMenuId;                                // itemId -> menuId (for locating which menu an item belongs to)
        std::map<int, std::vector<int>> submenuChildren;                       // submenuId -> list of child itemIds
        std::vector<int> disabledDeviceItemIds;                                // stub submenus of switched-off devices
        std::map<int, uint16_t> bindingCountItems;                             // itemId -> productId, label follows the live count
        std::map<int, uint16_t> bindingToggleItems;                            // itemId -> productId, check follows the preference

        static void handleMenuAction(void *mRef, void *iRef);
        void ensureMenuExists();
        int addItemInternal(const std::string &name, const MenuItemContent &content, bool persistent, bool checked, int submenuId);
        void addMenuItemsToMenu(XPLMMenuID parentMenu, const std::vector<MenuItem> &items, bool persistent);
        void appendXPlaneBindingItems(XPLMMenuID parentMenu, uint16_t productId, bool persistent, int parentSubmenuId);
        int registerAppendedItem(XPLMMenuID parentMenu, const std::string &name, const std::function<void(int)> &callback, bool persistent, int parentSubmenuId, bool checked);

    public:
        static PluginsMenu *getInstance();
        int addItem(const std::string &name, const MenuItemContent &content, bool checked = false, int submenuId = -1);
        int addPersistentItem(const std::string &name, const MenuItemContent &content, bool checked = false, int submenuId = -1);
        void removeItem(int itemId);
        void setItemName(int itemIndex, const std::string &name);
        void setItemChecked(int itemId, bool checked);
        void uncheckSubmenuSiblings(int itemId);
        bool isItemChecked(int itemIndex);
        void clearAllItems();
        void teardown();

        // The "Enabled" entry every device submenu ends with. Unchecking it
        // releases the device, so other software (MobiFlight, SimAppPro) can
        // drive it instead. Lives here rather than in USBDevice because the
        // stresstest build shares usbdevice.h but has no menu SDK.
        // respectsXPlaneBindings marks devices whose buttons the plugin acts
        // on, and so can be overridden by an X-Plane joystick assignment. Pass
        // false for a device the plugin drives no buttons on (the joysticks,
        // NWS, Orion throttle): an assignment there is what the user wants,
        // and the assignment items would only confuse.
        static MenuItem deviceEnabledItem(uint16_t productId, bool respectsXPlaneBindings = true);
        // Rebuilds the stub submenus of switched-off devices. Those stubs are
        // the only way back: an unclaimed device never adds a submenu itself.
        void syncDisabledDeviceItems();
        // Re-labels and re-checks the X-Plane assignment items of every device
        // submenu. Called whenever the bindings are rebuilt.
        void refreshXPlaneBindingItems();
};

#endif
