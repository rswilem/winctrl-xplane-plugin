#ifndef PLUGINS_MENU_H
#define PLUGINS_MENU_H

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

        XPLMMenuID mainMenuId;
        int mainMenuItemIndex;
        int nextItemId;
        std::map<int, std::pair<int, std::function<void(int)>>> menuCallbacks; // itemId -> (itemIndex, callback)
        std::map<int, std::string> itemNames;                                  // itemId -> name
        std::map<int, bool> persistentItems;                                   // itemId -> isPersistent
        std::map<int, std::pair<XPLMMenuID, std::vector<MenuItem>>> submenus;  // itemId -> (submenuId, items)
        std::map<int, XPLMMenuID> itemToMenuId;                                // itemId -> menuId (for locating which menu an item belongs to)
        std::map<int, std::vector<int>> submenuChildren;                       // submenuId -> list of child itemIds

        static void handleMenuAction(void *mRef, void *iRef);
        void ensureMenuExists();
        int addItemInternal(const std::string &name, const MenuItemContent &content, bool persistent, bool checked, int submenuId);
        void addMenuItemsToMenu(XPLMMenuID parentMenu, const std::vector<MenuItem> &items, bool persistent);

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
};

#endif
