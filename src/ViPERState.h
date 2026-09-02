#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>
#include "../ViPER4Mac/ViPERDSP/viper/ViPER.h"

#define VIPER_PROP(type, name, setter, default_val) \
    Q_PROPERTY(type name READ name WRITE setter NOTIFY name##Changed) \
    public: \
        type name() const { return m_##name; } \
        void setter(type value) { \
            if (m_##name != value) { \
                m_##name = value; \
                emit name##Changed(); \
                onPropertyChanged(#name); \
            } \
        } \
    Q_SIGNALS: \
        void name##Changed(); \
    private: \
        type m_##name = default_val;

class ViPERState : public QObject {
    Q_OBJECT

    // Driver & Engine Status
    VIPER_PROP(bool, driverInstalled, setDriverInstalled, true)
    VIPER_PROP(QString, driverStatusText, setDriverStatusText, "Active")
    VIPER_PROP(QString, audioServerName, setAudioServerName, "PipeWire 1.6.8")
    VIPER_PROP(QString, outputDeviceName, setOutputDeviceName, "Default Audio Sink")
    VIPER_PROP(int, currentSampleRate, setCurrentSampleRate, 48000)
    VIPER_PROP(QString, dspVersion, setDspVersion, "2.5.0.5")
    VIPER_PROP(bool, isProcessing, setIsProcessing, true)
    VIPER_PROP(bool, launchAtLogin, setLaunchAtLogin, false)

    // Master & Mode
    VIPER_PROP(bool, isEnabled, setIsEnabled, true)
    VIPER_PROP(int, fxType, setFxType, 0) // 0 = Headphone, 1 = Speaker

    // Output
    VIPER_PROP(int, outputVolume, setOutputVolume, 100)
    VIPER_PROP(int, channelPan, setChannelPan, 0)
    VIPER_PROP(int, limiter, setLimiter, 100)

    // FIR Equalizer
    VIPER_PROP(bool, equalizerEnabled, setEqualizerEnabled, false)
    VIPER_PROP(int, equalizerBandCount, setEqualizerBandCountProperty, 10)
    VIPER_PROP(QVariantList, equalizerBands, setEqualizerBandsProperty, QVariantList())
    VIPER_PROP(QString, equalizerPresetName, setEqualizerPresetName, "Flat")
    VIPER_PROP(QStringList, presetList, setPresetList, QStringList())
    Q_PROPERTY(QStringList equalizerBandLabels READ equalizerBandLabels NOTIFY equalizerBandCountChanged)
    Q_PROPERTY(QStringList equalizerPresetNames READ presetList NOTIFY presetListChanged)
    QStringList equalizerBandLabels() const;

    // ViPER Bass
    VIPER_PROP(bool, viperBassEnabled, setViperBassEnabled, false)
    VIPER_PROP(int, viperBassMode, setViperBassMode, 0)
    VIPER_PROP(int, viperBassFrequency, setViperBassFrequency, 60)
    VIPER_PROP(int, viperBassGain, setViperBassGain, 120)
    VIPER_PROP(bool, viperBassAntiPop, setViperBassAntiPop, true)

    // ViPER Bass Mono
    VIPER_PROP(bool, viperBassMonoEnabled, setViperBassMonoEnabled, false)
    VIPER_PROP(int, viperBassMonoMode, setViperBassMonoMode, 0)
    VIPER_PROP(int, viperBassMonoFrequency, setViperBassMonoFrequency, 60)
    VIPER_PROP(int, viperBassMonoGain, setViperBassMonoGain, 120)
    VIPER_PROP(bool, viperBassMonoAntiPop, setViperBassMonoAntiPop, true)

    // ViPER Clarity
    VIPER_PROP(bool, viperClarityEnabled, setViperClarityEnabled, false)
    VIPER_PROP(int, viperClarityMode, setViperClarityMode, 0)
    VIPER_PROP(int, viperClarityGain, setViperClarityGain, 120)

    // Convolver
    VIPER_PROP(bool, convolutionEnabled, setConvolutionEnabled, false)
    VIPER_PROP(QString, convolutionKernelPath, setConvolutionKernelPath, "")
    VIPER_PROP(int, convolutionCrossChannel, setConvolutionCrossChannel, 0)
    VIPER_PROP(QStringList, kernelFiles, setKernelFiles, QStringList())

    // ViPER-DDC
    VIPER_PROP(bool, ddcEnabled, setDdcEnabled, false)
    VIPER_PROP(QString, ddcFilePath, setDdcFilePath, "")
    VIPER_PROP(QStringList, ddcFiles, setDdcFiles, QStringList())

    // Field Surround
    VIPER_PROP(bool, fieldSurroundEnabled, setFieldSurroundEnabled, false)
    VIPER_PROP(int, fieldSurroundWidening, setFieldSurroundWidening, 0)
    VIPER_PROP(int, fieldSurroundMidImage, setFieldSurroundMidImage, 5)
    VIPER_PROP(int, fieldSurroundDepth, setFieldSurroundDepth, 0)

    // Differential Surround
    VIPER_PROP(bool, diffSurroundEnabled, setDiffSurroundEnabled, false)
    VIPER_PROP(int, diffSurroundDelay, setDiffSurroundDelay, 5)
    VIPER_PROP(bool, diffSurroundReverse, setDiffSurroundReverse, false)
    VIPER_PROP(int, diffSurroundWetDryMix, setDiffSurroundWetDryMix, 100)
    VIPER_PROP(int, diffSurroundLpCutoff, setDiffSurroundLpCutoff, 0)

    // Headphone Surround+ (VHE)
    VIPER_PROP(bool, vheEnabled, setVheEnabled, false)
    VIPER_PROP(int, vheQuality, setVheQuality, 0)

    // Reverberation
    VIPER_PROP(bool, reverberationEnabled, setReverberationEnabled, false)
    VIPER_PROP(int, reverberationRoomSize, setReverberationRoomSize, 0)
    VIPER_PROP(int, reverberationRoomWidth, setReverberationRoomWidth, 0)
    VIPER_PROP(int, reverberationRoomDampening, setReverberationRoomDampening, 0)
    VIPER_PROP(int, reverberationWetSignal, setReverberationWetSignal, 0)
    VIPER_PROP(int, reverberationDrySignal, setReverberationDrySignal, 50)

    // Dynamic System
    VIPER_PROP(bool, dynamicSystemEnabled, setDynamicSystemEnabled, false)
    VIPER_PROP(int, dynamicSystemDevice, setDynamicSystemDevice, 0)
    VIPER_PROP(int, dynamicSystemStrength, setDynamicSystemStrength, 50)
    VIPER_PROP(int, dsXLow, setDsXLow, 100)
    VIPER_PROP(int, dsXHigh, setDsXHigh, 5600)
    VIPER_PROP(int, dsYLow, setDsYLow, 40)
    VIPER_PROP(int, dsYHigh, setDsYHigh, 80)
    VIPER_PROP(int, dsSideGainLow, setDsSideGainLow, 50)
    VIPER_PROP(int, dsSideGainHigh, setDsSideGainHigh, 50)

    // CURE
    VIPER_PROP(bool, cureEnabled, setCureEnabled, false)
    VIPER_PROP(int, cureCrossfeedStrength, setCureCrossfeedStrength, 0)

    // Tube & AnalogX
    VIPER_PROP(bool, tubeSimulatorEnabled, setTubeSimulatorEnabled, false)
    VIPER_PROP(bool, analogXEnabled, setAnalogXEnabled, false)
    VIPER_PROP(int, analogXMode, setAnalogXMode, 0)
    VIPER_PROP(bool, speakerCorrectionEnabled, setSpeakerCorrectionEnabled, false)

    // Spectrum Extension
    VIPER_PROP(bool, spectrumExtensionEnabled, setSpectrumExtensionEnabled, false)
    VIPER_PROP(int, spectrumExtensionBark, setSpectrumExtensionBark, 7600)
    VIPER_PROP(int, spectrumExtensionBarkReconstruct, setSpectrumExtensionBarkReconstruct, 0)

    // FET Compressor
    VIPER_PROP(bool, fetCompressorEnabled, setFetCompressorEnabled, false)
    VIPER_PROP(int, fetCompressorThreshold, setFetCompressorThreshold, -18)
    VIPER_PROP(int, fetCompressorRatio, setFetCompressorRatio, 100)
    VIPER_PROP(bool, fetCompressorAutoKnee, setFetCompressorAutoKnee, true)
    VIPER_PROP(int, fetCompressorKnee, setFetCompressorKnee, 0)
    VIPER_PROP(int, fetCompressorKneeMulti, setFetCompressorKneeMulti, 0)
    VIPER_PROP(bool, fetCompressorAutoGain, setFetCompressorAutoGain, true)
    VIPER_PROP(int, fetCompressorGain, setFetCompressorGain, 0)
    VIPER_PROP(bool, fetCompressorAutoAttack, setFetCompressorAutoAttack, true)
    VIPER_PROP(int, fetCompressorAttack, setFetCompressorAttack, 1)
    VIPER_PROP(int, fetCompressorMaxAttack, setFetCompressorMaxAttack, 44)
    VIPER_PROP(bool, fetCompressorAutoRelease, setFetCompressorAutoRelease, true)
    VIPER_PROP(int, fetCompressorRelease, setFetCompressorRelease, 100)
    VIPER_PROP(int, fetCompressorMaxRelease, setFetCompressorMaxRelease, 200)
    VIPER_PROP(int, fetCompressorCrest, setFetCompressorCrest, 100)
    VIPER_PROP(int, fetCompressorAdapt, setFetCompressorAdapt, 50)
    VIPER_PROP(bool, fetCompressorNoClip, setFetCompressorNoClip, true)

    // Multiband Compressor
    VIPER_PROP(bool, mbcEnabled, setMbcEnabled, false)
    VIPER_PROP(int, mbcSelectedBand, setMbcSelectedBand, 0)
    VIPER_PROP(QVariantList, mbcBandEnables, setMbcBandEnables, QVariantList())
    VIPER_PROP(QVariantList, mbcThresholds, setMbcThresholds, QVariantList())
    VIPER_PROP(QVariantList, mbcRatios, setMbcRatios, QVariantList())
    VIPER_PROP(QVariantList, mbcGains, setMbcGains, QVariantList())
    VIPER_PROP(QVariantList, mbcAutoGains, setMbcAutoGains, QVariantList())
    VIPER_PROP(QVariantList, mbcAttacks, setMbcAttacks, QVariantList())
    VIPER_PROP(QVariantList, mbcAutoAttacks, setMbcAutoAttacks, QVariantList())
    VIPER_PROP(QVariantList, mbcReleases, setMbcReleases, QVariantList())
    VIPER_PROP(QVariantList, mbcAutoReleases, setMbcAutoReleases, QVariantList())
    VIPER_PROP(QVariantList, mbcKnees, setMbcKnees, QVariantList())
    VIPER_PROP(QVariantList, mbcAutoKnees, setMbcAutoKnees, QVariantList())
    VIPER_PROP(QVariantList, mbcKneeMultis, setMbcKneeMultis, QVariantList())
    VIPER_PROP(QVariantList, mbcMaxAttacks, setMbcMaxAttacks, QVariantList())
    VIPER_PROP(QVariantList, mbcMaxReleases, setMbcMaxReleases, QVariantList())
    VIPER_PROP(QVariantList, mbcCrests, setMbcCrests, QVariantList())
    VIPER_PROP(QVariantList, mbcAdapts, setMbcAdapts, QVariantList())
    VIPER_PROP(QVariantList, mbcNoClips, setMbcNoClips, QVariantList())
    VIPER_PROP(QVariantList, mbcCrossovers, setMbcCrossovers, QVariantList())

    // Dynamic EQ
    VIPER_PROP(bool, dynEqEnabled, setDynEqEnabled, false)
    VIPER_PROP(int, dynEqBandCount, setDynEqBandCount, 3)
    VIPER_PROP(int, dynEqSelectedBand, setDynEqSelectedBand, 0)
    VIPER_PROP(QVariantList, dynEqFreqs, setDynEqFreqs, QVariantList())
    VIPER_PROP(QVariantList, dynEqQs, setDynEqQs, QVariantList())
    VIPER_PROP(QVariantList, dynEqGains, setDynEqGains, QVariantList())
    VIPER_PROP(QVariantList, dynEqThresholds, setDynEqThresholds, QVariantList())
    VIPER_PROP(QVariantList, dynEqAttacks, setDynEqAttacks, QVariantList())
    VIPER_PROP(QVariantList, dynEqReleases, setDynEqReleases, QVariantList())
    VIPER_PROP(QVariantList, dynEqFilterTypes, setDynEqFilterTypes, QVariantList())

    // Stereo Imager
    VIPER_PROP(bool, stereoImgEnabled, setStereoImgEnabled, false)
    VIPER_PROP(int, stereoImgLowWidth, setStereoImgLowWidth, 100)
    VIPER_PROP(int, stereoImgMidWidth, setStereoImgMidWidth, 100)
    VIPER_PROP(int, stereoImgHighWidth, setStereoImgHighWidth, 100)
    VIPER_PROP(int, stereoImgLowCrossover, setStereoImgLowCrossover, 200)
    VIPER_PROP(int, stereoImgHighCrossover, setStereoImgHighCrossover, 4000)

    // Playback Gain Control (AGC)
    VIPER_PROP(bool, playbackGainEnabled, setPlaybackGainEnabled, false)
    VIPER_PROP(int, playbackGainStrength, setPlaybackGainStrength, 50)
    VIPER_PROP(int, playbackGainMaxGain, setPlaybackGainMaxGain, 100)
    VIPER_PROP(int, playbackGainOutputThreshold, setPlaybackGainOutputThreshold, 100)

    // LUFS Targeting
    VIPER_PROP(bool, lufsEnabled, setLufsEnabled, false)
    VIPER_PROP(int, lufsTarget, setLufsTarget, 140)
    VIPER_PROP(int, lufsMaxGain, setLufsMaxGain, 60)
    VIPER_PROP(int, lufsSpeed, setLufsSpeed, 1)

    // Psychoacoustic Bass
    VIPER_PROP(bool, psychoBassEnabled, setPsychoBassEnabled, false)
    VIPER_PROP(int, psychoBassCutoff, setPsychoBassCutoff, 80)
    VIPER_PROP(int, psychoBassIntensity, setPsychoBassIntensity, 50)
    VIPER_PROP(int, psychoBassHarmonicOrder, setPsychoBassHarmonicOrder, 3)
    VIPER_PROP(int, psychoBassOriginalLevel, setPsychoBassOriginalLevel, 100)

    // Presets list
    VIPER_PROP(QStringList, userPresetFiles, setUserPresetFiles, QStringList())

    // Real Audio Pipeline
    VIPER_PROP(bool, audioProcessingActive, setAudioProcessingActive, true)
    VIPER_PROP(bool, systemAudioRouted, setSystemAudioRoutedProperty, false)
    VIPER_PROP(qint64, processedFrames, setProcessedFrames, 0)
    VIPER_PROP(qreal, vuLeft, setVuLeft, 0.0)
    VIPER_PROP(qreal, vuRight, setVuRight, 0.0)

public:
    explicit ViPERState(QObject *parent = nullptr);
    ~ViPERState();
    static ViPERState* instance();

    Q_INVOKABLE void refreshDriverStatus();
    Q_INVOKABLE void reloadEngine();
    Q_INVOKABLE void toggleAudioProcessing();
    Q_INVOKABLE void toggleSystemAudioRouting();
    Q_INVOKABLE void playTestSound();

    Q_INVOKABLE void setEqualizerBandCount(int count);
    Q_INVOKABLE void setEqBandLevel(int bandIndex, qreal level);
    Q_INVOKABLE void applyEqPreset(const QString &name);
    Q_INVOKABLE void resetEq();

    Q_INVOKABLE void savePreset(const QString &name);
    Q_INVOKABLE void loadPreset(const QString &name);
    Q_INVOKABLE void deletePreset(const QString &name);

    Q_PROPERTY(QString defaultIrsFolder READ defaultIrsFolder CONSTANT)
    QString defaultIrsFolder() const;

    Q_INVOKABLE void selectConvolverKernel(const QString &filePath);
    Q_INVOKABLE void selectDdcProfile(const QString &filePath);

    bool loadConvolverKernel(const QString &filePath);
    bool loadDdcProfile(const QString &filePath);
    Q_INVOKABLE void stopEngine();

private:
    void initDefaults();
    void initEqualizerBands(int count);
    void onPropertyChanged(const char *propName);

    // Apply to DSP engine
    void syncMasterLimiter();
    void syncEqualizer();
    void syncBass();
    void syncBassMono();
    void syncClarity();
    void syncConvolver();
    void syncDdc();
    void syncReverb();
    void syncSurround();
    void syncDiffSurround();
    void syncTube();
    void syncAnalogX();
    void syncSpectrumExtension();
    void syncCure();
    void syncDynamicSystem();
    void syncFetCompressor();
    void syncStereoImager();
    void syncPlaybackGain();
    void syncLufs();
    void syncPsychoBass();
    void syncAll();

    std::unique_ptr<ViPER> m_engine;
    std::unique_ptr<class AudioEngineLinux> m_audioEngine;
};
