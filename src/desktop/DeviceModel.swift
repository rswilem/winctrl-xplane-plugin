//
//  DeviceModel.swift
//  WinctrlDesktop
//
//  Created by Ramon Swilem on 08/07/2025.
//

import SwiftUI
import Foundation
import Combine

// C bridge imports for device-specific functions
@_silgen_name("getDeviceCount")
func c_getDeviceCount() -> Int32
@_silgen_name("getDeviceName")
func c_getDeviceName(_ deviceIndex: Int32) -> UnsafePointer<CChar>?
@_silgen_name("getDeviceType")
func c_getDeviceType(_ deviceIndex: Int32) -> UnsafePointer<CChar>?
@_silgen_name("getDeviceProductId")
func c_getDeviceProductId(_ deviceIndex: Int32) -> UInt16
@_silgen_name("isDeviceConnected")
func c_isDeviceConnected(_ deviceIndex: Int32) -> Bool

// Device handle access
@_silgen_name("getDeviceHandle")
func c_getDeviceHandle(_ deviceIndex: Int32) -> UnsafeRawPointer?
@_silgen_name("getJoystickHandle")
func c_getJoystickHandle(_ deviceIndex: Int32) -> UnsafeRawPointer?
@_silgen_name("getFMCHandle")
func c_getFMCHandle(_ deviceIndex: Int32) -> UnsafeRawPointer?
@_silgen_name("getFCUEfisHandle")
func c_getFCUEfisHandle(_ deviceIndex: Int32) -> UnsafeRawPointer?

