#include "AudioEngineLinux.h"
#include <QProcess>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <pipewire/pipewire.h>
#include <pipewire/filter.h>
#include <pipewire/thread-loop.h>
#include <spa/param/audio/format-utils.h>

#include "../ViPER4Mac/ViPERDSP/viper/ViPER.h"

AudioEngineLinux::AudioEngineLinux(ViPER *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
}

AudioEngineLinux::~AudioEngineLinux() {
    stop();
}

void AudioEngineLinux::onProcessCallback(void *userdata, struct spa_io_position *position) {
    auto *self = static_cast<AudioEngineLinux *>(userdata);
    if (self) {
        self->processAudio(position);
    }
}

void AudioEngineLinux::processAudio(struct spa_io_position *position) {
    if (!position) return;
    uint32_t n_samples = position->clock.duration;
    if (n_samples == 0) return;

    float *in_l = static_cast<float *>(pw_filter_get_dsp_buffer(m_inPortL, n_samples));
    float *in_r = static_cast<float *>(pw_filter_get_dsp_buffer(m_inPortR, n_samples));
    float *out_l = static_cast<float *>(pw_filter_get_dsp_buffer(m_outPortL, n_samples));
    float *out_r = static_cast<float *>(pw_filter_get_dsp_buffer(m_outPortR, n_samples));

    if (!out_l || !out_r) return;

    if (!in_l || !in_r) {
        std::memset(out_l, 0, n_samples * sizeof(float));
        std::memset(out_r, 0, n_samples * sizeof(float));
        return;
    }

    // If master switch is bypassed, bit-perfect pass-through with 0 latency
    if (!m_masterEnabled.load()) {
        std::memcpy(out_l, in_l, n_samples * sizeof(float));
        std::memcpy(out_r, in_r, n_samples * sizeof(float));
        return;
    }

    size_t samplesNeeded = n_samples * 2;
    if (m_interleavedBuffer.size() != samplesNeeded) {
        m_interleavedBuffer.resize(samplesNeeded);
    }

    for (uint32_t i = 0; i < n_samples; ++i) {
        m_interleavedBuffer[i * 2]     = in_l[i];
        m_interleavedBuffer[i * 2 + 1] = in_r[i];
    }

    {
        std::lock_guard<std::mutex> lock(m_engineMutex);
        if (m_engine) {
            m_engine->Process(m_interleavedBuffer, n_samples);
        }
    }

    for (uint32_t i = 0; i < n_samples; ++i) {
        out_l[i] = m_interleavedBuffer[i * 2];
        out_r[i] = m_interleavedBuffer[i * 2 + 1];
    }
}

