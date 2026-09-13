#include "plugins-menu.h"

#include "appstate.h"
#include "config.h"
#include "dataref.h"
#include "logger.hpp"
#include "usbcontroller.h"
#include "usbdevice.h"
#include "xplane-bindings.h"

#include <XPLMPlanes.h>
#include <XPLMUtilities.h>

PluginsMenu *PluginsMenu::instance = nullptr;

PluginsMenu::PluginsMenu() : mainMenuId(nullptr), mainMenuItemIndex(-1), nextItemId(0) {
}

PluginsMenu::~PluginsMenu() {
    instance = nullptr;
}

PluginsMenu *PluginsMenu::getInstance() {
    if (instance == nullptr) {
        instance = new PluginsMenu();
    }

    return instance;
}

void PluginsMenu::ensureMenuExists() {
    if (mainMenuId == nullptr) {
        mainMenuItemIndex = XPLMAppendMenuItem(XPLMFindPluginsMenu(), FRIENDLY_NAME, nullptr, 1);
        mainMenuId = XPLMCreateMenu(FRIENDLY_NAME, XPLMFindPluginsMenu(), mainMenuItemIndex, handleMenuAction, this);
    }
}

void PluginsMenu::addMenuItemsToMenu(XPLMMenuID parentMenu, const std::vector<MenuItem> &items, bool persistent) {
    int parentSubmenuId = -1;
    // Find which submenu this parent menu belongs to
    for (const auto &entry : submenus) {
        if (entry.second.first == parentMenu) {
            parentSubmenuId = entry.first;
            break;
        }
    }

    for (const auto &item : items) {
        // Check if this is a separator
        if (item.name == "---") {
            int subItemId = nextItemId++;
            XPLMAppendMenuSeparator(parentMenu);
            itemNames[subItemId] = item.name;
            persistentItems[subItemId] = persistent;
            itemToMenuId[subItemId] = parentMenu;

            // Track this as a child of the parent submenu
            if (parentSubmenuId != -1) {
                submenuChildren[parentSubmenuId].push_back(subItemId);
            }
        } else if (std::holds_alternative<std::function<void(int)>>(item.content)) {
            // Regular menu item with callback
            const auto &callback = std::get<std::function<void(int)>>(item.content);
            int subItemId = nextItemId++;
            int subItemIndex = XPLMAppendMenuItem(parentMenu, item.name.c_str(), (void *) (intptr_t) subItemId, 0);
            menuCallbacks[subItemId] = std::make_pair(subItemIndex, callback);
            itemNames[subItemId] = item.name;
            persistentItems[subItemId] = persistent;
            itemToMenuId[subItemId] = parentMenu;

            // Track this as a child of the parent submenu
            if (parentSubmenuId != -1) {
                submenuChildren[parentSubmenuId].push_back(subItemId);
            }

            if (item.checked) {
                XPLMCheckMenuItem(parentMenu, subItemIndex, xplm_Menu_Checked);
            }
        } else {
            // Nested submenu
            const auto &nestedItems = std::get<std::vector<MenuItem>>(item.content);
            int subItemId = nextItemId++;
            // Use -1 as sentinel to prevent accidental callback execution
            int subItemIndex = XPLMAppendMenuItem(parentMenu, item.name.c_str(), (void *) (intptr_t) -1, 0);

            // Create nested submenu
            XPLMMenuID nestedSubmenuId = XPLMCreateMenu(item.name.c_str(), parentMenu, subItemIndex, handleMenuAction, this);
            submenus[subItemId] = std::make_pair(nestedSubmenuId, nestedItems);
            itemNames[subItemId] = item.name;
            persistentItems[subItemId] = persistent;
            itemToMenuId[subItemId] = parentMenu;

            // Track this as a child of the parent submenu
            if (parentSubmenuId != -1) {
                submenuChildren[parentSubmenuId].push_back(subItemId);
            }

            // Recursively add items to nested submenu
            addMenuItemsToMenu(nestedSubmenuId, nestedItems, persistent);
        }
    }

    // A submenu carrying the "Enabled" entry is a device submenu, so it ends
    // with the X-Plane assignment items. They come last, after whatever the
    // product added.
    for (const auto &item : items) {
        if (item.deviceProductId != 0) {
            appendXPlaneBindingItems(parentMenu, item.deviceProductId, persistent, parentSubmenuId);
            break;
        }
    }
}

