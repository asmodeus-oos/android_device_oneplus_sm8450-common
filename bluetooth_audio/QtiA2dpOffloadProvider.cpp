// Signed-off-by: Bubun Das <bubundas17@gmail.com>
/*
 * Copyright (C) 2025 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "QtiA2dpOffloadProvider"

#include "QtiA2dpOffloadProvider.h"

#include <android-base/logging.h>
#include <hidl/HidlSupport.h>

#include <aidl/android/hardware/bluetooth/audio/A2dpStatus.h>
#include <aidl/android/hardware/bluetooth/audio/AacConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/AptxConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/CodecConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/LdacConfiguration.h>
#include <aidl/android/hardware/bluetooth/audio/SbcConfiguration.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

using namespace ::android::hardware;

namespace {

// HIDL IBluetoothAudioPort bridge: forwards the QTI service's stream
// control callbacks back to the Bluetooth stack's AIDL port.
class HidlPortBridge : public V2_1::IBluetoothAudioPort {
  public:
    explicit HidlPortBridge(
            const std::shared_ptr<::aidl::android::hardware::bluetooth::audio::
                                            IBluetoothAudioPort>& aidl_port)
        : aidl_port_(aidl_port) {}

    Return<void> startStream() override {
        if (aidl_port_) {
            aidl_port_->startStream(/*is_low_latency=*/false);
        }
        return Void();
    }

    Return<void> suspendStream() override {
        if (aidl_port_) {
            aidl_port_->suspendStream();
        }
        return Void();
    }

    Return<void> stopStream() override {
        if (aidl_port_) {
            aidl_port_->stopStream();
        }
        return Void();
    }

    Return<void> getPresentationPosition(getPresentationPosition_cb _hidl_cb) override {
        _hidl_cb(V2_0::Status::SUCCESS, 0, 0, {0, 0});
        return Void();
    }

    Return<void> updateAptxMode(uint16_t /*aptx_mode*/) override {
        return Void();
    }

  private:
    std::shared_ptr<::aidl::android::hardware::bluetooth::audio::IBluetoothAudioPort> aidl_port_;
};

V2_0::SampleRate sampleRateToHidl(int32_t hz) {
    switch (hz) {
        case 16000: return V2_0::SampleRate::RATE_16000;
        case 24000: return V2_0::SampleRate::RATE_24000;
        case 44100: return V2_0::SampleRate::RATE_44100;
        case 48000: return V2_0::SampleRate::RATE_48000;
        case 88200: return V2_0::SampleRate::RATE_88200;
        case 96000: return V2_0::SampleRate::RATE_96000;
        case 176400: return V2_0::SampleRate::RATE_176400;
        case 192000: return V2_0::SampleRate::RATE_192000;
        default: return V2_0::SampleRate::RATE_UNKNOWN;
    }
}

V2_0::BitsPerSample bitsToHidl(int32_t bits) {
    switch (bits) {
        case 24: return V2_0::BitsPerSample::BITS_24;
        case 32: return V2_0::BitsPerSample::BITS_32;
        case 16:
        default: return V2_0::BitsPerSample::BITS_16;
    }
}

V2_0::ChannelMode channelModeToHidl(ChannelMode mode) {
    switch (mode) {
        case ChannelMode::MONO: return V2_0::ChannelMode::MONO;
        case ChannelMode::STEREO:
        case ChannelMode::DUALMONO:
        default: return V2_0::ChannelMode::STEREO;
    }
}

V2_0::SbcBlockLength sbcBlockToHidl(uint8_t blocks) {
    switch (blocks) {
        case 8: return V2_0::SbcBlockLength::BLOCKS_8;
        case 12: return V2_0::SbcBlockLength::BLOCKS_12;
        case 16: return V2_0::SbcBlockLength::BLOCKS_16;
        case 4:
        default: return V2_0::SbcBlockLength::BLOCKS_4;
    }
}

V2_0::SbcNumSubbands sbcSubbandsToHidl(uint8_t subbands) {
    return subbands == 8 ? V2_0::SbcNumSubbands::SUBBAND_8
                         : V2_0::SbcNumSubbands::SUBBAND_4;
}

V2_0::SbcAllocMethod sbcAllocToHidl(SbcAllocMethod method) {
    return method == SbcAllocMethod::ALLOC_MD_L
            ? V2_0::SbcAllocMethod::ALLOC_MD_L
            : V2_0::SbcAllocMethod::ALLOC_MD_S;
}

