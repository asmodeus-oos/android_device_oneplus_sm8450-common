// Signed-off-by: Bubun Das <bubundas17@gmail.com>
/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * BluetoothAudioProviderFactory with QTI A2DP offload support.
 * For A2DP hardware offload sessions, creates a bridge to the QTI HIDL
 * vendor.qti.hardware.bluetooth_audio@2.1 service.
 * All other session types use standard AOSP providers.
 */

#define LOG_TAG "QtiBtAudioProviderFactory"

#include "QtiBluetoothAudioProviderFactory.h"

#include <BluetoothAudioCodecs.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <com_android_btaudio_hal_flags.h>
#include <hidl/HidlTransportSupport.h>

// AOSP default providers (included from hardware/interfaces/bluetooth/audio/aidl/default/)
#include <A2dpOffloadAudioProvider.h>
#include <A2dpSoftwareAudioProvider.h>
#include <HearingAidAudioProvider.h>
#include <HfpSoftwareAudioProvider.h>
#include <HfpOffloadAudioProvider.h>
#include <LeAudioSoftwareAudioProvider.h>
#include <LeAudioOffloadAudioProvider.h>
#include <BluetoothAudioProvider.h>

// Our QTI offload bridge
#include "QtiA2dpOffloadProvider.h"

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using ::android::sp;
using ::vendor::qti::hardware::bluetooth_audio::V2_1::IBluetoothAudioProvidersFactory;

QtiBluetoothAudioProviderFactory::QtiBluetoothAudioProviderFactory() {}

QtiBluetoothAudioProviderFactory::~QtiBluetoothAudioProviderFactory() = default;

static sp<IBluetoothAudioProvidersFactory> getHidlFactory() {
    auto factory = IBluetoothAudioProvidersFactory::getService();
    if (!factory) {
        LOG(WARNING) << "vendor.qti.hardware.bluetooth_audio@2.1 not available, "
                        "A2DP offload disabled";
    }
    return factory;
}

ndk::ScopedAStatus QtiBluetoothAudioProviderFactory::openProvider(
        const SessionType session_type,
        std::shared_ptr<IBluetoothAudioProvider>* _aidl_return) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type);
    std::shared_ptr<BluetoothAudioProvider> provider = nullptr;

    switch (session_type) {
        case SessionType::A2DP_SOFTWARE_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<A2dpSoftwareEncodingAudioProvider>();
            break;
        case SessionType::A2DP_SOFTWARE_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<A2dpSoftwareDecodingAudioProvider>();
            break;
        case SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH: {
            auto hidl = getHidlFactory();
            if (hidl) {
                auto offload = ndk::SharedRefBase::make<QtiA2dpOffloadProvider>(hidl, true);
                if (offload->isValid(session_type)) {
                    *_aidl_return = offload;
                    return ndk::ScopedAStatus::ok();
                }
            }
            LOG(WARNING) << "QTI A2DP offload unavailable, falling back to AOSP offload";
            provider = ndk::SharedRefBase::make<A2dpOffloadEncodingAudioProvider>(
                    a2dp_offload_codec_factory_);
            break;
        }
        case SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH: {
            auto hidl = getHidlFactory();
            if (hidl) {
                auto offload = ndk::SharedRefBase::make<QtiA2dpOffloadProvider>(hidl, false);
                if (offload->isValid(session_type)) {
                    *_aidl_return = offload;
                    return ndk::ScopedAStatus::ok();
                }
            }
            LOG(WARNING) << "QTI A2DP offload unavailable, falling back to AOSP offload";
            provider = ndk::SharedRefBase::make<A2dpOffloadDecodingAudioProvider>(
                    a2dp_offload_codec_factory_);
            break;
        }
        case SessionType::HEARING_AID_SOFTWARE_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<HearingAidAudioProvider>();
            break;
        case SessionType::LE_AUDIO_SOFTWARE_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioSoftwareOutputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_SOFTWARE_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioSoftwareInputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadOutputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadInputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_BROADCAST_SOFTWARE_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioSoftwareBroadcastOutputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadBroadcastOutputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_BROADCAST_SOFTWARE_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioSoftwareBroadcastInputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadBroadcastInputAudioProvider>();
            break;
        case SessionType::HFP_SOFTWARE_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<HfpSoftwareOutputAudioProvider>();
            break;
        case SessionType::HFP_SOFTWARE_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<HfpSoftwareInputAudioProvider>();
            break;
        case SessionType::HFP_HARDWARE_OFFLOAD_DATAPATH:
            provider = ndk::SharedRefBase::make<HfpOffloadAudioProvider>();
            break;
        case SessionType::LE_AUDIO_PERIPHERAL_OFFLOAD_ENCODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadPeripheralOutputAudioProvider>();
            break;
        case SessionType::LE_AUDIO_PERIPHERAL_OFFLOAD_DECODING_DATAPATH:
            provider = ndk::SharedRefBase::make<LeAudioOffloadPeripheralInputAudioProvider>();
            break;
        default:
            provider = nullptr;
            break;
    }

    if (provider == nullptr || !provider->isValid(session_type)) {
        LOG(ERROR) << __func__ << " - SessionType=" << toString(session_type) << " not available";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    *_aidl_return = provider;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiBluetoothAudioProviderFactory::getProviderCapabilities(
        const SessionType session_type, std::vector<AudioCapabilities>* _aidl_return) {
    if (session_type == SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
        session_type == SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH) {
        if (::android::base::GetBoolProperty(kEnableA2dpCodecExtensibility, false)) {
            return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
        }
        auto codec_capabilities =
                BluetoothAudioCodecs::GetA2dpOffloadCodecCapabilities(session_type);
        _aidl_return->resize(codec_capabilities.size());
        for (int i = 0; i < codec_capabilities.size(); i++) {
            _aidl_return->at(i).set<AudioCapabilities::a2dpCapabilities>(codec_capabilities[i]);
        }
    } else if (session_type == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_ENCODING_DATAPATH ||
               session_type == SessionType::LE_AUDIO_HARDWARE_OFFLOAD_DECODING_DATAPATH ||
               session_type == SessionType::LE_AUDIO_BROADCAST_HARDWARE_OFFLOAD_ENCODING_DATAPATH) {
        auto db_codec_capabilities =
                BluetoothAudioCodecs::GetLeAudioOffloadCodecCapabilities(session_type);
        if (db_codec_capabilities.size()) {
            _aidl_return->resize(db_codec_capabilities.size());
            for (int i = 0; i < db_codec_capabilities.size(); ++i) {
                _aidl_return->at(i).set<AudioCapabilities::leAudioCapabilities>(
                        db_codec_capabilities[i]);
            }
        }
    } else if (session_type != SessionType::UNKNOWN) {
        auto pcm_capabilities = BluetoothAudioCodecs::GetSoftwarePcmCapabilities();
        _aidl_return->resize(pcm_capabilities.size());
        for (int i = 0; i < pcm_capabilities.size(); i++) {
            _aidl_return->at(i).set<AudioCapabilities::pcmCapabilities>(pcm_capabilities[i]);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiBluetoothAudioProviderFactory::getProviderInfo(
        SessionType session_type, std::optional<ProviderInfo>* _aidl_return) {
    *_aidl_return = std::nullopt;
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
