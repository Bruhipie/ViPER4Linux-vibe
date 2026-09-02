#pragma once

#include <QQuickPaintedItem>
#include <QVariantList>
#include <QPainter>
#include <QMouseEvent>

class EqGraphItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(QVariantList bands READ bands WRITE setBands NOTIFY bandsChanged)
    Q_PROPERTY(int bandCount READ bandCount WRITE setBandCount NOTIFY bandCountChanged)

public:
    explicit EqGraphItem(QQuickItem *parent = nullptr);

    QVariantList bands() const { return m_bands; }
    void setBands(const QVariantList &bands);

    int bandCount() const { return m_bandCount; }
    void setBandCount(int count);

    void paint(QPainter *painter) override;

signals:
    void bandsChanged();
    void bandCountChanged();
    void bandLevelChanged(int bandIndex, qreal level);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void handleMouse(const QPointF &pos);

    QVariantList m_bands;
    int m_bandCount = 10;
    int m_activeDragBand = -1;
};
