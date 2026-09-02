#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

class ViPER;
struct pa_simple;

class AudioEngineLinux : public QObject {
    Q_OBJECT

public:
    explicit AudioEngineLinux(ViPER *engine, QObject *parent = nullptr);
    ~AudioEngineLinux();

    bool start();
    void stop();
    bool isRunning() const { return m_running.load(); }

    void setMasterEnabled(bool enabled);
    bool isMasterEnabled() const { return m_masterEnabled; }

    void moveActiveStreamsTo(const QString &targetSink);

    std::mutex& engineMutex() { return m_engineMutex; }

signals:
    void statusChanged(bool active, const QString &statusText);

private:
    void runAudioLoop();
    void createVirtualSink();
    void removeVirtualSink();
    void setDefaultSink(const QString &sinkName);
    QString getDefaultSink();
    QString getHardwareOutputSink();

    ViPER *m_engine;
    std::mutex m_engineMutex;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};
    std::atomic<bool> m_masterEnabled{true};

    QString m_virtualSinkModuleId;
    QString m_hardwareOutputSink;

    std::thread m_workerThread;
};