V2_0::SbcChannelMode sbcChannelToHidl(SbcChannelMode mode) {
    switch (mode) {
        case SbcChannelMode::MONO: return V2_0::SbcChannelMode::MONO;
        case SbcChannelMode::DUAL: return V2_0::SbcChannelMode::DUAL;
        case SbcChannelMode::STEREO: return V2_0::SbcChannelMode::STEREO;
        case SbcChannelMode::JOINT_STEREO:
        default: return V2_0::SbcChannelMode::JOINT_STEREO;
    }
}

V2_0::AacObjectType aacObjectToHidl(AacObjectType object_type) {
    switch (object_type) {
        case AacObjectType::MPEG2_LC: return V2_0::AacObjectType::MPEG2_LC;
        case AacObjectType::MPEG4_LTP: return V2_0::AacObjectType::MPEG4_LTP;
        case AacObjectType::MPEG4_SCALABLE: return V2_0::AacObjectType::MPEG4_SCALABLE;
        case AacObjectType::MPEG4_LC:
        default: return V2_0::AacObjectType::MPEG4_LC;
    }
}

V2_0::LdacChannelMode ldacChannelToHidl(LdacChannelMode mode) {
    switch (mode) {
        case LdacChannelMode::STEREO: return V2_0::LdacChannelMode::STEREO;
        case LdacChannelMode::DUAL: return V2_0::LdacChannelMode::DUAL;
        case LdacChannelMode::MONO: return V2_0::LdacChannelMode::MONO;
        case LdacChannelMode::UNKNOWN:
        default: return V2_0::LdacChannelMode::UNKNOWN;
    }
}

V2_0::LdacQualityIndex ldacQualityToHidl(LdacQualityIndex index) {
    switch (index) {
        case LdacQualityIndex::HIGH: return V2_0::LdacQualityIndex::QUALITY_HIGH;
        case LdacQualityIndex::MID: return V2_0::LdacQualityIndex::QUALITY_MID;
        case LdacQualityIndex::LOW: return V2_0::LdacQualityIndex::QUALITY_LOW;
        case LdacQualityIndex::ABR:
        default: return V2_0::LdacQualityIndex::QUALITY_ABR;
    }
}

}  // namespace

QtiA2dpOffloadProvider::QtiA2dpOffloadProvider(
        const sp<V2_1::IBluetoothAudioProvidersFactory>& hidl_factory, bool encoding)
    : hidl_factory_(hidl_factory), encoding_(encoding) {}

QtiA2dpOffloadProvider::~QtiA2dpOffloadProvider() {
    if (session_active_ && hidl_provider_) {
        hidl_provider_->endSession();
    }
}

V2_0::SessionType QtiA2dpOffloadProvider::toHidlSessionType(bool encoding) {
    (void)encoding;  // HIDL has a single A2DP offload type
    return V2_0::SessionType::A2DP_HARDWARE_OFFLOAD_DATAPATH;
}

bool QtiA2dpOffloadProvider::openHidlProvider() {
    if (hidl_provider_) return true;
    if (!hidl_factory_) return false;

    hidl_provider_ = nullptr;
    auto ret = hidl_factory_->openProvider_2_1(
            toHidlSessionType(encoding_),
            [&](V2_0::Status status, const sp<V2_1::IBluetoothAudioProvider>& provider) {
                if (status == V2_0::Status::SUCCESS) {
                    hidl_provider_ = provider;
                }
            });
    if (!ret.isOk()) {
        LOG(ERROR) << "openProvider_2_1 transaction failed: " << ret.description();
        return false;
    }
    if (!hidl_provider_) {
        LOG(ERROR) << "QTI rejected A2DP offload provider";
        return false;
    }
    LOG(INFO) << "QTI A2DP offload provider opened";
    return true;
}

bool QtiA2dpOffloadProvider::isValid(const SessionType& session_type) {
    auto expected = encoding_
            ? SessionType::A2DP_HARDWARE_OFFLOAD_ENCODING_DATAPATH
            : SessionType::A2DP_HARDWARE_OFFLOAD_DECODING_DATAPATH;
    return session_type == expected && openHidlProvider();
}

