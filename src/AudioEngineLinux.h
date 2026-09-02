#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSet>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>

class ViPER;
class QTimer;
class QProcess;
struct pw_thread_loop;
struct pw_filter;
struct spa_io_position;

class AudioEngineLinux : public QObject {
    Q_OBJECT

public:
    explicit AudioEngineLinux(ViPER *engine, QObject *parent = nullptr);
    ~AudioEngineLinux();

    bool start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setMasterEnabled(bool enabled);
    bool isMasterEnabled() const { return m_masterEnabled.load(); }

    std::mutex& engineMutex() { return m_engineMutex; }

signals:
    void statusChanged(bool active, const QString &statusText);

private slots:
    void updateStreamRouting();

private:
    // PipeWire Filter Callbacks
    static void onProcessCallback(void *userdata, struct spa_io_position *position);
    void processAudio(struct spa_io_position *position);

    bool initPipeWireFilter();
    void cleanupPipeWireFilter();

    // Link Management via pw-link
    QString getHardwarePlaybackSink();
    void linkFilterOutput(const QString &hwSink);
    void routeClientStreams(const QString &hwSink);
    void restoreClientStreams(const QString &hwSink);
    void disconnectFilterOutput(const QString &hwSink);

    ViPER *m_engine;
    std::mutex m_engineMutex;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_masterEnabled{true};

    // PipeWire Native Objects
    struct pw_thread_loop *m_threadLoop = nullptr;
    struct pw_filter *m_filter = nullptr;
    void *m_inPortL = nullptr;
    void *m_inPortR = nullptr;
    void *m_outPortL = nullptr;
    void *m_outPortR = nullptr;

    // Buffer for DSP interleaving
    std::vector<float> m_interleavedBuffer;

    // Active Hardware Sink
    QString m_currentHardwareSink;

    // Track routed client sources (e.g. "Zen", "Spotify")
    QSet<QString> m_routedClients;

    // Timer to watch for newly appearing streams playing to hardware
    QTimer *m_routeTimer = nullptr;
};
