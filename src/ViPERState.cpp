#include "ViPERState.h"
#include "AudioEngineLinux.h"
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QUrl>
#include <QTextStream>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <iostream>

#include "../ViPER4Mac/ViPERDSP/viper/ViPER.h"
#include "../ViPER4Mac/ViPERDSP/include/ViPERParams.h"
#include "../ViPER4Mac/ViPERDSP/viper/utils/WavReader.h"

ViPERState* ViPERState::instance() {
    static ViPERState state;
    return &state;
}

ViPERState::ViPERState(QObject *parent)
    : QObject(parent),
      m_engine(std::make_unique<ViPER>())
{
    m_engine->SetSamplingRate(48000);
    initDefaults();
    syncAll();
    refreshDriverStatus();

    m_audioEngine = std::make_unique<AudioEngineLinux>(m_engine.get(), this);
    connect(m_audioEngine.get(), &AudioEngineLinux::statusChanged, this, [this](bool active, const QString &text) {
        setAudioProcessingActive(active);
        setDriverStatusText(text);
    });

    QTimer::singleShot(250, this, [this]() {
        if (m_audioEngine) m_audioEngine->start();
    });
}

std::unique_lock<std::mutex> ViPERState::lockEngine() {
    if (m_audioEngine) {
        return std::unique_lock<std::mutex>(m_audioEngine->engineMutex());
    }
    return std::unique_lock<std::mutex>();
}

ViPERState::~ViPERState() {
    stopEngine();
}

void ViPERState::stopEngine() {
    if (m_audioEngine) {
        m_audioEngine->stop();
    }
}

void ViPERState::initDefaults() {
    initEqualizerBands(10);

    // MBC Defaults (5 bands)
    m_mbcBandEnables = { true, true, true, true, true };
    m_mbcThresholds = { -18, -18, -18, -18, -18 };
    m_mbcRatios = { 50, 50, 50, 50, 50 };
    m_mbcGains = { 24, 24, 24, 24, 24 };
    m_mbcAutoGains = { true, true, true, true, true };
    m_mbcAttacks = { 1, 1, 1, 1, 1 };
    m_mbcAutoAttacks = { true, true, true, true, true };
    m_mbcReleases = { 100, 100, 100, 100, 100 };
    m_mbcAutoReleases = { true, true, true, true, true };
    m_mbcKnees = { 0, 0, 0, 0, 0 };
    m_mbcAutoKnees = { true, true, true, true, true };
    m_mbcKneeMultis = { 0, 0, 0, 0, 0 };
    m_mbcMaxAttacks = { 44, 44, 44, 44, 44 };
    m_mbcMaxReleases = { 200, 200, 200, 200, 200 };
    m_mbcCrests = { 100, 100, 100, 100, 100 };
    m_mbcAdapts = { 50, 50, 50, 50, 50 };
    m_mbcNoClips = { true, true, true, true, true };
    m_mbcCrossovers = { 120, 500, 4000, 8000 };

    // Dynamic EQ Defaults (3 bands)
    m_dynEqFreqs = { 400, 1000, 5000 };
    m_dynEqQs = { 150, 150, 200 };
    m_dynEqGains = { 0, 0, 0 };
    m_dynEqThresholds = { -250, -250, -200 };
    m_dynEqAttacks = { 10, 10, 10 };
    m_dynEqReleases = { 100, 100, 100 };
    m_dynEqFilterTypes = { 0, 0, 0 };

    // Built-in presets
    m_presetList = {
        "Flat", "Acoustic", "Bass Booster", "Bass Reducer", "Classical",
        "Deep", "R&B", "Rock", "Small Speakers", "Treble Booster",
        "Treble Reducer", "Vocal Booster"
    };

    // User presets from directory
    QString presetDir = QDir::homePath() + "/.config/viper4linux/presets";
    QDir dir(presetDir);
    if (dir.exists()) {
        QStringList filters;
        filters << "*.json";
        m_userPresetFiles = dir.entryList(filters, QDir::Files);
    }
}