V2_1::AudioConfiguration QtiA2dpOffloadProvider::toHidlAudioConfig(
        const AudioConfiguration& audio_config) {
    V2_1::AudioConfiguration result;
    V2_1::CodecConfiguration hidl_codec;

    if (audio_config.getTag() == AudioConfiguration::a2dpConfig) {
        const auto& cc = audio_config.get<AudioConfiguration::a2dpConfig>();
        hidl_codec.encodedAudioBitrate = cc.encodedAudioBitrate;
        hidl_codec.peerMtu = static_cast<uint16_t>(cc.peerMtu * 2);  // words -> octets
        hidl_codec.isScmstEnabled = cc.isScmstEnabled;

        switch (cc.codecType) {
            case CodecType::SBC:
                hidl_codec.codecType = V2_1::CodecType::SBC;
                if (cc.config.getTag() ==
                        CodecConfiguration::CodecSpecific::sbcConfig) {
                    const auto& sbc = cc.config.get<
                            CodecConfiguration::CodecSpecific::sbcConfig>();
                    hidl_codec.config.sbcConfig = V2_0::SbcParameters{
                            sampleRateToHidl(sbc.sampleRateHz),
                            sbcChannelToHidl(sbc.channelMode),
                            sbcBlockToHidl(sbc.blockLength),
                            sbcSubbandsToHidl(sbc.numSubbands),
                            sbcAllocToHidl(sbc.allocMethod),
                            bitsToHidl(sbc.bitsPerSample),
                            static_cast<uint8_t>(sbc.minBitpool),
                            static_cast<uint8_t>(sbc.maxBitpool)};
                }
                break;
            case CodecType::AAC:
                hidl_codec.codecType = V2_1::CodecType::AAC;
                if (cc.config.getTag() ==
                        CodecConfiguration::CodecSpecific::aacConfig) {
                    const auto& aac = cc.config.get<
                            CodecConfiguration::CodecSpecific::aacConfig>();
                    hidl_codec.config.aacConfig = V2_0::AacParameters{
                            aacObjectToHidl(aac.objectType),
                            sampleRateToHidl(aac.sampleRateHz),
                            channelModeToHidl(aac.channelMode),
                            aac.variableBitRateEnabled
                                    ? V2_0::AacVariableBitRate::ENABLED
                                    : V2_0::AacVariableBitRate::DISABLED,
                            bitsToHidl(aac.bitsPerSample)};
                }
                break;
            case CodecType::APTX:
            case CodecType::APTX_HD:
                hidl_codec.codecType = cc.codecType == CodecType::APTX
                        ? V2_1::CodecType::APTX
                        : V2_1::CodecType::APTX_HD;
                if (cc.config.getTag() ==
                        CodecConfiguration::CodecSpecific::aptxConfig) {
                    const auto& aptx = cc.config.get<
                            CodecConfiguration::CodecSpecific::aptxConfig>();
                    hidl_codec.config.aptxConfig = V2_0::AptxParameters{
                            sampleRateToHidl(aptx.sampleRateHz),
                            channelModeToHidl(aptx.channelMode),
                            bitsToHidl(aptx.bitsPerSample)};
                }
                break;
            case CodecType::LDAC:
                hidl_codec.codecType = V2_1::CodecType::LDAC;
                if (cc.config.getTag() ==
                        CodecConfiguration::CodecSpecific::ldacConfig) {
                    const auto& ldac = cc.config.get<
                            CodecConfiguration::CodecSpecific::ldacConfig>();
                    hidl_codec.config.ldacConfig = V2_0::LdacParameters{
                            sampleRateToHidl(ldac.sampleRateHz),
                            ldacChannelToHidl(ldac.channelMode),
                            ldacQualityToHidl(ldac.qualityIndex),
                            bitsToHidl(ldac.bitsPerSample)};
                }
                break;
            default:
                LOG(WARNING) << "Unsupported offload codec: "
                             << static_cast<int>(cc.codecType);
                hidl_codec.codecType = V2_1::CodecType::UNKNOWN;
                break;
        }
    } else {
        LOG(WARNING) << "Unexpected audio configuration tag: "
                     << static_cast<int>(audio_config.getTag());
    }

    result.codecConfig = hidl_codec;
    return result;
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::startSession(
        const std::shared_ptr<IBluetoothAudioPort>& host_if,
        const AudioConfiguration& audio_config,
        const std::vector<LatencyMode>& /*latency_modes*/,
        DataMQDesc* _aidl_return) {
    LOG(INFO) << __func__;

    if (!openHidlProvider()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    sp<HidlPortBridge> port_bridge = new HidlPortBridge(host_if);
    auto hidl_config = toHidlAudioConfig(audio_config);

    V2_0::Status hidl_status = V2_0::Status::FAILURE;
    auto ret = hidl_provider_->startSession_2_1(
            port_bridge, hidl_config,
            [&](V2_0::Status status, const MQDescriptorSync<uint8_t>&) {
                hidl_status = status;
            });
    if (!ret.isOk() || hidl_status != V2_0::Status::SUCCESS) {
        LOG(ERROR) << "QTI startSession_2_1 failed: " << ret.description()
                   << " status=" << static_cast<int>(hidl_status);
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    session_active_ = true;
    // Hardware offload: no PCM data path through the HAL.
    *_aidl_return = DataMQDesc();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::endSession() {
    LOG(INFO) << __func__;
    if (session_active_ && hidl_provider_) {
        hidl_provider_->endSession();
    }
    session_active_ = false;
    return ndk::ScopedAStatus::ok();
}

V2_0::Status QtiA2dpOffloadProvider::toHidlStatus(BluetoothAudioStatus status) {
    return status == BluetoothAudioStatus::SUCCESS
            ? V2_0::Status::SUCCESS
            : V2_0::Status::FAILURE;
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::streamStarted(BluetoothAudioStatus status) {
    LOG(INFO) << __func__ << " status=" << static_cast<int>(status);
    if (hidl_provider_) {
        hidl_provider_->streamStarted(toHidlStatus(status));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::streamSuspended(BluetoothAudioStatus status) {
    LOG(INFO) << __func__ << " status=" << static_cast<int>(status);
    if (hidl_provider_) {
        hidl_provider_->streamSuspended(toHidlStatus(status));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::updateAudioConfiguration(
        const AudioConfiguration& /*audio_config*/) {
    LOG(INFO) << __func__;
    // The QTI session is configured at startSession time; HIDL has no
    // update callback for A2DP.
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::setLowLatencyModeAllowed(bool /*allowed*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::parseA2dpConfiguration(
        const CodecId& /*codec_id*/,
        const std::vector<uint8_t>& /*configuration*/,
        CodecParameters* /*codec_parameters*/,
        A2dpStatus* _aidl_return) {
    *_aidl_return = A2dpStatus::OK;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getA2dpConfiguration(
        const std::vector<A2dpRemoteCapabilities>& /*remote_a2dp_capabilities*/,
        const A2dpConfigurationHint& /*hint*/,
        std::optional<A2dpConfiguration>* _aidl_return) {
    *_aidl_return = std::nullopt;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::setCodecPriority(
        const CodecId& /*codec_id*/, int /*priority*/) {
    return ndk::ScopedAStatus::ok();
}

// --- Unsupported for A2DP sessions ---

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioAseConfiguration(
        const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&,
        const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&,
        const std::vector<P::LeAudioConfigurationRequirement>&,
        std::vector<P::LeAudioAseConfigurationSetting>*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioAseQosConfiguration(
        const P::LeAudioAseQosConfigurationRequirement&,
        P::LeAudioAseQosConfigurationPair*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioAseDatapathConfiguration(
        const std::optional<P::StreamConfig>&,
        const std::optional<P::StreamConfig>&,
        P::LeAudioDataPathConfigurationPair*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::onSinkAseMetadataChanged(
        P::AseState, int32_t, int32_t,
        const std::optional<std::vector<std::optional<MetadataLtv>>>&) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::onSourceAseMetadataChanged(
        P::AseState, int32_t, int32_t,
        const std::optional<std::vector<std::optional<MetadataLtv>>>&) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioBroadcastConfiguration(
        const std::optional<std::vector<std::optional<P::LeAudioDeviceCapabilities>>>&,
        const P::LeAudioBroadcastConfigurationRequirement&,
        P::LeAudioBroadcastConfigurationSetting*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioBroadcastDatapathConfiguration(
        const AudioContext&,
        const std::vector<LeAudioBroadcastConfiguration::BroadcastStreamMap>&,
        P::LeAudioDataPathConfiguration*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus QtiA2dpOffloadProvider::getLeAudioAseCodecConfiguredParameters(
        const std::optional<std::vector<std::optional<LeAudioAseConfiguration>>>&,
        const std::optional<std::vector<std::optional<LeAudioAseConfiguration>>>&,
        std::optional<P::LeAudioAseCodecConfiguredResponse>*) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