int PluginsMenu::registerAppendedItem(XPLMMenuID parentMenu, const std::string &name, const std::function<void(int)> &callback, bool persistent, int parentSubmenuId, bool checked) {
    int itemId = nextItemId++;
    int itemIndex = XPLMAppendMenuItem(parentMenu, name.c_str(), (void *) (intptr_t) itemId, 0);
    menuCallbacks[itemId] = std::make_pair(itemIndex, callback);
    itemNames[itemId] = name;
    persistentItems[itemId] = persistent;
    itemToMenuId[itemId] = parentMenu;

    if (parentSubmenuId != -1) {
        submenuChildren[parentSubmenuId].push_back(itemId);
    }

    if (checked) {
        XPLMCheckMenuItem(parentMenu, itemIndex, xplm_Menu_Checked);
    }

    return itemId;
}

// Counts are per product, merged across identical units: the preference behind
// the toggle is per device family too, so a per-unit split would say more than
// the switch below it can act on.
static std::string xplaneBindingCountLabel(uint16_t productId) {
    size_t count = XPlaneBindings::getInstance()->boundButtonsForProduct(WINCTRL_VENDOR_ID, productId).size();
    if (count == 0) {
        return "Button overrides: none";
    }

    return "Button overrides: " + std::to_string(count);
}

void PluginsMenu::appendXPlaneBindingItems(XPLMMenuID parentMenu, uint16_t productId, bool persistent, int parentSubmenuId) {
    int separatorId = nextItemId++;
    XPLMAppendMenuSeparator(parentMenu);
    itemNames[separatorId] = "---";
    persistentItems[separatorId] = persistent;
    itemToMenuId[separatorId] = parentMenu;
    if (parentSubmenuId != -1) {
        submenuChildren[parentSubmenuId].push_back(separatorId);
    }

    // Clicking it opens X-Plane's settings, where the assignment has to be
    // cleared, and writes the overridden buttons to the log: that list is what
    // support needs, and the only place the user can see which button.
    int countId = registerAppendedItem(parentMenu, xplaneBindingCountLabel(productId), [productId](int) {
        Dataref::getInstance()->executeCommand("sim/operation/toggle_settings_window");

        std::vector<uint16_t> buttons = XPlaneBindings::getInstance()->boundButtonsForProduct(WINCTRL_VENDOR_ID, productId);
        if (buttons.empty()) {
            Logger::getInstance()->info("Device 0x%04X has no buttons assigned in X-Plane joystick settings\n", productId);
            return;
        }

        std::string list;
        for (uint16_t button : buttons) {
            list += (list.empty() ? "" : ", ") + std::to_string(button);
        }
        Logger::getInstance()->info("Device 0x%04X: X-Plane joystick settings assign button(s) %s; clear them there, or switch off \"Use X-Plane joystick assignments\" in the " FRIENDLY_NAME " menu\n", productId, list.c_str());
    },
        persistent, parentSubmenuId, false);
    bindingCountItems[countId] = productId;

    int toggleId = registerAppendedItem(parentMenu, "Use X-Plane joystick assignments", [productId](int itemId) {
        const DeviceFamily *family = USBDevice::FamilyForProduct(productId);
        if (family == nullptr) {
            return;
        }

        bool uses = !USBDevice::FamilyUsesXPlaneBindings(*family);
        USBDevice::SetFamilyUsesXPlaneBindings(*family, uses);
        PluginsMenu::getInstance()->refreshXPlaneBindingItems();
    },
        persistent, parentSubmenuId, USBDevice::ProductUsesXPlaneBindings(productId));
    bindingToggleItems[toggleId] = productId;
}

void PluginsMenu::refreshXPlaneBindingItems() {
    for (const auto &[itemId, productId] : bindingCountItems) {
        auto callbackIt = menuCallbacks.find(itemId);
        auto menuIt = itemToMenuId.find(itemId);
        if (callbackIt == menuCallbacks.end() || menuIt == itemToMenuId.end()) {
            continue;
        }

        std::string label = xplaneBindingCountLabel(productId);
        itemNames[itemId] = label;
        XPLMSetMenuItemName(menuIt->second, callbackIt->second.first, label.c_str(), 0);
    }

    for (const auto &[itemId, productId] : bindingToggleItems) {
        setItemChecked(itemId, USBDevice::ProductUsesXPlaneBindings(productId));
    }
}

