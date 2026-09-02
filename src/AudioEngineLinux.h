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
class QProcess;
class QTimer;

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

    // Volume & Mute Synchronization
    QString getSinkVolume(const QString &sinkName);
    bool getSinkMute(const QString &sinkName);
    void setSinkVolume(const QString &sinkName, const QString &volume);
    void setSinkMute(const QString &sinkName, bool muted);
    void syncVolumes();

signals:
    void statusChanged(bool active, const QString &statusText);

private slots:
    void handleSubscriptionOutput();

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
    QString m_outputSinkModuleId;   // ViPER4Linux_Out sink
    QString m_loopbackModuleId;     // module-loopback Out.monitor → hardware
    QString m_hardwareOutputSink;

    std::thread m_workerThread;

    QProcess *m_subscribeProc = nullptr;
    QTimer *m_volumeSyncTimer = nullptr;
    QString m_lastSyncedVolume;
    bool m_lastSyncedMute = false;
    std::atomic<bool> m_isSyncing{false};
};
