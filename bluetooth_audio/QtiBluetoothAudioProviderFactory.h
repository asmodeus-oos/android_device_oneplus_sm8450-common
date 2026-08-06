// Signed-off-by: Bubun Das <bubundas17@gmail.com>
/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/bluetooth/audio/BnBluetoothAudioProviderFactory.h>
#include <a2dp/A2dpOffloadCodecFactory.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

class QtiBluetoothAudioProviderFactory
    : public BnBluetoothAudioProviderFactory {
  public:
    QtiBluetoothAudioProviderFactory();
    ~QtiBluetoothAudioProviderFactory() override;

    ndk::ScopedAStatus openProvider(
            const SessionType session_type,
            std::shared_ptr<IBluetoothAudioProvider>* _aidl_return) override;

    ndk::ScopedAStatus getProviderCapabilities(
            const SessionType session_type,
            std::vector<AudioCapabilities>* _aidl_return) override;

    ndk::ScopedAStatus getProviderInfo(
            SessionType session_type,
            std::optional<ProviderInfo>* _aidl_return) override;

  private:
    A2dpOffloadCodecFactory a2dp_offload_codec_factory_;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