void ViPERState::initEqualizerBands(int count) {
    m_equalizerBandCount = count;
    QVariantList list;
    for (int i = 0; i < count; ++i) {
        list.append(0.0);
    }
    m_equalizerBands = list;
    emit equalizerBandsChanged();
    emit equalizerBandCountChanged();
}

void ViPERState::refreshDriverStatus() {
    QProcess proc;
    proc.start("pactl", QStringList() << "info");
    if (proc.waitForFinished(1500)) {
        QString output = proc.readAllStandardOutput();
        QStringList lines = output.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("Server Name:")) {
                QString name = line.section(':', 1).trimmed();
                if (name.contains("PipeWire", Qt::CaseInsensitive)) {
                    setAudioServerName("PipeWire 1.6.8");
                } else {
                    setAudioServerName(name);
                }
            } else if (line.startsWith("Default Sink:")) {
                QString sink = line.section(':', 1).trimmed();
                if (sink.contains("Speaker", Qt::CaseInsensitive) || sink.contains("Speaker")) {
                    setOutputDeviceName("Built-in Speaker (" + sink.section('.', 0, 0) + ")");
                } else if (sink.contains("Headphone", Qt::CaseInsensitive)) {
                    setOutputDeviceName("Headphones (" + sink.section('.', 0, 0) + ")");
                } else {
                    setOutputDeviceName(sink);
                }
            } else if (line.startsWith("Default Sample Specification:")) {
                QString spec = line.section(':', 1).trimmed();
                if (spec.contains("48000Hz")) setCurrentSampleRate(48000);
                else if (spec.contains("44100Hz")) setCurrentSampleRate(44100);
                else if (spec.contains("96000Hz")) setCurrentSampleRate(96000);
            }
        }
        setDriverInstalled(true);
        setDriverStatusText("Active (Normal)");
        setIsProcessing(true);
    } else {
        // Fallback detection
        setAudioServerName("PipeWire / PulseAudio (Local)");
        setOutputDeviceName("Default Linux Sink");
        setDriverInstalled(true);
        setDriverStatusText("Active");
        setIsProcessing(true);
    }
}

void ViPERState::reloadEngine() {
    if (m_engine) {
        m_engine->ResetAllEffects();
        m_engine->SetSamplingRate(m_currentSampleRate > 0 ? m_currentSampleRate : 48000);
    }
    syncMasterLimiter();
    syncEqualizer();
    syncBass();
    syncBassMono();
    syncClarity();
    syncReverb();
    syncSurround();
    syncDiffSurround();
    syncTube();
    syncAnalogX();
    syncSpectrumExtension();
    syncCure();
    syncDynamicSystem();
    syncFetCompressor();
    syncStereoImager();
    syncPlaybackGain();
    syncLufs();
    syncPsychoBass();
    refreshDriverStatus();

    if (m_audioEngine) {
        m_audioEngine->stop();
        m_audioEngine->start();
    }
}

void ViPERState::toggleAudioProcessing() {
    setIsEnabled(!m_isEnabled);
}

void ViPERState::toggleSystemAudioRouting() {
    setIsEnabled(!m_isEnabled);
}

void ViPERState::playTestSound() {
}

void ViPERState::setEqualizerBandCount(int count) {
    if (count != 10 && count != 15 && count != 25 && count != 31) count = 10;
    if (m_equalizerBandCount != count) {
        m_equalizerBandCount = count;
        initEqualizerBands(count);
        syncEqualizer();
    }
}

