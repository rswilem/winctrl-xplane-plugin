## WINCTRL plugin for X-Plane

For X-Plane 11 and X-Plane 12. Formerly the Winwing plugin for X-Plane.

This plugin allows you to use WINCTRL devices in X-Plane on MacOS and Linux.
The plugin theoretically works on Windows, but is not required there, as WINCTRL provides a native Windows driver through the SimAppPro software.

### Installation

1. Download the latest release from the releases page.
2. Unzip the downloaded file.
3. Copy the `winctrl` folder to your X-Plane `Resources/plugins` directory.
4. Start X-Plane.
5. Updating can be done by replacing the `winctrl` folder or using Skunkcrafts Updater if you have it installed.

For linux, see the [Linux udev rules](#linux-udev-rules) section below to ensure proper permissions and stable device recognition.

### Contributing

- Fork the repository.
- Download the latest X-Plane SDK from https://developer.x-plane.com/sdk/plugin-sdk-downloads/.
- Unzip and copy the `SDK/` folder to the root of the repository.
- Make your changes.
- Test your changes in X-Plane. Datareftool plugin recommented, and make sure you uninstall FlyWithLua as it can interfere with the "Reload Plugins" functionality.
- Commit your changes and push to your fork.
- Create a pull request with a description of your changes.

### Usage

- The plugin will automatically detect your WINCTRL devices.
- There are no user-configurable settings at this time.

### Compatibility Matrix

The matrix below shows device and aircraft compatibility. Devices are listed vertically, aircraft horizontally.
| &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Device&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ToLiss A3xx | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Zibo / LevelUp 737 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; X-Crafts E-jets | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; FlightFactor 767 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; FlightFactor 777 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; FlightFactor A350 V1 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Rotate MD-11 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; IXEG 737 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; SSG 747 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; JustFlight 146 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Laminar 737 | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Laminar A330 (Aerogenesis) | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Laminar Citation X | &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Laminar C172 |
|--- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **URSA MINOR Airline Joystick L+R** | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; | &check; |
| **FMC (MCDU, PFP 3N/4/7)** | &check; | &check; | &check; | &check; | &check; | &nbsp; | &check; | &check; | &check; | &nbsp; | &nbsp; | &check; | &check; | &nbsp; |
| **3N PAP MCP** | &nbsp; | &check; | &check; | &nbsp; | &check; | &nbsp; | &check; | &nbsp; | &nbsp; | &nbsp; | &check; | &nbsp; | &nbsp; | &nbsp; |
| **3N / 3M PDC** | &nbsp; | &check; | &nbsp; | &nbsp; | &check; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; |
| **FCU (+ optional EFIS L+R)** | &check; | &nbsp; | &check; | &check; | &check; | &check; | &nbsp; | &nbsp; | &nbsp; | &check; | &check; | &check; | &nbsp; | &check; |
| **32 ECAM** | &check; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; |
| **32 AGP Metal** | &check; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; |
| **URSA MINOR 32 Throttle Metal (+ optional 32 PAC Metal)** | &check; | &check; | &check; | &nbsp; | &check; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; | &nbsp; |

### Known Issues

- The SSG 747 does not expose any colour datarefs yet. Therefore, the PFP will not show the correct colours.
- The SSG 747 has a dual FMC, but the datarefs seem to overwrite eachother.

### Credits

- [@Schenlap](https://github.com/schenlap) for the initial python implementation and providing the HID protocol documentation.
- [@zodiac1214](https://github.com/zodiac1214) for the Ursa Minor Joystick HID protocol.
- [@CyberGuerro](https://github.com/cyberguerro) for the PFP3N HID protocol and a lot of testing work.
- [@claaslange](https://github.com/claaslange) for the FCU-EFIS integration.
- [@shred86](https://github.com/shred86) for identifying the Ursa Minor Joystick R.
- [@Belnadifia](https://github.com/Belnadifia) for the complete PAP3 HID protocol and Zibo profile.
- [@ColinM9991](https://github.com/ColinM9991) for the FF777 profile and parts of the FMC protocol.
- [@tukan68](https://github.com/tukan68) for FF767 profile.
- [@verres1](https://github.com/verres1) for enriching the FF777 profile.
- [@teropa](https://forums.x-plane.org/profile/1028374-teropa/) for data capture and testing of multiple devices.
- [@SoarByWire](https://forums.x-plane.org/profile/411164-soarbywire/) for the testing of multiple devices.
- [@MortyMars](https://github.com/MortyMars) for the FCU/EFIS FF777, FF767, and FFA350 v1 profiles.
- [@loftwing](https://github.com/loftwing) for the JF146 profile and testing.

### Linux udev rules

To ensure WINCTRL panels are accessible without root and have stable device names, create a udev rules file:

```bash
sudo nano /etc/udev/rules.d/99-winctrl.rules
```

```udev
KERNEL=="hidraw*", ATTRS{idProduct}=="bc27", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-ursa-minor-l"
KERNEL=="hidraw*", ATTRS{idProduct}=="bc28", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-ursa-minor-r"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb36", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-mcdu32-cpt"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3e", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-mcdu32-fo"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3a", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-mcdu32-obs"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb35", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp3n-cpt"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb39", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp3n-fo"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3d", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp3n-obs"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb38", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp4-cpt"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb40", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp4-fo"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3c", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp4-obs"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb37", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp7-cpt"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3f", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp7-fo"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb3b", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pfp7-obs"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb10", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-fcu"
KERNEL=="hidraw*", ATTRS{idProduct}=="bc1e", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-fcu-efis-r"
KERNEL=="hidraw*", ATTRS{idProduct}=="bc1d", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-fcu-efis-l"
KERNEL=="hidraw*", ATTRS{idProduct}=="ba01", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-fcu-efis-lr"
KERNEL=="hidraw*", ATTRS{idProduct}=="bf0f", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-pap3"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb70", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-ecam"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb80", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-agp"
KERNEL=="hidraw*", ATTRS{idProduct}=="b920", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-throttle-l"
KERNEL=="hidraw*", ATTRS{idProduct}=="b930", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-throttle-r"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb61", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-3n-pdc-l"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb62", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-3n-pdc-r"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb51", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-3m-pdc-l"
KERNEL=="hidraw*", ATTRS{idProduct}=="bb52", ATTRS{idVendor}=="4098", MODE="0666", SYMLINK+="winctrl-3m-pdc-r"
```

### Demonstration

Also see the [X-Plane.org forum thread](https://forums.x-plane.org/files/file/95987-winctrl-plugin-for-x-plane-mac-linux-windows/) for more information and discussion.

<img src="https://github.com/user-attachments/assets/75d4e3e0-af9e-488f-bd5e-2d834bea110d" alt="Airbus A20N" width="256" />
<img src="https://github.com/user-attachments/assets/8f5750e2-f913-479a-9f7a-6e3d6c31382d" alt="Boeing B738" width="256" />

