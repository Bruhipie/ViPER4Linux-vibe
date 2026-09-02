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

static const char *kVirtualSinkName    = "ViPER4Linux_Sink";
static const char *kVirtualMonitorName = "ViPER4Linux_Sink.monitor";
static const char *kOutputSinkName     = "ViPER4Linux_Out";
static const char *kOutputMonitorName  = "ViPER4Linux_Out.monitor";


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
    // Topology (matches EasyEffects patchbay):
    //   apps → ViPER4Linux_Sink → [ViPER DSP thread reads monitor] →
    //          writes to ViPER4Linux_Out → module-loopback → hardware

    // 1. Tear down any leftover modules from previous run
    system("for id in $(pactl list modules short 2>/dev/null | grep -iE 'ViPER4Linux' | awk '{print $1}'); do pactl unload-module $id 2>/dev/null; done");

    auto loadSink = [](const char *name, const char *desc) -> QString {
        QProcess p;
        p.start("pactl", QStringList()
            << "load-module" << "module-null-sink"
            << QString("sink_name=%1").arg(name)
            << QString("sink_properties=device.description=%1").arg(desc));
        p.waitForFinished(2000);
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };

    // 2. Input sink — where all apps play to
    m_virtualSinkModuleId = loadSink(kVirtualSinkName, "ViPER4Linux_Input");
    setSinkVolume(kVirtualSinkName, "100%");

    // 3. Output sink — where ViPER writes processed audio
    m_outputSinkModuleId = loadSink(kOutputSinkName, "ViPER4Linux_Output");
    setSinkVolume(kOutputSinkName, "100%");

    // 4. Loopback: Out.monitor → hardware (creates the visible connection in Helvum)
    QString hwSink = getHardwareOutputSink();
    if (!hwSink.isEmpty()) {
        QProcess lp;
        lp.start("pactl", QStringList()
            << "load-module" << "module-loopback"
            << QString("source=%1").arg(kOutputMonitorName)
            << QString("sink=%1").arg(hwSink)
            << "latency_msec=30"
            << "adjust_time=0");
        lp.waitForFinished(2000);
        m_loopbackModuleId = QString::fromUtf8(lp.readAllStandardOutput()).trimmed();
    }

    m_lastSyncedVolume = "100%";
}


void AudioEngineLinux::removeVirtualSink() {
    if (!m_hardwareOutputSink.isEmpty()) {
        setDefaultSink(m_hardwareOutputSink);
        moveActiveStreamsTo(m_hardwareOutputSink);
    }

    // Unload loopback first, then sinks
    auto unloadModule = [](const QString &id) {
        if (!id.isEmpty())
            QProcess::execute("pactl", QStringList() << "unload-module" << id);
    };
    unloadModule(m_loopbackModuleId);   m_loopbackModuleId.clear();
    unloadModule(m_outputSinkModuleId); m_outputSinkModuleId.clear();
    unloadModule(m_virtualSinkModuleId); m_virtualSinkModuleId.clear();
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
        // Lock virtual sink to 100% again (it's always at 100% when ViPER is active)
        setSinkVolume(kVirtualSinkName, "100%");
        setSinkMute(kVirtualSinkName, false);
        m_lastSyncedVolume = "100%";

        setDefaultSink(kVirtualSinkName);
        moveActiveStreamsTo(kVirtualSinkName);
        emit statusChanged(true, "Processing Live Audio");
    } else {
        if (!m_hardwareOutputSink.isEmpty()) {
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

    m_isSyncing.store(true);

    // Strategy: ViPER4Linux_Sink is ALWAYS locked at 100%.
    // Volume keys change the virtual sink; we detect this, apply the new level
    // to the hardware output device, and immediately reset the virtual sink back to 100%.
    // This way the system volume OSD reflects the user's intent while virtual stays at unity.

    QString vVirtual = getSinkVolume(kVirtualSinkName);
    if (!vVirtual.isEmpty() && vVirtual != "100%") {
        // User pressed volume keys — apply their intent directly to hardware
        setSinkVolume(hwSink, vVirtual);
        // Lock virtual sink back to 100%
        setSinkVolume(kVirtualSinkName, "100%");
    }

    // Mute: if user muted the virtual sink, apply mute to hardware and unmute virtual
    bool mVirtual = getSinkMute(kVirtualSinkName);
    if (mVirtual) {
        setSinkMute(hwSink, true);
        setSinkMute(kVirtualSinkName, false);
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
        // Write to ViPER4Linux_Out (which loopback forwards to hardware)
        if (!s_playback) {
            s_playback = pa_simple_new(
                nullptr, "ViPER4Linux", PA_STREAM_PLAYBACK,
                kOutputSinkName, "ViPER Enhanced Playback",
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