QStringList ViPERState::equalizerBandLabels() const {
    if (m_equalizerBandCount == 15) {
        return { "25", "40", "63", "100", "160", "250", "400", "630", "1k", "1.6k", "2.5k", "4k", "6.3k", "10k", "16k" };
    } else if (m_equalizerBandCount == 31) {
        return { "20", "25", "31.5", "40", "50", "63", "80", "100", "125", "160", "200", "250", "315", "400", "500", "630", "800", "1k", "1.25k", "1.6k", "2k", "2.5k", "3.15k", "4k", "5k", "6.3k", "8k", "10k", "12.5k", "16k", "20k" };
    }
    return { "31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
}

void ViPERState::setEqBandLevel(int bandIndex, qreal level) {
    if (bandIndex >= 0 && bandIndex < m_equalizerBands.size()) {
        m_equalizerBands[bandIndex] = level;
        emit equalizerBandsChanged();
        syncEqualizer();
    }
}

void ViPERState::applyEqPreset(const QString &name) {
    setEqualizerPresetName(name);

    // Standard preset mappings for 10 bands
    static const QMap<QString, QVector<float>> presets10 = {
        { "Flat",           { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
        { "Acoustic",       { 4.5f, 4.5f, 3.5f, 1.2f, 1.0f, 0.5f, 1.4f, 1.75f, 3.5f, 2.5f } },
        { "Bass Booster",   { 6.0f, 4.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
        { "Bass Reducer",   { -6.0f, -4.0f, -2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
        { "Classical",      { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -3.0f, -3.0f, -3.0f, -5.0f } },
        { "Deep",           { 3.0f, 2.0f, 1.0f, 0.5f, 0.5f, 0.0f, -1.0f, -2.0f, -3.0f, -3.5f } },
        { "R&B",            { 3.0f, 6.0f, 4.0f, 1.0f, -1.0f, -0.5f, 1.0f, 1.5f, 2.5f, 3.0f } },
        { "Rock",           { 4.0f, 3.0f, 1.0f, 0.0f, -0.5f, 0.0f, 1.5f, 2.5f, 3.5f, 4.0f } },
        { "Small Speakers", { 3.0f, 2.0f, 1.5f, 1.0f, 0.5f, -0.5f, -1.5f, -2.0f, -3.0f, -3.5f } },
        { "Treble Booster", { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f } },
        { "Treble Reducer", { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, -2.0f, -3.0f, -4.0f, -5.0f } },
        { "Vocal Booster",  { -1.0f, -0.5f, 0.0f, 1.5f, 3.0f, 3.0f, 2.0f, 1.0f, 0.0f, -1.0f } }
    };

    if (presets10.contains(name)) {
        const auto &src = presets10[name];
        QVariantList list;
        int count = m_equalizerBandCount;
        for (int i = 0; i < count; ++i) {
            // Map index from count to 10
            float srcIdx = (float)i / (float)(count - 1) * 9.0f;
            int i0 = (int)srcIdx;
            int i1 = std::min(i0 + 1, 9);
            float f = srcIdx - i0;
            float val = src[i0] * (1.0f - f) + src[i1] * f;
            list.append(val);
        }
        m_equalizerBands = list;
        emit equalizerBandsChanged();
        syncEqualizer();
    }
}

void ViPERState::resetEq() {
    applyEqPreset("Flat");
}

void ViPERState::savePreset(const QString &name) {
    if (name.trimmed().isEmpty()) return;
    QString dirPath = QDir::homePath() + "/.config/viper4linux/presets";
    QDir().mkpath(dirPath);

    QJsonObject obj;
    obj["name"] = name;
    obj["outputVolume"] = m_outputVolume;
    obj["channelPan"] = m_channelPan;
    obj["limiter"] = m_limiter;
    obj["equalizerEnabled"] = m_equalizerEnabled;
    obj["equalizerBandCount"] = m_equalizerBandCount;
    QJsonArray bandsArr;
    for (const auto &b : m_equalizerBands) bandsArr.append(b.toDouble());
    obj["equalizerBands"] = bandsArr;
    obj["viperBassEnabled"] = m_viperBassEnabled;
    obj["viperBassMode"] = m_viperBassMode;
    obj["viperBassFrequency"] = m_viperBassFrequency;
    obj["viperBassGain"] = m_viperBassGain;
    obj["viperClarityEnabled"] = m_viperClarityEnabled;
    obj["viperClarityMode"] = m_viperClarityMode;
    obj["viperClarityGain"] = m_viperClarityGain;
    obj["reverberationEnabled"] = m_reverberationEnabled;
    obj["tubeSimulatorEnabled"] = m_tubeSimulatorEnabled;
    obj["analogXEnabled"] = m_analogXEnabled;

    QFile file(dirPath + "/" + name + ".json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
        if (!m_userPresetFiles.contains(name + ".json")) {
            m_userPresetFiles.append(name + ".json");
            emit userPresetFilesChanged();
        }
    }
}

void ViPERState::loadPreset(const QString &name) {
    QString fileName = name.endsWith(".json") ? name : name + ".json";
    QFile file(QDir::homePath() + "/.config/viper4linux/presets/" + fileName);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    if (obj.contains("outputVolume")) setOutputVolume(obj["outputVolume"].toInt());
    if (obj.contains("channelPan")) setChannelPan(obj["channelPan"].toInt());
    if (obj.contains("limiter")) setLimiter(obj["limiter"].toInt());
    if (obj.contains("equalizerEnabled")) setEqualizerEnabled(obj["equalizerEnabled"].toBool());
    if (obj.contains("equalizerBandCount")) setEqualizerBandCount(obj["equalizerBandCount"].toInt());
    if (obj.contains("equalizerBands")) {
        QVariantList list;
        for (const auto &v : obj["equalizerBands"].toArray()) list.append(v.toDouble());
        m_equalizerBands = list;
        emit equalizerBandsChanged();
    }
    if (obj.contains("viperBassEnabled")) setViperBassEnabled(obj["viperBassEnabled"].toBool());
    if (obj.contains("viperBassMode")) setViperBassMode(obj["viperBassMode"].toInt());
    if (obj.contains("viperBassFrequency")) setViperBassFrequency(obj["viperBassFrequency"].toInt());
    if (obj.contains("viperBassGain")) setViperBassGain(obj["viperBassGain"].toInt());
    if (obj.contains("viperClarityEnabled")) setViperClarityEnabled(obj["viperClarityEnabled"].toBool());
    if (obj.contains("viperClarityMode")) setViperClarityMode(obj["viperClarityMode"].toInt());
    if (obj.contains("viperClarityGain")) setViperClarityGain(obj["viperClarityGain"].toInt());
    if (obj.contains("reverberationEnabled")) setReverberationEnabled(obj["reverberationEnabled"].toBool());
    if (obj.contains("tubeSimulatorEnabled")) setTubeSimulatorEnabled(obj["tubeSimulatorEnabled"].toBool());
    if (obj.contains("analogXEnabled")) setAnalogXEnabled(obj["analogXEnabled"].toBool());

    reloadEngine();
}

void ViPERState::deletePreset(const QString &name) {
    QString fileName = name.endsWith(".json") ? name : name + ".json";
    QFile::remove(QDir::homePath() + "/.config/viper4linux/presets/" + fileName);
    m_userPresetFiles.removeAll(fileName);
    emit userPresetFilesChanged();
}

QString ViPERState::defaultIrsFolder() const {
    QString irsDir = QDir::homePath() + "/.local/share/easyeffects/irs";
    if (QDir(irsDir).exists()) {
        return irsDir;
    }
    return QDir::homePath();
}

void ViPERState::selectConvolverKernel(const QString &filePath) {
    QString localPath = filePath;
    if (localPath.startsWith("file://")) {
        localPath = QUrl(filePath).toLocalFile();
    } else if (localPath.contains("%")) {
        localPath = QUrl::fromPercentEncoding(localPath.toUtf8());
    }

    setConvolutionKernelPath(localPath);
    if (!localPath.isEmpty()) {
        bool ok = loadConvolverKernel(localPath);
        setConvolutionEnabled(ok);
    } else {
        setConvolutionEnabled(false);
        if (m_engine) m_engine->UnloadConvolverKernel();
    }
    syncConvolver();
}

bool ViPERState::loadConvolverKernel(const QString &filePath) {
    if (!m_engine || filePath.isEmpty()) return false;

    QByteArray pathBytes = filePath.toUtf8();
    WavData wav{};
    if (!ReadWavFile(pathBytes.constData(), &wav)) {
        qWarning() << "[ViPER][Convolver] Failed to read impulse response file:" << filePath;
        m_engine->UnloadConvolverKernel();
        return false;
    }

    if (wav.samples == nullptr || wav.frame_count < 16 || wav.channels < 1 || wav.channels > 2) {
        qWarning() << "[ViPER][Convolver] Unsupported channel count or frame count:" << wav.channels << wav.frame_count;
        if (wav.samples) delete[] wav.samples;
        m_engine->UnloadConvolverKernel();
        return false;
    }

    static uint32_t s_kernelIdCounter = 1;
    uint32_t kernelId = ++s_kernelIdCounter;

    auto res = m_engine->LoadConvolverKernel(wav.samples, wav.frame_count, wav.channels, kernelId);
    delete[] wav.samples;

    if (!res.has_value()) {
        qWarning() << "[ViPER][Convolver] LoadConvolverKernel failed for:" << filePath;
        return false;
    }

    qDebug() << "[ViPER][Convolver] Successfully loaded kernel:" << filePath
             << "frames=" << wav.frame_count << "ch=" << wav.channels << "sr=" << wav.sample_rate;
    return true;
}

void ViPERState::syncConvolver() {
    if (!m_engine) return;

    viper::ConvolverParams p;
    p.enable = m_isEnabled && m_convolutionEnabled && !m_convolutionKernelPath.isEmpty();
    p.cross_channel = static_cast<float>(m_convolutionCrossChannel) / 100.0f;
    m_engine->ApplyConvolver(p);
}

void ViPERState::selectDdcProfile(const QString &filePath) {
    QString localPath = filePath;
    if (localPath.startsWith("file://")) {
        localPath = QUrl(filePath).toLocalFile();
    } else if (localPath.contains("%")) {
        localPath = QUrl::fromPercentEncoding(localPath.toUtf8());
    }

    setDdcFilePath(localPath);
    if (!localPath.isEmpty()) {
        bool ok = loadDdcProfile(localPath);
        setDdcEnabled(ok);
    } else {
        setDdcEnabled(false);
        if (m_engine) m_engine->LoadDdcCoefficients(nullptr, nullptr, 0);
    }
    syncDdc();
}

bool ViPERState::loadDdcProfile(const QString &filePath) {
    if (!m_engine || filePath.isEmpty()) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ViPER][DDC] Failed to open DDC file:" << filePath;
        return false;
    }

    QTextStream in(&file);
    QVector<float> coeffs44100;
    QVector<float> coeffs48000;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("SR_44100:", Qt::CaseInsensitive)) {
            QString str = line.mid(9).trimmed();
            QStringList tokens = str.split(',', Qt::SkipEmptyParts);
            for (const QString &t : tokens) {
                bool ok = false;
                float val = t.trimmed().toFloat(&ok);
                if (ok) coeffs44100.append(val);
            }
        } else if (line.startsWith("SR_48000:", Qt::CaseInsensitive)) {
            QString str = line.mid(9).trimmed();
            QStringList tokens = str.split(',', Qt::SkipEmptyParts);
            for (const QString &t : tokens) {
                bool ok = false;
                float val = t.trimmed().toFloat(&ok);
                if (ok) coeffs48000.append(val);
            }
        }
    }
    file.close();

    if (coeffs44100.isEmpty() || coeffs48000.isEmpty() ||
        coeffs44100.size() != coeffs48000.size() || (coeffs44100.size() % 5 != 0)) {
        qWarning() << "[ViPER][DDC] Invalid coefficients format in:" << filePath;
        return false;
    }

    uint32_t sectionCount = coeffs44100.size() / 5;
    const viper::BiquadSection *sec44 = reinterpret_cast<const viper::BiquadSection *>(coeffs44100.constData());
    const viper::BiquadSection *sec48 = reinterpret_cast<const viper::BiquadSection *>(coeffs48000.constData());

    m_engine->LoadDdcCoefficients(sec44, sec48, sectionCount);
    qDebug() << "[ViPER][DDC] Loaded DDC profile successfully:" << filePath << "sections=" << sectionCount;
    return true;
}

void ViPERState::syncDdc() {
    if (!m_engine) return;

    viper::DdcParams p;
    p.enable = m_isEnabled && m_ddcEnabled && !m_ddcFilePath.isEmpty();
    m_engine->ApplyDdc(p);
}

void ViPERState::onPropertyChanged(const char *propName) {
    if (!m_engine) return;

    QString name(propName);
    if (name == "isEnabled") {
        if (m_audioEngine) {
            m_audioEngine->setMasterEnabled(m_isEnabled);
        }
        syncAll();
        return;
    }
    if (name.startsWith("output") || name == "channelPan" || name == "limiter") {
        syncMasterLimiter();
    } else if (name.startsWith("equalizer")) {
        syncEqualizer();
    } else if (name.startsWith("viperBassMono")) {
        syncBassMono();
    } else if (name.startsWith("viperBass")) {
        syncBass();
    } else if (name.startsWith("viperClarity")) {
        syncClarity();
    } else if (name.startsWith("reverberation")) {
        syncReverb();
    } else if (name.startsWith("fieldSurround")) {
        syncSurround();
    } else if (name.startsWith("diffSurround")) {
        syncDiffSurround();
    } else if (name.startsWith("tubeSimulator")) {
        syncTube();
    } else if (name.startsWith("analogX")) {
        syncAnalogX();
    } else if (name.startsWith("spectrumExtension")) {
        syncSpectrumExtension();
    } else if (name.startsWith("cure")) {
        syncCure();
    } else if (name.startsWith("dynamicSystem") || name.startsWith("ds")) {
        syncDynamicSystem();
    } else if (name.startsWith("fetCompressor")) {
        syncFetCompressor();
    } else if (name.startsWith("stereoImg")) {
        syncStereoImager();
    } else if (name.startsWith("playbackGain")) {
        syncPlaybackGain();
    } else if (name.startsWith("lufs")) {
        syncLufs();
    } else if (name.startsWith("psychoBass")) {
        syncPsychoBass();
    } else if (name.startsWith("convolution")) {
        syncConvolver();
    } else if (name.startsWith("ddc")) {
        syncDdc();
    } else if (name == "speakerCorrectionEnabled") {
        m_engine->ApplySpeakerCorrection({ m_speakerCorrectionEnabled });
    }
}

// Typed sync methods - zero console spam
void ViPERState::syncMasterLimiter() {
    if (!m_engine) return;
    auto lock = lockEngine();
    viper::MasterLimiterParams p;
    p.threshold = static_cast<float>(m_limiter) / 100.0f;
    p.output_volume = static_cast<float>(m_outputVolume) / 100.0f;
    p.channel_pan = static_cast<float>(m_channelPan) / 100.0f;
    m_engine->ApplyMasterLimiter(p);
}

void ViPERState::syncEqualizer() {
    if (!m_engine) return;
    auto lock = lockEngine();
    viper::EqualizerParams p;
    p.enable = m_isEnabled && m_equalizerEnabled;
    p.band_count = static_cast<uint32_t>(m_equalizerBandCount);
    for (int i = 0; i < m_equalizerBands.size() && i < (int)p.band_levels.size(); ++i) {
        p.band_levels[i] = static_cast<float>(m_equalizerBands[i].toDouble());
    }
    m_engine->ApplyEqualizer(p);
}

void ViPERState::syncBass() {
    viper::BassParams p;
    p.enable = m_isEnabled && m_viperBassEnabled;
    p.mode = m_viperBassMode;
    p.frequency = static_cast<uint32_t>(m_viperBassFrequency);
    p.gain = static_cast<float>(m_viperBassGain) / 100.0f;
    p.anti_pop = m_viperBassAntiPop;
    m_engine->ApplyBass(p);
}

void ViPERState::syncBassMono() {
    viper::BassMonoParams p;
    p.enable = m_isEnabled && m_viperBassMonoEnabled;
    p.mode = m_viperBassMonoMode;
    p.frequency = static_cast<uint32_t>(m_viperBassMonoFrequency);
    p.gain = static_cast<float>(m_viperBassMonoGain) / 100.0f;
    p.anti_pop = m_viperBassMonoAntiPop;
    m_engine->ApplyBassMono(p);
}

void ViPERState::syncClarity() {
    viper::ClarityParams p;
    p.enable = m_isEnabled && m_viperClarityEnabled;
    p.mode = m_viperClarityMode;
    p.gain = static_cast<float>(m_viperClarityGain) / 100.0f;
    m_engine->ApplyClarity(p);
}

void ViPERState::syncReverb() {
    viper::ReverbParams p;
    p.enable = m_isEnabled && m_reverberationEnabled;
    p.room_size = static_cast<float>(m_reverberationRoomSize) / 100.0f;
    p.width = static_cast<float>(m_reverberationRoomWidth) / 100.0f;
    p.damp = static_cast<float>(m_reverberationRoomDampening) / 100.0f;
    p.wet = static_cast<float>(m_reverberationWetSignal) / 100.0f;
    p.dry = static_cast<float>(m_reverberationDrySignal) / 100.0f;
    m_engine->ApplyReverb(p);
}

void ViPERState::syncSurround() {
    viper::FieldSurroundParams p;
    p.enable = m_isEnabled && m_fieldSurroundEnabled;
    p.widening = m_fieldSurroundWidening;
    p.mid_image = m_fieldSurroundMidImage;
    p.depth = m_fieldSurroundDepth;
    m_engine->ApplyFieldSurround(p);
}

void ViPERState::syncDiffSurround() {
    viper::DiffSurroundParams p;
    p.enable = m_isEnabled && m_diffSurroundEnabled;
    p.delay = m_diffSurroundDelay;
    p.reverse = m_diffSurroundReverse;
    p.wet_dry_mix = m_diffSurroundWetDryMix;
    p.lp_cutoff = m_diffSurroundLpCutoff;
    m_engine->ApplyDiffSurround(p);
}

void ViPERState::syncTube() {
    m_engine->ApplyTubeSimulator({ m_isEnabled && m_tubeSimulatorEnabled });
}

void ViPERState::syncAnalogX() {
    viper::AnalogXParams p;
    p.enable = m_isEnabled && m_analogXEnabled;
    p.mode = m_analogXMode;
    m_engine->ApplyAnalogX(p);
}

void ViPERState::syncSpectrumExtension() {
    viper::SpectrumExtensionParams p;
    p.enable = m_isEnabled && m_spectrumExtensionEnabled;
    p.strength = m_spectrumExtensionBark;
    p.exciter = static_cast<float>(m_spectrumExtensionBarkReconstruct) / 100.0f;
    m_engine->ApplySpectrumExtension(p);
}

void ViPERState::syncCure() {
    viper::CureParams p;
    p.enable = m_isEnabled && m_cureEnabled;
    p.crossfeed_preset = m_cureCrossfeedStrength;
    m_engine->ApplyCure(p);
}

void ViPERState::syncDynamicSystem() {
    viper::DynamicSystemParams p;
    p.enable = m_isEnabled && m_dynamicSystemEnabled;
    p.x_coeff_low = m_dsXLow;
    p.x_coeff_high = m_dsXHigh;
    p.y_coeff_low = m_dsYLow;
    p.y_coeff_high = m_dsYHigh;
    p.side_gain_low = static_cast<float>(m_dsSideGainLow) / 100.0f;
    p.side_gain_high = static_cast<float>(m_dsSideGainHigh) / 100.0f;
    p.strength = static_cast<float>(m_dynamicSystemStrength) / 100.0f;
    m_engine->ApplyDynamicSystem(p);
}

void ViPERState::syncFetCompressor() {
    viper::FetCompressorParams p;
    p.enable = m_isEnabled && m_fetCompressorEnabled;
    p.threshold = static_cast<float>(m_fetCompressorThreshold);
    p.ratio = static_cast<float>(m_fetCompressorRatio) / 100.0f;
    p.knee = static_cast<float>(m_fetCompressorKnee);
    p.knee_auto = m_fetCompressorAutoKnee;
    p.gain = static_cast<float>(m_fetCompressorGain);
    p.gain_auto = m_fetCompressorAutoGain;
    p.attack = static_cast<float>(m_fetCompressorAttack);
    p.attack_auto = m_fetCompressorAutoAttack;
    p.release = static_cast<float>(m_fetCompressorRelease);
    p.release_auto = m_fetCompressorAutoRelease;
    p.knee_multi = static_cast<float>(m_fetCompressorKneeMulti) / 100.0f;
    p.max_attack = static_cast<float>(m_fetCompressorMaxAttack);
    p.max_release = static_cast<float>(m_fetCompressorMaxRelease);
    p.crest = static_cast<float>(m_fetCompressorCrest);
    p.adapt = static_cast<float>(m_fetCompressorAdapt) / 100.0f;
    p.no_clip = m_fetCompressorNoClip;
    m_engine->ApplyFetCompressor(p);
}

void ViPERState::syncStereoImager() {
    viper::StereoImagerParams p;
    p.enable = m_isEnabled && m_stereoImgEnabled;
    p.low_width = static_cast<float>(m_stereoImgLowWidth) / 100.0f;
    p.mid_width = static_cast<float>(m_stereoImgMidWidth) / 100.0f;
    p.high_width = static_cast<float>(m_stereoImgHighWidth) / 100.0f;
    p.low_crossover = m_stereoImgLowCrossover;
    p.high_crossover = m_stereoImgHighCrossover;
    m_engine->ApplyStereoImager(p);
}

void ViPERState::syncPlaybackGain() {
    viper::PlaybackGainControlParams p;
    p.enable = m_isEnabled && m_playbackGainEnabled;
    p.strength = static_cast<float>(m_playbackGainStrength) / 100.0f;
    p.max_gain = static_cast<float>(m_playbackGainMaxGain) / 100.0f;
    p.output_threshold = static_cast<float>(m_playbackGainOutputThreshold) / 100.0f;
    m_engine->ApplyPlaybackGainControl(p);
}

void ViPERState::syncLufs() {
    viper::LufsParams p;
    p.enable = m_isEnabled && m_lufsEnabled;
    p.target = static_cast<float>(m_lufsTarget) / -10.0f;
    p.max_gain = static_cast<float>(m_lufsMaxGain) / 10.0f;
    p.speed = m_lufsSpeed;
    m_engine->ApplyLufs(p);
}

void ViPERState::syncPsychoBass() {
    viper::PsychoacousticBassParams p;
    p.enable = m_isEnabled && m_psychoBassEnabled;
    p.cutoff = static_cast<uint32_t>(m_psychoBassCutoff);
    p.intensity = static_cast<uint32_t>(m_psychoBassIntensity);
    p.harmonic_order = static_cast<uint32_t>(m_psychoBassHarmonicOrder);
    p.original_level = static_cast<uint32_t>(m_psychoBassOriginalLevel);
    m_engine->ApplyPsychoacousticBass(p);
}

void ViPERState::syncAll() {
    syncMasterLimiter();
    syncEqualizer();
    syncBass();
    syncBassMono();
    syncClarity();
    syncReverb();
    syncSurround();
    syncDiffSurround();
    syncTube();
    syncAnalogX();
    syncSpectrumExtension();
    syncCure();
    syncDynamicSystem();
    syncFetCompressor();
    syncStereoImager();
    syncPlaybackGain();
    syncLufs();
    syncPsychoBass();
    syncConvolver();
    syncDdc();
}