bool AudioEngineLinux::initPipeWireFilter() {
    pw_init(nullptr, nullptr);

    m_threadLoop = pw_thread_loop_new("ViPER-PipeWire", nullptr);
    if (!m_threadLoop) {
        qWarning() << "[ViPER] Failed to create PipeWire thread loop";
        return false;
    }

    static const struct pw_filter_events filterEvents = {
        PW_VERSION_FILTER_EVENTS,
        .process = onProcessCallback,
    };

    m_filter = pw_filter_new_simple(
        pw_thread_loop_get_loop(m_threadLoop),
        "ViPER4Linux",
        pw_properties_new(
            PW_KEY_MEDIA_TYPE, "Audio",
            PW_KEY_MEDIA_CATEGORY, "Filter",
            PW_KEY_MEDIA_ROLE, "DSP",
            PW_KEY_NODE_NAME, "ViPER4Linux",
            PW_KEY_NODE_DESCRIPTION, "ViPER4Linux Audio DSP Filter",
            PW_KEY_NODE_PASSIVE, "false",
            PW_KEY_NODE_AUTOCONNECT, "false",
            nullptr),
        &filterEvents,
        this
    );

    if (!m_filter) {
        qWarning() << "[ViPER] Failed to create PipeWire filter";
        pw_thread_loop_destroy(m_threadLoop);
        m_threadLoop = nullptr;
        return false;
    }

    m_inPortL = pw_filter_add_port(m_filter,
        PW_DIRECTION_INPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "input_FL",
            PW_KEY_AUDIO_CHANNEL, "FL",
            nullptr),
        nullptr, 0);

    m_inPortR = pw_filter_add_port(m_filter,
        PW_DIRECTION_INPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "input_FR",
            PW_KEY_AUDIO_CHANNEL, "FR",
            nullptr),
        nullptr, 0);

    m_outPortL = pw_filter_add_port(m_filter,
        PW_DIRECTION_OUTPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "output_FL",
            PW_KEY_AUDIO_CHANNEL, "FL",
            nullptr),
        nullptr, 0);

    m_outPortR = pw_filter_add_port(m_filter,
        PW_DIRECTION_OUTPUT,
        PW_FILTER_PORT_FLAG_MAP_BUFFERS,
        0,
        pw_properties_new(
            PW_KEY_FORMAT_DSP, "32 bit float mono audio",
            PW_KEY_PORT_NAME, "output_FR",
            PW_KEY_AUDIO_CHANNEL, "FR",
            nullptr),
        nullptr, 0);

    if (pw_filter_connect(m_filter, PW_FILTER_FLAG_RT_PROCESS, nullptr, 0) < 0) {
        qWarning() << "[ViPER] Failed to connect PipeWire filter";
        pw_filter_destroy(m_filter);
        m_filter = nullptr;
        pw_thread_loop_destroy(m_threadLoop);
        m_threadLoop = nullptr;
        return false;
    }

    if (pw_thread_loop_start(m_threadLoop) < 0) {
        qWarning() << "[ViPER] Failed to start PipeWire thread loop";
        pw_filter_destroy(m_filter);
        m_filter = nullptr;
        pw_thread_loop_destroy(m_threadLoop);
        m_threadLoop = nullptr;
        return false;
    }

    qDebug() << "[ViPER] Native PipeWire filter node started successfully";
    return true;
}

void AudioEngineLinux::cleanupPipeWireFilter() {
    if (m_threadLoop) {
        pw_thread_loop_stop(m_threadLoop);
    }
    if (m_filter) {
        pw_filter_destroy(m_filter);
        m_filter = nullptr;
    }
    if (m_threadLoop) {
        pw_thread_loop_destroy(m_threadLoop);
        m_threadLoop = nullptr;
    }
    pw_deinit();
}

QString AudioEngineLinux::getHardwarePlaybackSink() {
    // 1. Check wpctl status for the currently starred default sink
    QProcess procWp;
    procWp.start("wpctl", QStringList() << "status");
    if (procWp.waitForFinished(600)) {
        QString out = QString::fromUtf8(procWp.readAllStandardOutput());
        int sinksIdx = out.indexOf("Sinks:");
        int sourcesIdx = out.indexOf("Sources:");
        if (sinksIdx >= 0 && sourcesIdx > sinksIdx) {
            QString sinksBlock = out.mid(sinksIdx, sourcesIdx - sinksIdx);
            static QRegularExpression reStar(R"(\*\s*(\d+)\.)");
            auto match = reStar.match(sinksBlock);
            if (match.hasMatch()) {
                QString idStr = match.captured(1);
                QProcess procInsp;
                procInsp.start("wpctl", QStringList() << "inspect" << idStr);
                if (procInsp.waitForFinished(600)) {
                    QString inspOut = QString::fromUtf8(procInsp.readAllStandardOutput());
                    static QRegularExpression reNode("node\\.name\\s*=\\s*\"([^\"]+)\"");
                    auto nodeMatch = reNode.match(inspOut);
                    if (nodeMatch.hasMatch()) {
                        QString nodeName = nodeMatch.captured(1).trimmed();
                        if (!nodeName.contains("ViPER", Qt::CaseInsensitive)) {
                            return nodeName;
                        }
                    }
                }
            }
        }
    }

    // 2. Query available playback input ports from pw-link
    QProcess procLink;
    procLink.start("pw-link", QStringList() << "-i");
    if (procLink.waitForFinished(600)) {
        QStringList lines = QString::fromUtf8(procLink.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        QStringList sinks;
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.endsWith(":playback_FL") && !trimmed.contains("ViPER", Qt::CaseInsensitive)) {
                sinks << trimmed.left(trimmed.length() - 12);
            }
        }
        // Priority 1: Bluetooth
        for (const QString &s : sinks) {
            if (s.startsWith("bluez_output")) return s;
        }
        // Priority 2: Built-in Speakers
        for (const QString &s : sinks) {
            if (s.contains("Speaker", Qt::CaseInsensitive) || s.contains("analog", Qt::CaseInsensitive)) {
                return s;
            }
        }
        // Priority 3: Non-HDMI
        for (const QString &s : sinks) {
            if (!s.contains("HDMI", Qt::CaseInsensitive)) return s;
        }
        if (!sinks.isEmpty()) return sinks.first();
    }
    return QString();
}