int PluginsMenu::addItemInternal(const std::string &name, const MenuItemContent &content, bool persistent, bool checked, int submenuId) {
    ensureMenuExists();

    int itemId = nextItemId++;
    XPLMMenuID targetMenu = mainMenuId;

    // If submenuId is provided, find the corresponding submenu
    if (submenuId >= 0) {
        auto it = submenus.find(submenuId);
        if (it != submenus.end()) {
            targetMenu = it->second.first;
        }
    }

    if (std::holds_alternative<std::function<void(int)>>(content)) {
        // Regular menu item
        const auto &callback = std::get<std::function<void(int)>>(content);
        int itemIndex = XPLMAppendMenuItem(targetMenu, name.c_str(), (void *) (intptr_t) itemId, 0);
        menuCallbacks[itemId] = std::make_pair(itemIndex, callback);
        itemNames[itemId] = name;
        persistentItems[itemId] = persistent;
        itemToMenuId[itemId] = targetMenu;

        // Track this as a child of the parent submenu
        if (submenuId >= 0) {
            submenuChildren[submenuId].push_back(itemId);
        }

        if (checked) {
            XPLMCheckMenuItem(targetMenu, itemIndex, xplm_Menu_Checked);
        }

        return itemId;
    } else {
        // Submenu
        const auto &items = std::get<std::vector<MenuItem>>(content);
        // Use -1 as sentinel to prevent accidental callback execution
        int itemIndex = XPLMAppendMenuItem(targetMenu, name.c_str(), (void *) (intptr_t) -1, 0);

        // Create the submenu
        XPLMMenuID newSubmenuId = XPLMCreateMenu(name.c_str(), targetMenu, itemIndex, handleMenuAction, this);
        submenus[itemId] = std::make_pair(newSubmenuId, items);

        itemNames[itemId] = name;
        persistentItems[itemId] = persistent;
        itemToMenuId[itemId] = targetMenu;
        // Store a placeholder in menuCallbacks so we can find the itemIndex
        menuCallbacks[itemId] = std::make_pair(itemIndex, [](int) {});

        // Track this as a child of the parent submenu
        if (submenuId >= 0) {
            submenuChildren[submenuId].push_back(itemId);
        }

        // Add items to the submenu (handles nested submenus recursively)
        addMenuItemsToMenu(newSubmenuId, items, persistent);

        return itemId;
    }
}

int PluginsMenu::addItem(const std::string &name, const MenuItemContent &content, bool checked, int submenuId) {
    return addItemInternal(name, content, false, checked, submenuId);
}

int PluginsMenu::addPersistentItem(const std::string &name, const MenuItemContent &content, bool checked, int submenuId) {
    return addItemInternal(name, content, true, checked, submenuId);
}

void PluginsMenu::removeItem(int itemId) {
    if (mainMenuId == nullptr) {
        return;
    }

    // Find the itemIndex and menu for this itemId
    int itemIndexToRemove = -1;
    XPLMMenuID menuToRemoveFrom = nullptr;
    auto callbackIt = menuCallbacks.find(itemId);
    if (callbackIt != menuCallbacks.end()) {
        itemIndexToRemove = callbackIt->second.first;
    }

    auto menuIdIt = itemToMenuId.find(itemId);
    if (menuIdIt != itemToMenuId.end()) {
        menuToRemoveFrom = menuIdIt->second;
    }

    if (itemIndexToRemove >= 0 && menuToRemoveFrom != nullptr) {
        // If this is a submenu, destroy it and all its children
        auto submenuIt = submenus.find(itemId);
        if (submenuIt != submenus.end()) {
            XPLMDestroyMenu(submenuIt->second.first);

            // Recursively clean up children
            auto childrenIt = submenuChildren.find(itemId);
            if (childrenIt != submenuChildren.end()) {
                for (int childId : childrenIt->second) {
                    menuCallbacks.erase(childId);
                    itemNames.erase(childId);
                    persistentItems.erase(childId);
                    itemToMenuId.erase(childId);
                    submenus.erase(childId);
                    submenuChildren.erase(childId);
                    bindingCountItems.erase(childId);
                    bindingToggleItems.erase(childId);
                }
                submenuChildren.erase(childrenIt);
            }

            submenus.erase(submenuIt);
        }

        // Remove from the menu it belongs to
        XPLMRemoveMenuItem(menuToRemoveFrom, itemIndexToRemove);
        menuCallbacks.erase(itemId);
        itemNames.erase(itemId);
        persistentItems.erase(itemId);
        itemToMenuId.erase(itemId);
        bindingCountItems.erase(itemId);
        bindingToggleItems.erase(itemId);

        // Update stored indices for items in the same menu after the removed one
        for (auto &entry : menuCallbacks) {
            XPLMMenuID entryMenu = mainMenuId;
            auto entryMenuIt = itemToMenuId.find(entry.first);
            if (entryMenuIt != itemToMenuId.end()) {
                entryMenu = entryMenuIt->second;
            }

            if (entryMenu == menuToRemoveFrom) {
                int &storedIndex = entry.second.first;
                if (storedIndex > itemIndexToRemove) {
                    storedIndex--;
                }
            }
        }

        // Remove from submenu children tracking
        for (auto &entry : submenuChildren) {
            auto &children = entry.second;
            auto childIt = std::find(children.begin(), children.end(), itemId);
            if (childIt != children.end()) {
                children.erase(childIt);
            }
        }
    }
}

