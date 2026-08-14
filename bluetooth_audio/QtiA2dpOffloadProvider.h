// Signed-off-by: Bubun Das <bubundas17@gmail.com>
/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 *
 * AIDL <-> HIDL bridge for A2DP hardware offload sessions.
 * Forwards the Bluetooth stack's AIDL calls to the QTI
 * vendor.qti.hardware.bluetooth_audio@2.1 HIDL service.
 */

#pragma once

#include <aidl/android/hardware/bluetooth/audio/BnBluetoothAudioProvider.h>
#include <vendor/qti/hardware/bluetooth_audio/2.1/IBluetoothAudioProvider.h>
#include <vendor/qti/hardware/bluetooth_audio/2.1/IBluetoothAudioProvidersFactory.h>
#include <vendor/qti/hardware/bluetooth_audio/2.1/types.h>

#include <utils/StrongPointer.h>

#include "BluetoothAudioProvider.h"  // DataMQDesc, MQDescriptor types

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using ::android::sp;
namespace V2_0 = ::vendor::qti::hardware::bluetooth_audio::V2_0;
namespace V2_1 = ::vendor::qti::hardware::bluetooth_audio::V2_1;
using P = ::aidl::android::hardware::bluetooth::audio::IBluetoothAudioProvider;

class QtiA2dpOffloadProvider : public BnBluetoothAudioProvider {
  public:
    QtiA2dpOffloadProvider(const sp<V2_1::IBluetoothAudioProvidersFactory>& hidl_factory,
                           bool encoding);
    ~QtiA2dpOffloadProvider() override;

    bool isValid(const SessionType& session_type);

    // A2DP session control
    ndk::ScopedAStatus startSession(
            const std::shared_ptr<IBluetoothAudioPort>& host_if,
            const AudioConfiguration& audio_config,
            const std::vector<LatencyMode>& latency_modes,
            DataMQDesc* _aidl_return) override;
    ndk::ScopedAStatus endSession() override;
    ndk::ScopedAStatus streamStarted(BluetoothAudioStatus status) override;
    ndk::ScopedAStatus streamSuspended(BluetoothAudioStatus status) override;
    ndk::ScopedAStatus updateAudioConfiguration(
            const AudioConfiguration& audio_config) override;
    ndk::ScopedAStatus setLowLatencyModeAllowed(bool allowed) override;

    // A2DP codec helpers
    ndk::ScopedAStatus parseA2dpConfiguration(
            const CodecId& codec_id,
            const std::vector<uint8_t>& configuration,
            CodecParameters* codec_parameters,
            A2dpStatus* _aidl_return) override;
    ndk::ScopedAStatus getA2dpConfiguration(
            const std::vector<A2dpRemoteCapabilities>& remote_a2dp_capabilities,
            const A2dpConfigurationHint& hint,
            std::optional<A2dpConfiguration>* _aidl_return) override;
    ndk::ScopedAStatus setCodecPriority(const CodecId& codec_id, int priority) override;

    // Unused for A2DP: unsupported stubs
    ndk::ScopedAStatus getLeAudioAseConfiguration(
            const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&
                    in_remote_sink_audio_capabilities,
            const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&
                    in_remote_source_audio_capabilities,
            const std::vector<P::LeAudioConfigurationRequirement>& in_requirements,
            std::vector<P::LeAudioAseConfigurationSetting>* _aidl_return) override;
    ndk::ScopedAStatus getLeAudioAseQosConfiguration(
            const P::LeAudioAseQosConfigurationRequirement& in_qos_requirement,
            P::LeAudioAseQosConfigurationPair* _aidl_return) override;
    ndk::ScopedAStatus getLeAudioAseDatapathConfiguration(
            const std::optional<P::StreamConfig>& in_sink_config,
            const std::optional<P::StreamConfig>& in_source_config,
            P::LeAudioDataPathConfigurationPair* _aidl_return) override;
    ndk::ScopedAStatus onSinkAseMetadataChanged(
            P::AseState in_state, int32_t cig_id, int32_t cis_id,
            const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) override;
    ndk::ScopedAStatus onSourceAseMetadataChanged(
            P::AseState in_state, int32_t cig_id, int32_t cis_id,
            const std::optional<std::vector<std::optional<MetadataLtv>>>& in_metadata) override;
    ndk::ScopedAStatus getLeAudioBroadcastConfiguration(
            const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&
                    in_remote_sink_audio_capabilities,
            const P::LeAudioBroadcastConfigurationRequirement& in_requirement,
            P::LeAudioBroadcastConfigurationSetting* _aidl_return) override;
    ndk::ScopedAStatus getLeAudioBroadcastDatapathConfiguration(
            const AudioContext& in_context,
            const std::vector<LeAudioBroadcastConfiguration::BroadcastStreamMap>& in_stream_map,
            P::LeAudioDataPathConfiguration* _aidl_return) override;
    ndk::ScopedAStatus getLeAudioAseCodecConfiguredParameters(
            const std::optional<std::vector<std::optional<LeAudioAseConfiguration>>>&
                    in_sink_ase_configuration,
            const std::optional<std::vector<std::optional<LeAudioAseConfiguration>>>&
                    in_source_ase_configuration,
            std::optional<P::LeAudioAseCodecConfiguredResponse>* _aidl_return) override;

  private:
    bool openHidlProvider();
    V2_1::AudioConfiguration toHidlAudioConfig(const AudioConfiguration& audio_config);
    static V2_0::SessionType toHidlSessionType(bool encoding);
    static V2_0::Status toHidlStatus(BluetoothAudioStatus status);

    sp<V2_1::IBluetoothAudioProvidersFactory> hidl_factory_;
    sp<V2_1::IBluetoothAudioProvider> hidl_provider_;
    bool encoding_;
    bool session_active_ = false;
};

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
