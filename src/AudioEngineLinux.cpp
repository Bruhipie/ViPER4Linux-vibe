#include "AudioEngineLinux.h"
#include <QProcess>
#include <QCoreApplication>
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
    // 1. If we already found the hardware sink and it's valid, return it
    if (!m_hardwareOutputSink.isEmpty() && !m_hardwareOutputSink.contains("ViPER", Qt::CaseInsensitive)) {
        return m_hardwareOutputSink;
    }

    // 2. Query default sink if not ViPER
    QString current = getDefaultSink();
    if (!current.isEmpty() && !current.contains("ViPER", Qt::CaseInsensitive)) {
        m_hardwareOutputSink = current;
        return current;
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

void AudioEngineLinux::createVirtualSink() {
    // Unload all previous instances cleanly
    system("for id in $(pactl list modules short 2>/dev/null | grep -i ViPER4Linux_Sink | awk '{print $1}'); do pactl unload-module $id 2>/dev/null; done");

    // Load module-null-sink
    QProcess procLoad;
    procLoad.start("pactl", QStringList() 
        << "load-module" << "module-null-sink" 
        << QString("sink_name=%1").arg(kVirtualSinkName)
        << "sink_properties=device.description=ViPER4Linux_Virtual_Sink"
    );
    if (procLoad.waitForFinished(2000)) {
        m_virtualSinkModuleId = QString::fromUtf8(procLoad.readAllStandardOutput()).trimmed();
    }
}

void AudioEngineLinux::removeVirtualSink() {
    if (!m_hardwareOutputSink.isEmpty()) {
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

    emit statusChanged(m_masterEnabled, m_masterEnabled ? "Processing Live Audio" : "Bypassed");
    return true;
}

void AudioEngineLinux::stop() {
    if (!m_running.load()) return;

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

        int readRes = pa_simple_read(s_record, buffer.data(), buffer.size() * sizeof(float), &err);
        if (readRes < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(m_engineMutex);
            if (m_engine) {
                m_engine->Process(buffer, framesPerChunk);
            }
        }

        pa_simple_write(s_playback, buffer.data(), buffer.size() * sizeof(float), &err);
    }

    if (s_record) pa_simple_free(s_record);
    if (s_playback) pa_simple_free(s_playback);
}