void PluginsMenu::setItemName(int itemIndex, const std::string &name) {
    ensureMenuExists();
    XPLMSetMenuItemName(mainMenuId, itemIndex, name.c_str(), 0);
}

void PluginsMenu::setItemChecked(int itemId, bool checked) {
    ensureMenuExists();

    auto menuIt = itemToMenuId.find(itemId);
    if (menuIt == itemToMenuId.end()) {
        // Fallback to old behavior for backward compatibility
        XPLMCheckMenuItem(mainMenuId, itemId, checked ? xplm_Menu_Checked : xplm_Menu_Unchecked);
        return;
    }

    XPLMMenuID menuId = menuIt->second;
    auto callbackIt = menuCallbacks.find(itemId);
    if (callbackIt != menuCallbacks.end()) {
        int itemIndex = callbackIt->second.first;
        XPLMCheckMenuItem(menuId, itemIndex, checked ? xplm_Menu_Checked : xplm_Menu_Unchecked);
    }
}

void PluginsMenu::uncheckSubmenuSiblings(int itemId) {
    ensureMenuExists();

    // Find which submenu this item belongs to
    for (const auto &entry : submenuChildren) {
        int submenuId = entry.first;
        const auto &children = entry.second;

        // Check if this item is in this submenu's children
        if (std::find(children.begin(), children.end(), itemId) != children.end()) {
            // Found the parent submenu, now uncheck all siblings
            auto submenuIt = submenus.find(submenuId);
            if (submenuIt != submenus.end()) {
                XPLMMenuID menuId = submenuIt->second.first;

                for (int siblingId : children) {
                    if (siblingId != itemId) {
                        auto callbackIt = menuCallbacks.find(siblingId);
                        if (callbackIt != menuCallbacks.end()) {
                            int itemIndex = callbackIt->second.first;
                            XPLMCheckMenuItem(menuId, itemIndex, xplm_Menu_Unchecked);
                        }
                    }
                }
            }
            return;
        }
    }
}

bool PluginsMenu::isItemChecked(int itemIndex) {
    ensureMenuExists();
    XPLMMenuCheck currentState;
    XPLMCheckMenuItemState(mainMenuId, itemIndex, &currentState);
    return currentState == xplm_Menu_Checked;
}