void AudioEngineLinux::linkFilterOutput(const QString &hwSink) {
    if (hwSink.isEmpty()) return;
    QProcess::execute("pw-link", QStringList() << "ViPER4Linux:output_FL" << (hwSink + ":playback_FL"));
    QProcess::execute("pw-link", QStringList() << "ViPER4Linux:output_FR" << (hwSink + ":playback_FR"));
}

void AudioEngineLinux::disconnectFilterOutput(const QString &hwSink) {
    if (hwSink.isEmpty()) return;
    QProcess::execute("pw-link", QStringList() << "-d" << "ViPER4Linux:output_FL" << (hwSink + ":playback_FL"));
    QProcess::execute("pw-link", QStringList() << "-d" << "ViPER4Linux:output_FR" << (hwSink + ":playback_FR"));
}

void AudioEngineLinux::routeClientStreams(const QString &hwSink) {
    if (hwSink.isEmpty()) return;

    QProcess proc;
    proc.start("pw-link", QStringList() << "-l");
    if (!proc.waitForFinished(600)) return;

    QString output = QString::fromUtf8(proc.readAllStandardOutput());
    QStringList lines = output.split('\n');

    QString currentSource;
    QString targetFL = hwSink + ":playback_FL";
    QString targetFR = hwSink + ":playback_FR";

    for (const QString &rawLine : lines) {
        if (rawLine.isEmpty()) continue;
        if (!rawLine.startsWith(' ') && !rawLine.startsWith('\t')) {
            currentSource = rawLine.trimmed();
        } else if (rawLine.contains("|->")) {
            QString dest = rawLine.mid(rawLine.indexOf("|->") + 3).trimmed();
            if (dest == targetFL) {
                // Ignore ViPER's own filter output
                if (currentSource.startsWith("ViPER4Linux", Qt::CaseInsensitive)) continue;
                if (!currentSource.endsWith(":output_FL")) continue;

                QString clientBase = currentSource.left(currentSource.length() - 10);
                QString clientFL = clientBase + ":output_FL";
                QString clientFR = clientBase + ":output_FR";

                // Connect application stream into ViPER Filter input
                QProcess::execute("pw-link", QStringList() << clientFL << "ViPER4Linux:input_FL");
                QProcess::execute("pw-link", QStringList() << clientFR << "ViPER4Linux:input_FR");

                // Disconnect direct links from application to hardware
                QProcess::execute("pw-link", QStringList() << "-d" << clientFL << targetFL);
                QProcess::execute("pw-link", QStringList() << "-d" << clientFR << targetFR);

                m_routedClients.insert(clientBase);
            }
        }
    }
}