// Generic device functions via handle
@_silgen_name("device_connect")
func c_device_connect(_ handle: UnsafeRawPointer) -> Bool
@_silgen_name("device_disconnect")
func c_device_disconnect(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("device_update")
func c_device_update(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("device_force_state_sync")
func c_device_force_state_sync(_ handle: UnsafeRawPointer) -> Void

// Joystick functions via handle
@_silgen_name("joystick_setVibration")
func c_joystick_setVibration(_ handle: UnsafeRawPointer, _ vibration: UInt8) -> Bool
@_silgen_name("joystick_setLedBrightness")
func c_joystick_setLedBrightness(_ handle: UnsafeRawPointer, _ brightness: UInt8) -> Bool

// FMC functions via handle
@_silgen_name("fmc_clearDisplay")
func c_fmc_clearDisplay(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("fmc_unloadProfile")
func c_fmc_unloadProfile(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("fmc_showBackground")
func c_fmc_showBackground(_ handle: UnsafeRawPointer, _ variant: Int32) -> Void
@_silgen_name("fmc_setLed")
func c_fmc_setLed(_ handle: UnsafeRawPointer, _ ledId: Int32, _ value: UInt8) -> Bool
@_silgen_name("fmc_setLedBrightness")
func c_fmc_setLedBrightness(_ handle: UnsafeRawPointer, _ ledId: Int32, _ brightness: UInt8) -> Void
@_silgen_name("fmc_writeData")
func c_fmc_writeData(_ handle: UnsafeRawPointer, _ data: UnsafePointer<UInt8>, _ length: Int32) -> Bool
@_silgen_name("fmc_setFont")
func c_fmc_setFont(_ handle: UnsafeRawPointer, _ fontType: Int32) -> Void
@_silgen_name("fmc_setScreenLayout")
func c_fmc_setScreenLayout(_ handle: UnsafeRawPointer, _ fontType: Int32, _ characterHeight: Int32, _ characterWidth: Int32, _ x: Int32, _ y: Int32) -> Void

// FCU-EFIS functions via handle
@_silgen_name("fcuefis_clear")
func c_fcuefis_clear(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("fcuefis_efisRightClear")
func c_fcuefis_efisRightClear(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("fcuefis_efisLeftClear")
func c_fcuefis_efisLeftClear(_ handle: UnsafeRawPointer) -> Void
@_silgen_name("fcuefis_setLed")
func c_fcuefis_setLed(_ handle: UnsafeRawPointer, _ ledId: Int32, _ value: UInt8) -> Bool
@_silgen_name("fcuefis_setLedBrightness")
func c_fcuefis_setLedBrightness(_ handle: UnsafeRawPointer, _ ledId: Int32, _ brightness: UInt8) -> Void
@_silgen_name("fcuefis_testDisplay")
func c_fcuefis_testDisplay(_ handle: UnsafeRawPointer, _ testType: UnsafePointer<CChar>) -> Void
@_silgen_name("fcuefis_efisRightTestDisplay")
func c_fcuefis_efisRightTestDisplay(_ handle: UnsafeRawPointer, _ testType: UnsafePointer<CChar>) -> Void
@_silgen_name("fcuefis_efisLeftTestDisplay")
func c_fcuefis_efisLeftTestDisplay(_ handle: UnsafeRawPointer, _ testType: UnsafePointer<CChar>) -> Void

enum DeviceType: String, CaseIterable {
    case joystick = "joystick"
    case fmc = "fmc"
    case fcuEfis = "fcu-efis"
    case unknown = "unknown"
}

struct WinctrlDevice: Identifiable, Equatable, Hashable {
    let id: Int
    let name: String
    let type: DeviceType
    let productId: UInt16
    var isConnected: Bool
    
    // Device handles (cached for performance)
    private let deviceHandle: UnsafeRawPointer?
    private let joystickHandle: UnsafeRawPointer?
    private let fmcHandle: UnsafeRawPointer?
    private let fcuEfisHandle: UnsafeRawPointer?
    
    init(id: Int, name: String, type: DeviceType, productId: UInt16, isConnected: Bool) {
        self.id = id
        self.name = name
        self.type = type
        self.productId = productId
        self.isConnected = isConnected
        
        // Cache device handles
        self.deviceHandle = c_getDeviceHandle(Int32(id))
        self.joystickHandle = c_getJoystickHandle(Int32(id))
        self.fmcHandle = c_getFMCHandle(Int32(id))
        self.fcuEfisHandle = c_getFCUEfisHandle(Int32(id))
    }
    
    static func == (lhs: WinctrlDevice, rhs: WinctrlDevice) -> Bool {
        return lhs.id == rhs.id && 
               lhs.name == rhs.name && 
               lhs.type == rhs.type && 
               lhs.productId == rhs.productId && 
               lhs.isConnected == rhs.isConnected
    }
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
        hasher.combine(name)
        hasher.combine(type)
        hasher.combine(productId)
        hasher.combine(isConnected)
    }
    
    // Device wrapper methods
    @discardableResult
    func connect() -> Bool {
        guard let handle = deviceHandle else { return false }
        return c_device_connect(handle)
    }
    
    func disconnect() {
        guard let handle = deviceHandle else { return }
        c_device_disconnect(handle)
    }
    
    func update() {
        guard let handle = deviceHandle else { return }
        c_device_update(handle)
    }
    
    func forceStateSync() {
        guard let handle = deviceHandle else { return }
        c_device_force_state_sync(handle)
    }

    // Joystick wrapper methods
    var joystick: JoystickWrapper? {
        guard let handle = joystickHandle else { return nil }
        return JoystickWrapper(handle: handle)
    }
    
    // FMC wrapper methods  
    var fmc: FMCWrapper? {
        guard let handle = fmcHandle else { return nil }
        return FMCWrapper(handle: handle)
    }
    
    // FCU-EFIS wrapper methods
    var fcuEfis: FCUEfisWrapper? {
        guard let handle = fcuEfisHandle else { return nil }
        return FCUEfisWrapper(handle: handle)
    }
}

// Swift wrapper for Joystick functions
struct JoystickWrapper {
    private let handle: UnsafeRawPointer
    
    init(handle: UnsafeRawPointer) {
        self.handle = handle
    }
    
    @discardableResult
    func setVibration(_ vibration: UInt8) -> Bool {
        return c_joystick_setVibration(handle, vibration)
    }
    
    @discardableResult
    func setLedBrightness(_ brightness: UInt8) -> Bool {
        return c_joystick_setLedBrightness(handle, brightness)
    }
}

// Swift wrapper for FMC functions
struct FMCWrapper {
    private let handle: UnsafeRawPointer
    
    // LED IDs based on the C++ enum
    enum LEDId: Int {
        case backlight = 0
        case screenBacklight = 1
        case overallLedsBrightness = 2
        case fail = 8
        case fm = 9
        case mcdu = 10
        case menu = 11
        case fm1 = 12
        case ind = 13
        case rdy = 14
        case status = 15
        case fm2 = 16
    }
    
    init(handle: UnsafeRawPointer) {
        self.handle = handle
    }
    
    func clearDisplay() {
        c_fmc_clearDisplay(handle)
    }
    
    func unloadProfile() {
        c_fmc_unloadProfile(handle)
    }
    
    func showBackground(_ variant: Int) {
        c_fmc_showBackground(handle, Int32(variant))
    }
    
    @discardableResult
    func setLed(_ ledId: LEDId, value: UInt8) -> Bool {
        return c_fmc_setLed(handle, Int32(ledId.rawValue), value)
    }
    
    @discardableResult
    func setLed(_ ledId: LEDId, state: Bool) -> Bool {
        return setLed(ledId, value: state ? 255 : 0)
    }
    
    @discardableResult
    func setLed(_ ledId: Int, value: UInt8) -> Bool {
        return c_fmc_setLed(handle, Int32(ledId), value)
    }
    
    @discardableResult
    func setLed(_ ledId: Int, state: Bool) -> Bool {
        return setLed(ledId, value: state ? 255 : 0)
    }
    
    func setLedBrightness(_ ledId: LEDId, brightness: UInt8) {
        c_fmc_setLedBrightness(handle, Int32(ledId.rawValue), brightness)
    }
    
    func setLedBrightness(_ ledId: Int, brightness: UInt8) {
        c_fmc_setLedBrightness(handle, Int32(ledId), brightness)
    }
    
    // Convenience methods for common LEDs
    func setBacklight(_ brightness: UInt8) {
        setLedBrightness(.backlight, brightness: brightness)
    }
    
    func setScreenBacklight(_ brightness: UInt8) {
        setLedBrightness(.screenBacklight, brightness: brightness)
    }
    
    func setOverallLedsBrightness(_ brightness: UInt8) {
        setLedBrightness(.overallLedsBrightness, brightness: brightness)
    }
    
    // Write raw data to the device
    @discardableResult
    func writeData(_ data: [UInt8]) -> Bool {
        return data.withUnsafeBufferPointer { bufferPointer in
            guard let baseAddress = bufferPointer.baseAddress else { return false }
            return c_fmc_writeData(handle, baseAddress, Int32(data.count))
        }
    }
    
    // Font types available for the FMC
    enum FontType: Int, CaseIterable {
        case standard = 0
        case airbus = 1
        case b737 = 2
        case xcrafts = 3
        case vga1 = 4
        
        
        var displayName: String {
            switch self {
            case .standard: return "Default"
            case .airbus: return "Airbus"
            case .b737: return "737"
            case .xcrafts: return "X-Crafts E-Jet"
            case .vga1: return "VGA 1"
            }
        }
    }
    
    // Set the font for the FMC display
    func setFont(_ fontType: FontType) {
        c_fmc_setFont(handle, Int32(fontType.rawValue))
    }

    // Apply the SimAppPro "Screen Layout Settings" as one unit: Character Size
    // (width x height of each character) plus Screen Position (top-left x/y). Used for
    // PFP devices so 14 display rows line up with the physical LSK keys. Defaults are
    // the MCDU spec (character 23 x 29, position 16/17).
    func setScreenLayout(_ fontType: FontType, characterHeight: Int = 29, characterWidth: Int = 23, x: Int = 16, y: Int = 17) {
        c_fmc_setScreenLayout(handle, Int32(fontType.rawValue), Int32(characterHeight), Int32(characterWidth), Int32(x), Int32(y))
    }
}

// Swift wrapper for FCU-EFIS functions
struct FCUEfisWrapper {
    private let handle: UnsafeRawPointer
    
    // LED IDs based on the C++ enum
    enum LEDId: Int {
        // FCU LEDs
        case backlight = 0
        case screenBacklight = 1
        case locGreen = 3
        case ap1Green = 5
        case ap2Green = 7
        case athrGreen = 9
        case expedGreen = 11
        case apprGreen = 13
        case flagGreen = 17
        case expedYellow = 30
        
        // EFIS Right LEDs (100-199)
        case efisrBacklight = 100
        case efisrScreenBacklight = 101
        case efisrFlagGreen = 102
        case efisrFdGreen = 103
        case efisrLsGreen = 104
        case efisrCstrGreen = 105
        case efisrWptGreen = 106
        case efisrVordGreen = 107
        case efisrNdbGreen = 108
        case efisrArptGreen = 109
        
        // EFIS Left LEDs (200-299)
        case efislBacklight = 200
        case efislScreenBacklight = 201
        case efislFlagGreen = 202
        case efislFdGreen = 203
        case efislLsGreen = 204
        case efislCstrGreen = 205
        case efislWptGreen = 206
        case efislVordGreen = 207
        case efislNdbGreen = 208
        case efislArptGreen = 209
    }
    
    init(handle: UnsafeRawPointer) {
        self.handle = handle
    }
    
    func clear() {
        c_fcuefis_clear(handle)
    }
    
    @discardableResult
    func setLed(_ ledId: LEDId, _ value: UInt8) -> Bool {
        return c_fcuefis_setLed(handle, Int32(ledId.rawValue), value)
    }
    
    @discardableResult
    func setLed(_ ledId: Int, _ value: UInt8) -> Bool {
        return c_fcuefis_setLed(handle, Int32(ledId), value)
    }
    
    func setLedBrightness(_ ledId: LEDId, brightness: UInt8) {
        c_fcuefis_setLedBrightness(handle, Int32(ledId.rawValue), brightness)
    }
    
    func setLedBrightness(_ ledId: Int, brightness: UInt8) {
        c_fcuefis_setLedBrightness(handle, Int32(ledId), brightness)
    }
    
    func testDisplay(_ testType: String) {
        testType.withCString { cString in
            c_fcuefis_testDisplay(handle, cString)
        }
    }
    
    func efisRightTestDisplay(_ testType: String) {
        testType.withCString { cString in
            c_fcuefis_efisRightTestDisplay(handle, cString)
        }
    }
    
    func efisLeftTestDisplay(_ testType: String) {
        testType.withCString { cString in
            c_fcuefis_efisLeftTestDisplay(handle, cString)
        }
    }
    
    func efisRightClear() {
        c_fcuefis_efisRightClear(handle)
    }
    
    func efisLeftClear() {
        c_fcuefis_efisLeftClear(handle)
    }
    
    // Convenience methods for common LEDs
    func setBacklight(_ brightness: UInt8) {
        setLedBrightness(.backlight, brightness: brightness)
    }
    
    func setScreenBacklight(_ brightness: UInt8) {
        setLedBrightness(.screenBacklight, brightness: brightness)
    }
    
    func setEfisRightBacklight(_ brightness: UInt8) {
        setLedBrightness(.efisrBacklight, brightness: brightness)
    }
  
  func setEfisRightScreenBacklight(_ brightness: UInt8) {
    setLedBrightness(.efisrScreenBacklight, brightness: brightness)
  }
    
    func setEfisLeftBacklight(_ brightness: UInt8) {
        setLedBrightness(.efislBacklight, brightness: brightness)
    }
  
  func setEfisLeftScreenBacklight(_ brightness: UInt8) {
    setLedBrightness(.efislScreenBacklight, brightness: brightness)
  }
}

@MainActor
class DeviceManager: ObservableObject {
    @Published var devices: [WinctrlDevice] = []
    @Published var selectedDeviceId: Int?
    
    var selectedDevice: WinctrlDevice? {
        guard let id = selectedDeviceId else { return nil }
        return devices.first { $0.id == id }
    }
    
    func refreshDevices() {
        let count = Int(c_getDeviceCount())
        var newDevices: [WinctrlDevice] = []
        
        for i in 0..<count {
            guard let namePtr = c_getDeviceName(Int32(i)),
                  let typePtr = c_getDeviceType(Int32(i)) else {
                continue
            }
            
            let name = String(cString: namePtr)
            let typeString = String(cString: typePtr)
            let type = DeviceType(rawValue: typeString) ?? .unknown
            let productId = c_getDeviceProductId(Int32(i))
            let isConnected = c_isDeviceConnected(Int32(i))
            
            let device = WinctrlDevice(
                id: i,
                name: name,
                type: type,
                productId: productId,
                isConnected: isConnected
            )
            newDevices.append(device)
        }
        
        // Only update if devices actually changed (for performance)
        let devicesChanged = devices.count != newDevices.count || 
                           !devices.elementsEqual(newDevices, by: ==)
        
        if devicesChanged {
            devices = newDevices
            
            // Update selected device or select first if none selected
            if let selectedId = selectedDeviceId {
                // Check if selected device still exists
                if !devices.contains(where: { $0.id == selectedId }) {
                    selectedDeviceId = devices.first?.id
                }
            } else if !devices.isEmpty {
                selectedDeviceId = devices.first?.id
            }
        }
    }
    
    func selectDevice(_ device: WinctrlDevice) {
        selectedDeviceId = device.id
    }
}