void PluginsMenu::clearAllItems() {
    if (mainMenuId != nullptr) {
        // Collect persistent items and submenus
        std::vector<std::tuple<int, std::string, MenuItemContent>> persistentItemsToKeep;

        for (const auto &entry : persistentItems) {
            int itemId = entry.first;
            bool isPersistent = entry.second;
            if (isPersistent) {
                auto callbackIt = menuCallbacks.find(itemId);
                auto nameIt = itemNames.find(itemId);
                auto submenuIt = submenus.find(itemId);

                if (nameIt != itemNames.end()) {
                    if (submenuIt != submenus.end()) {
                        // Persistent submenu
                        persistentItemsToKeep.push_back(std::make_tuple(
                            itemId,
                            nameIt->second,
                            submenuIt->second.second));
                    } else if (callbackIt != menuCallbacks.end()) {
                        // Persistent regular item
                        persistentItemsToKeep.push_back(std::make_tuple(
                            itemId,
                            nameIt->second,
                            callbackIt->second.second));
                    }
                }
            }
        }

        // Destroy all non-persistent submenus
        for (const auto &entry : submenus) {
            int itemId = entry.first;
            XPLMMenuID submenuId = entry.second.first;

            // Only destroy if not persistent (will be recreated)
            auto persistentIt = persistentItems.find(itemId);
            bool isPersistent = persistentIt != persistentItems.end() && persistentIt->second;

            if (!isPersistent && submenuId != nullptr) {
                XPLMDestroyMenu(submenuId);
            }
        }

        // Clear everything
        XPLMClearAllMenuItems(mainMenuId);
        menuCallbacks.clear();
        itemNames.clear();
        persistentItems.clear();
        submenus.clear();
        itemToMenuId.clear();
        submenuChildren.clear();
        bindingCountItems.clear();
        bindingToggleItems.clear();
        // The stubs are gone with everything else; their ids must not be reused
        // for removal or they would take a freshly added item down instead.
        disabledDeviceItemIds.clear();
        nextItemId = 0;

        // Re-add persistent items and submenus
        for (const auto &item : persistentItemsToKeep) {
            const std::string &name = std::get<1>(item);
            const auto &callbackOrSubmenu = std::get<2>(item);
            addPersistentItem(name, callbackOrSubmenu);
        }

        syncDisabledDeviceItems();
    }
}

MenuItem PluginsMenu::deviceEnabledItem(uint16_t productId, bool respectsXPlaneBindings) {
    return {.name = "Enabled", .checked = true, .content = [productId](int itemId) {
                const DeviceFamily *family = USBDevice::FamilyForProduct(productId);
                if (family == nullptr) {
                    return;
                }

                USBDevice::SetFamilyEnabled(*family, false);
                // Takes this item's own submenu down with the device, hence the
                // stub added right after.
                USBController::getInstance()->releaseDisabledDevices();
                PluginsMenu::getInstance()->syncDisabledDeviceItems();
            },
        .deviceProductId = productId};
}

void PluginsMenu::syncDisabledDeviceItems() {
    for (int itemId : disabledDeviceItemIds) {
        removeItem(itemId);
    }
    disabledDeviceItemIds.clear();

    // A DeviceFamily reference stays valid: the table has static storage duration.
    for (const auto &family : USBDevice::DeviceFamilies()) {
        if (USBDevice::IsFamilyEnabled(family)) {
            continue;
        }

        disabledDeviceItemIds.push_back(addItem(family.name,
            std::vector<MenuItem>{
                {.name = "Enable", .content = [&family](int itemId) {
                     USBDevice::SetFamilyEnabled(family, true);
                     // Drops this stub; the device adds its own submenu once
                     // enumeration picks it up again.
                     PluginsMenu::getInstance()->syncDisabledDeviceItems();
                     USBController::getInstance()->connectAllDevices();
                 }},
            }));
    }
}

void PluginsMenu::teardown() {
    if (mainMenuId != nullptr) {
        XPLMDestroyMenu(mainMenuId);
        if (mainMenuItemIndex >= 0) {
            XPLMRemoveMenuItem(XPLMFindPluginsMenu(), mainMenuItemIndex);
            mainMenuItemIndex = -1;
        }
        mainMenuId = nullptr;
    }

    menuCallbacks.clear();
    itemNames.clear();
    persistentItems.clear();
    submenus.clear();
    itemToMenuId.clear();
    submenuChildren.clear();
    bindingCountItems.clear();
    bindingToggleItems.clear();
    disabledDeviceItemIds.clear();
    nextItemId = 0;
}

void PluginsMenu::handleMenuAction(void *mRef, void *iRef) {
    if (mRef == nullptr) {
        return;
    }

    auto *self = static_cast<PluginsMenu *>(mRef);
    int itemId = (int) (intptr_t) iRef;

    if (itemId < 0) {
        return;
    }

    try {
        auto it = self->menuCallbacks.find(itemId);
        if (it != self->menuCallbacks.end() && it->second.second) {
            // Invoke a copy: the callback may call removeItem/clearAllItems
            // (e.g. "Reload devices"), which erases the map entry and would
            // destroy the std::function currently executing.
            auto callback = it->second.second;
            callback(itemId);
        }
    } catch (...) {
        // Swallow all exceptions to prevent crashes from menu callbacks
    }
}