void AudioEngineLinux::restoreClientStreams(const QString &hwSink) {
    if (hwSink.isEmpty()) return;

    QString targetFL = hwSink + ":playback_FL";
    QString targetFR = hwSink + ":playback_FR";

    // Re-check pw-link -l for any clients currently hooked into ViPER
    QProcess proc;
    proc.start("pw-link", QStringList() << "-l");
    if (proc.waitForFinished(600)) {
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QStringList lines = output.split('\n');
        QString currentSource;
        for (const QString &rawLine : lines) {
            if (rawLine.isEmpty()) continue;
            if (!rawLine.startsWith(' ') && !rawLine.startsWith('\t')) {
                currentSource = rawLine.trimmed();
            } else if (rawLine.contains("|->")) {
                QString dest = rawLine.mid(rawLine.indexOf("|->") + 3).trimmed();
                if (dest == "ViPER4Linux:input_FL" && currentSource.endsWith(":output_FL")) {
                    QString clientBase = currentSource.left(currentSource.length() - 10);
                    m_routedClients.insert(clientBase);
                }
            }
        }
    }

    for (const QString &clientBase : m_routedClients) {
        QString clientFL = clientBase + ":output_FL";
        QString clientFR = clientBase + ":output_FR";

        // Reconnect client directly back to hardware sink
        QProcess::execute("pw-link", QStringList() << clientFL << targetFL);
        QProcess::execute("pw-link", QStringList() << clientFR << targetFR);

        // Disconnect from ViPER
        QProcess::execute("pw-link", QStringList() << "-d" << clientFL << "ViPER4Linux:input_FL");
        QProcess::execute("pw-link", QStringList() << "-d" << clientFR << "ViPER4Linux:input_FR");
    }
    m_routedClients.clear();
}

void AudioEngineLinux::updateStreamRouting() {
    if (!m_running.load()) return;

    QString activeSink = getHardwarePlaybackSink();
    if (activeSink.isEmpty()) return;

    // Handle audio output device switch (e.g. bluetooth connected/disconnected)
    if (activeSink != m_currentHardwareSink) {
        if (!m_currentHardwareSink.isEmpty()) {
            disconnectFilterOutput(m_currentHardwareSink);
            restoreClientStreams(m_currentHardwareSink);
        }
        m_currentHardwareSink = activeSink;
        linkFilterOutput(m_currentHardwareSink);
    }

    // Intercept any new streams playing to hardware
    routeClientStreams(m_currentHardwareSink);
}

void AudioEngineLinux::setMasterEnabled(bool enabled) {
    if (m_masterEnabled == enabled) return;
    m_masterEnabled = enabled;

    emit statusChanged(m_masterEnabled.load(), m_masterEnabled.load() ? "Processing Live Audio" : "Bypassed");
}

bool AudioEngineLinux::start() {
    if (m_running.load()) return true;

    if (!initPipeWireFilter()) {
        qWarning() << "[ViPER] Failed to initialize PipeWire filter";
        return false;
    }

    m_running.store(true);
    m_currentHardwareSink = getHardwarePlaybackSink();
    linkFilterOutput(m_currentHardwareSink);

    // Initial stream routing
    routeClientStreams(m_currentHardwareSink);

    // Start background watcher timer (250ms) to intercept new playback streams smoothly
    if (!m_routeTimer) {
        m_routeTimer = new QTimer(this);
        connect(m_routeTimer, &QTimer::timeout, this, &AudioEngineLinux::updateStreamRouting);
        m_routeTimer->start(250);
    }

    emit statusChanged(m_masterEnabled.load(), m_masterEnabled.load() ? "Processing Live Audio" : "Bypassed");
    return true;
}

void AudioEngineLinux::stop() {
    if (!m_running.load()) return;

    if (m_routeTimer) {
        m_routeTimer->stop();
        delete m_routeTimer;
        m_routeTimer = nullptr;
    }

    if (!m_currentHardwareSink.isEmpty()) {
        restoreClientStreams(m_currentHardwareSink);
        disconnectFilterOutput(m_currentHardwareSink);
    }

    cleanupPipeWireFilter();
    m_running.store(false);
    emit statusChanged(false, "Stopped");
}
