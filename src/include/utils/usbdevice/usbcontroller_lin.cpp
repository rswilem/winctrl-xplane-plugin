#if LIN
#include "appstate.h"
#include "config.h"
#include "usbcontroller.h"
#include "usbdevice.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <libudev.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <XPLMUtilities.h>

USBController *USBController::instance = nullptr;
static std::atomic<bool> shouldStopMonitoring{false};

USBController::USBController() {
    struct udev *udev = udev_new();
    if (!udev) {
        Logger::getInstance()->error("Failed to create udev context");
        return;
    }

    hidManager = udev_monitor_new_from_netlink(udev, "udev");
    if (!hidManager) {
        Logger::getInstance()->error("Failed to create udev monitor");
        udev_unref(udev);
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(hidManager, "hidraw", nullptr);
    udev_monitor_enable_receiving(hidManager);

    shouldStopMonitoring = false;
    std::thread monitorThread([this]() {
        monitorDevices();
    });
    monitorThread.detach();
}

USBController::~USBController() {
    destroy();
}

USBController *USBController::getInstance() {
    if (instance == nullptr) {
        instance = new USBController();
    }
    return instance;
}

void USBController::destroy() {
    shouldStopMonitoring = true;

    // Give the monitoring thread time to exit gracefully
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto ptr : devices) {
        delete ptr;
    }
    devices.clear();

    if (hidManager) {
        struct udev *udev = udev_monitor_get_udev(hidManager);
        udev_monitor_unref(hidManager);
        udev_unref(udev);
        hidManager = nullptr;
    }

    instance = nullptr;
}

USBDevice *USBController::createDeviceFromPath(const std::string &devicePath) {
    int fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
        return nullptr;
    }

    struct hidraw_devinfo info;
    if (ioctl(fd, HIDIOCGRAWINFO, &info) < 0 || info.vendor != WINCTRL_VENDOR_ID) {
        close(fd);
        return nullptr;
    }

    char name[256] = {};
    if (ioctl(fd, HIDIOCGRAWNAME(sizeof(name)), name) < 0) {
        close(fd);
        return nullptr;
    }

    return USBDevice::Device(fd, info.vendor, info.product, "WINCTRL", std::string(name));
}

bool USBController::deviceExistsAtPath(const std::string &devicePath) {
    for (auto *dev : devices) {
        if (dev->hidDevice >= 0) {
            char existingPath[256];
            snprintf(existingPath, sizeof(existingPath), "/proc/self/fd/%d", dev->hidDevice);
            char linkTarget[256];
            ssize_t len = readlink(existingPath, linkTarget, sizeof(linkTarget) - 1);
            if (len > 0) {
                linkTarget[len] = '\0';
                if (strcmp(linkTarget, devicePath.c_str()) == 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

void USBController::addDeviceFromPath(const std::string &devicePath) {
    AppState::getInstance()->executeAfter(0, [this, devicePath]() {
        if (deviceExistsAtPath(devicePath)) {
            return;
        }

        USBDevice *device = createDeviceFromPath(devicePath);
        if (device) {
            devices.push_back(device);
        }
    });
}

void USBController::enumerateDevices() {
    if (!AppState::getInstance()->pluginInitialized) {
        return;
    }

    DIR *dir = opendir("/dev");
    if (!dir) {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "hidraw", 6) == 0) {
            std::string devicePath = "/dev/" + std::string(entry->d_name);
            addDeviceFromPath(devicePath);
        }
    }
    closedir(dir);
}

void USBController::monitorDevices() {
    if (!AppState::getInstance()->pluginInitialized || shouldStopMonitoring) {
        return;
    }

    int fd = udev_monitor_get_fd(hidManager);
    while (!shouldStopMonitoring) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        struct timeval timeout = {1, 0};
        int ret = select(fd + 1, &fds, nullptr, nullptr, &timeout);

        if (ret > 0 && FD_ISSET(fd, &fds) && !shouldStopMonitoring) {
            struct udev_device *device = udev_monitor_receive_device(hidManager);
            if (device) {
                const char *action = udev_device_get_action(device);
                if (strcmp(action, "add") == 0) {
                    DeviceAddedCallback(this, device);
                } else if (strcmp(action, "remove") == 0) {
                    DeviceRemovedCallback(this, device);
                }
                udev_device_unref(device);
            }
        }
    }
    Logger::getInstance()->debug("Monitoring thread is exiting\n");
}

void USBController::DeviceAddedCallback(void *context, struct udev_device *device) {
    auto *self = static_cast<USBController *>(context);

    const char *devicePath = udev_device_get_devnode(device);
    if (!devicePath) {
        return;
    }

    self->addDeviceFromPath(std::string(devicePath));
}

void USBController::DeviceRemovedCallback(void *context, struct udev_device *device) {
    auto *self = static_cast<USBController *>(context);

    const char *devicePath = udev_device_get_devnode(device);
    if (!devicePath) {
        return;
    }

    // First pass: disconnect
    for (auto it = self->devices.begin(); it != self->devices.end(); ++it) {
        if ((*it)->hidDevice >= 0) {
            char existingPath[256];
            snprintf(existingPath, sizeof(existingPath), "/proc/self/fd/%d", (*it)->hidDevice);
            char linkTarget[256];
            ssize_t len = readlink(existingPath, linkTarget, sizeof(linkTarget) - 1);
            if (len > 0) {
                linkTarget[len] = '\0';
                if (strcmp(linkTarget, devicePath) == 0) {
                    (*it)->blackout();
                    (*it)->disconnect();
                    break;
                }
            }
        } else {
            (*it)->blackout();
            (*it)->disconnect();
            break;
        }
    }

    // Second pass: deferred erase
    AppState::getInstance()->executeAfter(0, [self, devicePath = std::string(devicePath)]() {
        for (auto it = self->devices.begin(); it != self->devices.end();) {
            if (!(*it) || !(*it)->connected) {
                delete *it;
                it = self->devices.erase(it);
            } else {
                char existingPath[256];
                snprintf(existingPath, sizeof(existingPath), "/proc/self/fd/%d", (*it)->hidDevice);
                char linkTarget[256];
                ssize_t len = readlink(existingPath, linkTarget, sizeof(linkTarget) - 1);
                if (len > 0) {
                    linkTarget[len] = '\0';
                    if (strcmp(linkTarget, devicePath.c_str()) == 0) {
                        delete *it;
                        it = self->devices.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
    });
}
#endif
