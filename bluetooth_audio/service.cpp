// Signed-off-by: Bubun Das <bubundas17@gmail.com>
/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "BtAudioAIDLService"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Log.h>

#include "QtiBluetoothAudioProviderFactory.h"

using aidl::android::hardware::bluetooth::audio::QtiBluetoothAudioProviderFactory;

extern "C" __attribute__((visibility("default"))) binder_status_t
createIBluetoothAudioProviderFactory() {
    auto factory = ndk::SharedRefBase::make<QtiBluetoothAudioProviderFactory>();
    const std::string instance_name =
            std::string() + QtiBluetoothAudioProviderFactory::descriptor + "/default";
    binder_status_t aidl_status =
            AServiceManager_addService(factory->asBinder().get(), instance_name.c_str());
    ALOGW_IF(aidl_status != STATUS_OK, "Could not register %s, status=%d",
             instance_name.c_str(), aidl_status);
    return aidl_status;
}
