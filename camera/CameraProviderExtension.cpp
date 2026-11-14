/*
 * Copyright (C) 2024 LibreMobileOS Foundation
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CameraProviderExtension.h"

#include <algorithm>
#include <fstream>
#include <string>

// PM8350C flash LED. The rear camera flash of marble is wired to channels 1
// and 4 (torch_0 + torch_3), both driven by switch_0 (qcom,led-mask = <9>),
// which the camera HAL itself toggles through "switch0_trigger".
#define FLASH_LED_PATH                                                   \
    "/sys/devices/platform/soc/c42d000.qcom,spmi/spmi-0/0-02/"           \
    "c42d000.qcom,spmi:qcom,pm8350c@2:qcom,flash_led@ee00/leds/"

static const std::string kTorchNodes[] = {
        FLASH_LED_PATH "led:torch_0/brightness",
        FLASH_LED_PATH "led:torch_3/brightness",
};
static const std::string kSwitchNode = FLASH_LED_PATH "led:switch_0/brightness";

// Levels are the torch current per channel in mA. The HAL uses 65 mA, keep
// that as the default; the driver allows up to 500 mA per channel, cap well
// below for a torch that can stay on.
static constexpr int32_t kDefaultLevel = 65;
static constexpr int32_t kMaxLevel = 250;

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

bool supportsTorchStrengthControlExt() {
    return true;
}

int32_t getTorchDefaultStrengthLevelExt() {
    return kDefaultLevel;
}

int32_t getTorchMaxStrengthLevelExt() {
    return kMaxLevel;
}

int32_t getTorchStrengthLevelExt() {
    return std::clamp(get(kTorchNodes[0], kDefaultLevel), 1, kMaxLevel);
}

void setTorchStrengthLevelExt(int32_t torchStrength, bool enabled) {
    int32_t level = std::clamp(torchStrength, 1, kMaxLevel);

    for (const auto& node : kTorchNodes) {
        set(node, level);
    }

    // Writing a non zero brightness to the switch applies the new current.
    // Never write 0 there: the LED core would detach switch0_trigger and the
    // camera HAL could no longer turn the torch (or the flash) off. Turning
    // off is left to the HAL, which cameraservice calls first.
    if (enabled) {
        set(kSwitchNode, 255);
    }
}
