#include "AudioEngineLinux.h"
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <cstdlib>
#include <pulse/simple.h>
#include <pulse/error.h>

#include "../ViPER4Mac/ViPERDSP/viper/ViPER.h"

static const char *kVirtualSinkName = "ViPER4Linux_Sink";
static const char *kVirtualMonitorName = "ViPER4Linux_Sink.monitor";

AudioEngineLinux::AudioEngineLinux(ViPER *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
}

AudioEngineLinux::~AudioEngineLinux() {
    stop();
}

QString AudioEngineLinux::getDefaultSink() {
    QProcess proc;
    proc.start("pactl", QStringList() << "get-default-sink");
    if (proc.waitForFinished(1000)) {
        return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }
    return QString();
}

void AudioEngineLinux::setDefaultSink(const QString &sinkName) {
    if (sinkName.isEmpty()) return;
    QProcess::execute("pactl", QStringList() << "set-default-sink" << sinkName);
}

QString AudioEngineLinux::getHardwareOutputSink() {
    // 1. Check current system default sink first (as long as it's not ViPER)
    QString current = getDefaultSink();
    if (!current.isEmpty() && !current.contains("ViPER", Qt::CaseInsensitive)) {
        m_hardwareOutputSink = current;
        return current;
    }

    // 2. If cached hardware sink is valid and non-ViPER, return it
    if (!m_hardwareOutputSink.isEmpty() && !m_hardwareOutputSink.contains("ViPER", Qt::CaseInsensitive)) {
        return m_hardwareOutputSink;
    }

    // 3. Query all non-ViPER sinks from pactl
    QProcess proc;
    proc.start("sh", QStringList() << "-c" << "pactl list sinks short 2>/dev/null | grep -v -i ViPER | awk '{print $2}'");
    if (proc.waitForFinished(1000)) {
        QStringList sinks = QString::fromUtf8(proc.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        // Priority 1: Bluetooth headphones (e.g. Airdopes 161)
        for (const QString &s : sinks) {
            QString trimmed = s.trimmed();
            if (trimmed.startsWith("bluez_output")) {
                m_hardwareOutputSink = trimmed;
                return trimmed;
            }
        }
        // Priority 2: Built-in Speaker / analog output
        for (const QString &s : sinks) {
            QString trimmed = s.trimmed();
            if (trimmed.contains("Speaker", Qt::CaseInsensitive) || trimmed.contains("analog", Qt::CaseInsensitive)) {
                m_hardwareOutputSink = trimmed;
                return trimmed;
            }
        }
        // Priority 3: First non-HDMI sink
        for (const QString &s : sinks) {
            QString trimmed = s.trimmed();
            if (!trimmed.contains("HDMI", Qt::CaseInsensitive)) {
                m_hardwareOutputSink = trimmed;
                return trimmed;
            }
        }
        if (!sinks.isEmpty()) {
            m_hardwareOutputSink = sinks.first().trimmed();
            return m_hardwareOutputSink;
        }
    }
    return QString();
}

QString AudioEngineLinux::getSinkVolume(const QString &sinkName) {
    if (sinkName.isEmpty()) return QString();
    QProcess proc;
    proc.start("pactl", QStringList() << "get-sink-volume" << sinkName);
    if (!proc.waitForFinished(500)) return QString();
    QString out = QString::fromUtf8(proc.readAllStandardOutput());
    static QRegularExpression re(R"((\d+)%)");
    auto match = re.match(out);
    if (match.hasMatch()) {
        return match.captured(1) + "%";
    }
    return QString();
}

bool AudioEngineLinux::getSinkMute(const QString &sinkName) {
    if (sinkName.isEmpty()) return false;
    QProcess proc;
    proc.start("pactl", QStringList() << "get-sink-mute" << sinkName);
    if (!proc.waitForFinished(500)) return false;
    QString out = QString::fromUtf8(proc.readAllStandardOutput()).toLower();
    return out.contains("yes");
}

void AudioEngineLinux::setSinkVolume(const QString &sinkName, const QString &volume) {
    if (sinkName.isEmpty() || volume.isEmpty()) return;
    QProcess::execute("pactl", QStringList() << "set-sink-volume" << sinkName << volume);
}

void AudioEngineLinux::setSinkMute(const QString &sinkName, bool muted) {
    if (sinkName.isEmpty()) return;
    QProcess::execute("pactl", QStringList() << "set-sink-mute" << sinkName << (muted ? "1" : "0"));
}

void AudioEngineLinux::createVirtualSink() {
    // 1. Capture current hardware sink's volume and mute BEFORE creating virtual sink
    QString hwSink = getHardwareOutputSink();
    QString initialVol;
    bool initialMute = false;
    if (!hwSink.isEmpty()) {
        initialVol = getSinkVolume(hwSink);
        initialMute = getSinkMute(hwSink);
    }

    // 2. Unload all previous instances cleanly
    system("for id in $(pactl list modules short 2>/dev/null | grep -i ViPER4Linux_Sink | awk '{print $1}'); do pactl unload-module $id 2>/dev/null; done");

    // 3. Load module-null-sink
    QProcess procLoad;
    procLoad.start("pactl", QStringList() 
        << "load-module" << "module-null-sink" 
        << QString("sink_name=%1").arg(kVirtualSinkName)
        << "sink_properties=device.description=ViPER4Linux_Virtual_Sink"
    );
    if (procLoad.waitForFinished(2000)) {
        m_virtualSinkModuleId = QString::fromUtf8(procLoad.readAllStandardOutput()).trimmed();
    }

    // 4. Immediately initialize virtual sink with identical volume and mute as the hardware sink
    if (!initialVol.isEmpty()) {
        setSinkVolume(kVirtualSinkName, initialVol);
        m_lastSyncedVolume = initialVol;
    }
    setSinkMute(kVirtualSinkName, initialMute);
    m_lastSyncedMute = initialMute;
}

void AudioEngineLinux::removeVirtualSink() {
    if (!m_hardwareOutputSink.isEmpty()) {
        // Ensure hardware sink has latest volume before restoring
        QString vVol = getSinkVolume(kVirtualSinkName);
        if (!vVol.isEmpty()) {
            setSinkVolume(m_hardwareOutputSink, vVol);
        }
        setSinkMute(m_hardwareOutputSink, getSinkMute(kVirtualSinkName));

        setDefaultSink(m_hardwareOutputSink);
        moveActiveStreamsTo(m_hardwareOutputSink);
    }

    system("for id in $(pactl list modules short 2>/dev/null | grep -i ViPER4Linux_Sink | awk '{print $1}'); do pactl unload-module $id 2>/dev/null; done");
    m_virtualSinkModuleId.clear();
}

void AudioEngineLinux::moveActiveStreamsTo(const QString &targetSink) {
    if (targetSink.isEmpty()) return;

    QProcess proc;
    proc.start("pactl", QStringList() << "list" << "sink-inputs");
    if (!proc.waitForFinished(1000)) return;

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList sections = output.split("Sink Input #", Qt::SkipEmptyParts);

    qint64 myPid = QCoreApplication::applicationPid();
    QString myPidStr = QString("application.process.id = \"%1\"").arg(myPid);

    for (const QString &section : sections) {
        int firstLineEnd = section.indexOf('\n');
        if (firstLineEnd <= 0) continue;
        QString idStr = section.left(firstLineEnd).trimmed();
        bool ok = false;
        int inputId = idStr.toInt(&ok);
        if (!ok) continue;

        // Skip our own process and any ViPER playback stream
        if (section.contains(myPidStr) || section.contains("ViPER", Qt::CaseInsensitive)) {
            continue;
        }

        // Move client stream to targetSink
        QProcess::execute("pactl", QStringList() << "move-sink-input" << QString::number(inputId) << targetSink);
    }
}

void AudioEngineLinux::setMasterEnabled(bool enabled) {
    if (m_masterEnabled == enabled) return;
    m_masterEnabled = enabled;

    if (m_masterEnabled) {
        // Sync hardware volume to virtual sink before switching
        QString hwSink = getHardwareOutputSink();
        if (!hwSink.isEmpty()) {
            QString hwVol = getSinkVolume(hwSink);
            if (!hwVol.isEmpty()) {
                setSinkVolume(kVirtualSinkName, hwVol);
                m_lastSyncedVolume = hwVol;
            }
            bool hwMute = getSinkMute(hwSink);
            setSinkMute(kVirtualSinkName, hwMute);
            m_lastSyncedMute = hwMute;
        }

        setDefaultSink(kVirtualSinkName);
        moveActiveStreamsTo(kVirtualSinkName);
        emit statusChanged(true, "Processing Live Audio");
    } else {
        if (!m_hardwareOutputSink.isEmpty()) {
            // Sync virtual sink volume to hardware sink before switching
            QString vVol = getSinkVolume(kVirtualSinkName);
            if (!vVol.isEmpty()) {
                setSinkVolume(m_hardwareOutputSink, vVol);
                m_lastSyncedVolume = vVol;
            }
            bool vMute = getSinkMute(kVirtualSinkName);
            setSinkMute(m_hardwareOutputSink, vMute);
            m_lastSyncedMute = vMute;

            setDefaultSink(m_hardwareOutputSink);
            moveActiveStreamsTo(m_hardwareOutputSink);
        }
        emit statusChanged(false, "Bypassed");
    }
}

void AudioEngineLinux::handleSubscriptionOutput() {
    if (!m_subscribeProc) return;
    QByteArray data = m_subscribeProc->readAllStandardOutput();
    if (data.contains("sink")) {
        syncVolumes();
    }
}

void AudioEngineLinux::syncVolumes() {
    if (m_isSyncing.load()) return;
    if (!m_running.load() || !m_masterEnabled.load()) return;

    QString hwSink = getHardwareOutputSink();
    if (hwSink.isEmpty()) return;

    QString vVirtual = getSinkVolume(kVirtualSinkName);
    QString vHw = getSinkVolume(hwSink);
    if (vVirtual.isEmpty() || vHw.isEmpty()) return;

    bool mVirtual = getSinkMute(kVirtualSinkName);
    bool mHw = getSinkMute(hwSink);

    // If already identical, nothing to do
    if (vVirtual == vHw && mVirtual == mHw) {
        m_lastSyncedVolume = vVirtual;
        m_lastSyncedMute = mVirtual;
        return;
    }

    m_isSyncing.store(true);

    // 1. Volume Synchronization
    if (vVirtual != m_lastSyncedVolume) {
        // User changed virtual sink (keyboard volume keys / KDE OSD) -> sync to hardware
        setSinkVolume(hwSink, vVirtual);
        m_lastSyncedVolume = vVirtual;
    } else if (vHw != m_lastSyncedVolume) {
        // User changed hardware sink (bluetooth controls / settings) -> sync to virtual
        setSinkVolume(kVirtualSinkName, vHw);
        m_lastSyncedVolume = vHw;
    } else {
        // Fallback default to virtual sink
        setSinkVolume(hwSink, vVirtual);
        m_lastSyncedVolume = vVirtual;
    }

    // 2. Mute Synchronization
    if (mVirtual != m_lastSyncedMute) {
        setSinkMute(hwSink, mVirtual);
        m_lastSyncedMute = mVirtual;
    } else if (mHw != m_lastSyncedMute) {
        setSinkMute(kVirtualSinkName, mHw);
        m_lastSyncedMute = mHw;
    }

    m_isSyncing.store(false);
}

bool AudioEngineLinux::start() {
    if (m_running.load()) return true;

    m_hardwareOutputSink = getHardwareOutputSink();
    m_shouldStop.store(false);
    m_running.store(true);

    m_workerThread = std::thread([this]() {
        createVirtualSink();
        
        if (m_masterEnabled) {
            setDefaultSink(kVirtualSinkName);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            moveActiveStreamsTo(kVirtualSinkName);
        }

        runAudioLoop();
    });

    // Start background pactl subscribe process to track sink events instantaneously
    if (!m_subscribeProc) {
        m_subscribeProc = new QProcess(this);
        connect(m_subscribeProc, &QProcess::readyReadStandardOutput, this, &AudioEngineLinux::handleSubscriptionOutput);
        m_subscribeProc->start("pactl", QStringList() << "subscribe");
    }

    // Periodic heartbeat timer (every 200ms) to ensure guaranteed lockstep
    if (!m_volumeSyncTimer) {
        m_volumeSyncTimer = new QTimer(this);
        connect(m_volumeSyncTimer, &QTimer::timeout, this, &AudioEngineLinux::syncVolumes);
        m_volumeSyncTimer->start(200);
    }

    emit statusChanged(m_masterEnabled, m_masterEnabled ? "Processing Live Audio" : "Bypassed");
    return true;
}

void AudioEngineLinux::stop() {
    if (!m_running.load()) return;

    if (m_volumeSyncTimer) {
        m_volumeSyncTimer->stop();
        delete m_volumeSyncTimer;
        m_volumeSyncTimer = nullptr;
    }

    if (m_subscribeProc) {
        m_subscribeProc->terminate();
        m_subscribeProc->waitForFinished(500);
        delete m_subscribeProc;
        m_subscribeProc = nullptr;
    }

    m_shouldStop.store(true);
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    removeVirtualSink();
    m_running.store(false);
    emit statusChanged(false, "Stopped");
}

void AudioEngineLinux::runAudioLoop() {
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = 48000;
    ss.channels = 2;

    int err = 0;
    pa_simple *s_record = nullptr;
    pa_simple *s_playback = nullptr;

    const size_t framesPerChunk = 1024;
    const size_t samplesPerChunk = framesPerChunk * 2; // Stereo
    std::vector<float> buffer(samplesPerChunk, 0.0f);

    while (!m_shouldStop.load()) {
        // 1. Maintain s_record permanently locked to virtual sink monitor
        if (!s_record) {
            s_record = pa_simple_new(
                nullptr, "ViPER4Linux", PA_STREAM_RECORD,
                kVirtualMonitorName, "ViPER Monitor Record",
                &ss, nullptr, nullptr, &err
            );
            if (!s_record) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
        }

        // 2. If master toggle is OFF (disabled):
        // Disconnect s_playback so it produces zero sound and vanishes from physical hardware
        if (!m_masterEnabled.load()) {
            if (s_playback) {
                pa_simple_free(s_playback);
                s_playback = nullptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // 3. If master toggle is ON (enabled):
        // Ensure s_playback is open and locked strictly to physical hardware sink
        if (!s_playback) {
            QString hwSink = getHardwareOutputSink();
            QByteArray hwBytes = hwSink.toUtf8();
            const char *targetDev = hwBytes.isEmpty() ? nullptr : hwBytes.constData();

            s_playback = pa_simple_new(
                nullptr, "ViPER4Linux", PA_STREAM_PLAYBACK,
                targetDev, "ViPER Enhanced Playback",
                &ss, nullptr, nullptr, &err
            );
            if (!s_playback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
        }

        // 4. Capture one audio chunk from virtual sink
        if (pa_simple_read(s_record, buffer.data(), samplesPerChunk * sizeof(float), &err) < 0) {
            pa_simple_free(s_record);
            s_record = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        // 5. Apply ViPER DSP Processing
        {
            std::lock_guard<std::mutex> lock(m_engineMutex);
            if (m_engine) {
                m_engine->Process(buffer, framesPerChunk);
            }
        }

        // 6. Play processed audio directly into physical hardware sink
        if (s_playback) {
            if (pa_simple_write(s_playback, buffer.data(), samplesPerChunk * sizeof(float), &err) < 0) {
                pa_simple_free(s_playback);
                s_playback = nullptr;
            }
        }
    }

    if (s_record) pa_simple_free(s_record);
    if (s_playback) pa_simple_free(s_playback);
}
