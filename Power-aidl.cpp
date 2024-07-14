/*
 * Copyright (C) 2017 The Android Open Source Project
 * Copyright (C) 2024 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.power-service-nvidia"

#define SATA_POWER_CONTROL_PATH "/sys/devices/tegra-sata.0/ata1/power/control"
#define HDD_STANDBY_TIMEOUT     60

#include <android/log.h>
#include <utils/Log.h>
#include "Power-aidl.h"
#include "tegra_sata_hal.h"

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace nvidia {

Power::Power() {
    pInfo = new powerhal_info();

    common_power_open(pInfo);

    pInfo->input_devs.push_back({-1, "touch\n"});
    pInfo->input_devs.push_back({-1, "raydium_ts\n"});

    pInfo->no_sclk_boost = true;

    common_power_init(pInfo);
}

void Power::setInteractive(bool interactive) {
    common_power_set_interactive(pInfo, interactive ? 1 : 0);

#if TARGET_TEGRA_VERSION == 210
    set_power_level_floor(interactive);
#endif

    if (!access(SATA_POWER_CONTROL_PATH, F_OK)) {
        /*
        * Turn-off Foster HDD at display off
        */
        ALOGI("HAL: Display is %s, set HDD to %s standby mode.", interactive?"on":"off", interactive?"disable":"enter");
        int error = 0;
        if (interactive) {
            error = hdd_disable_standby_timer();
            if (error)
                ALOGE("HAL: Failed to set standby timer, error: %d", error);
        }
        else {
            error = hdd_set_standby_timer(HDD_STANDBY_TIMEOUT);
            if (error)
                ALOGE("HAL: Failed to set standby timer, error: %d", error);
        }
    }
}

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    switch (type) {
        case Mode::LOW_POWER:
            common_power_hint(pInfo, NvPowerHint::LOW_POWER, &enabled);
            break;
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
            common_power_hint(pInfo, NvPowerHint::AUDIO_LOW_LATENCY, &enabled);
            break;
        case Mode::LAUNCH:
            common_power_hint(pInfo, NvPowerHint::LAUNCH, &enabled);
            break;
        case Mode::INTERACTIVE:
            setInteractive(enabled);
            break;
        default:
            ALOGI("Mode %d Not Supported", static_cast<int32_t>(type));
            break;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    switch (type) {
        case Mode::LAUNCH:
        case Mode::INTERACTIVE:
        case Mode::LOW_POWER:
        case Mode::AUDIO_STREAMING_LOW_LATENCY:
            *_aidl_return = true;
            break;
        default:
            *_aidl_return = false;
            break;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost /* type */, int32_t /* durationMs */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::isBoostSupported(Boost /* type */, bool* _aidl_return) {
    *_aidl_return = false;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::createHintSession(int32_t /* tgid */, int32_t /* uid */,
                                            const std::vector<int32_t>& /* threadIds */,
                                            int64_t /* durationNanos */,
                                            std::shared_ptr<IPowerHintSession>* /* _aidl_return */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Power::getHintSessionPreferredRate(int64_t* /* outNanoseconds */) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace nvidia
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
