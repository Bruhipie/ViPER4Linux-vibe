#include "EqGraphItem.h"
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <cmath>

static const QStringList kFreqLabels10 = {
    "31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"
};

EqGraphItem::EqGraphItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAntialiasing(true);
    for (int i = 0; i < 10; ++i) {
        m_bands.append(0.0);
    }
}

void EqGraphItem::setBands(const QVariantList &bands) {
    if (m_bands != bands) {
        m_bands = bands;
        emit bandsChanged();
        update();
    }
}

void EqGraphItem::setBandCount(int count) {
    if (m_bandCount != count && count > 1) {
        m_bandCount = count;
        emit bandCountChanged();
        update();
    }
}

void EqGraphItem::mousePressEvent(QMouseEvent *event) {
    handleMouse(event->position());
}

void EqGraphItem::mouseMoveEvent(QMouseEvent *event) {
    if (m_activeDragBand >= 0) {
        handleMouse(event->position());
    }
}

void EqGraphItem::mouseReleaseEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    m_activeDragBand = -1;
}

void EqGraphItem::handleMouse(const QPointF &pos) {
    qreal padLeft = 32.0;
    qreal padRight = 16.0;
    qreal padTop = 14.0;
    qreal padBottom = 20.0;
    qreal w = width() - padLeft - padRight;
    qreal h = height() - padTop - padBottom;
    if (w <= 0 || h <= 0 || m_bands.isEmpty()) return;

    int n = m_bands.size();

    if (m_activeDragBand < 0) {
        // Find closest band horizontally
        qreal minDx = 9999.0;
        int closest = 0;
        for (int i = 0; i < n; ++i) {
            qreal bx = padLeft + (w / (n - 1)) * i;
            qreal dx = std::abs(pos.x() - bx);
            if (dx < minDx) {
                minDx = dx;
                closest = i;
            }
        }
        m_activeDragBand = closest;
    }

    // Map pos.y to dB (-12 to +12)
    qreal normY = (pos.y() - padTop) / h;
    normY = std::max(0.0, std::min(1.0, normY));
    qreal db = 12.0 - normY * 24.0; // 0 = +12, 1 = -12
    db = std::round(db * 10.0) / 10.0; // Round to 0.1dB

    if (m_activeDragBand >= 0 && m_activeDragBand < n) {
        m_bands[m_activeDragBand] = db;
        emit bandLevelChanged(m_activeDragBand, db);
        emit bandsChanged();
        update();
    }
}

void EqGraphItem::paint(QPainter *painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF rect = contentsBoundingRect();
    qreal padLeft = 32.0;
    qreal padRight = 16.0;
    qreal padTop = 14.0;
    qreal padBottom = 20.0;
    qreal gw = rect.width() - padLeft - padRight;
    qreal gh = rect.height() - padTop - padBottom;

    // Background Card
    painter->setPen(QPen(QColor("#242236"), 1));
    painter->setBrush(QColor("#0F0E18"));
    painter->drawRoundedRect(rect.adjusted(1, 1, -1, -1), 8, 8);

    if (gw <= 0 || gh <= 0) return;

    // dB Grid Lines
    QFont font = painter->font();
    font.setPixelSize(9);
    font.setStyleHint(QFont::Monospace);
    painter->setFont(font);

    const qreal dbVals[5] = { 12.0, 6.0, 0.0, -6.0, -12.0 };
    for (int i = 0; i < 5; ++i) {
        qreal y = padTop + (gh / 4.0) * i;
        if (dbVals[i] == 0.0) {
            painter->setPen(QPen(QColor(255, 255, 255, 50), 1, Qt::SolidLine));
        } else {
            painter->setPen(QPen(QColor(255, 255, 255, 20), 1, Qt::DashLine));
        }
        painter->drawLine(QPointF(padLeft, y), QPointF(padLeft + gw, y));

        QString label = dbVals[i] > 0 ? QString("+%1").arg((int)dbVals[i]) : QString::number((int)dbVals[i]);
        painter->setPen(QColor(200, 200, 220, 120));
        painter->drawText(QRectF(2, y - 6, padLeft - 6, 12), Qt::AlignRight | Qt::AlignVCenter, label);
    }

    int n = m_bands.size();
    if (n < 2) return;

    // Frequency Grid & Labels
    for (int i = 0; i < n; ++i) {
        qreal x = padLeft + (gw / (n - 1)) * i;
        painter->setPen(QPen(QColor(255, 255, 255, 15), 1, Qt::DotLine));
        painter->drawLine(QPointF(x, padTop), QPointF(x, padTop + gh));

        if (n == 10 && i < kFreqLabels10.size()) {
            painter->setPen(QColor(160, 160, 180, 140));
            painter->drawText(QRectF(x - 15, padTop + gh + 3, 30, 14), Qt::AlignHCenter | Qt::AlignTop, kFreqLabels10[i]);
        }
    }

    // Compute curve points
    QVector<QPointF> points(n);
    for (int i = 0; i < n; ++i) {
        qreal x = padLeft + (gw / (n - 1)) * i;
        qreal db = m_bands[i].toReal();
        db = std::max(-12.0, std::min(12.0, db));
        qreal y = padTop + gh * (1.0 - (db + 12.0) / 24.0);
        points[i] = QPointF(x, y);
    }

    // Build smooth cubic path
    QPainterPath path;
    path.moveTo(points[0]);
    for (int i = 0; i < n - 1; ++i) {
        QPointF p0 = (i > 0) ? points[i - 1] : points[i];
        QPointF p1 = points[i];
        QPointF p2 = points[i + 1];
        QPointF p3 = (i + 2 < n) ? points[i + 2] : p2;

        qreal c1x = p1.x() + (p2.x() - p0.x()) / 6.0;
        qreal c1y = p1.y() + (p2.y() - p0.y()) / 6.0;
        qreal c2x = p2.x() - (p3.x() - p1.x()) / 6.0;
        qreal c2y = p2.y() - (p3.y() - p1.y()) / 6.0;

        path.cubicTo(c1x, c1y, c2x, c2y, p2.x(), p2.y());
    }

    // Gradient Fill
    QPainterPath fillPath = path;
    fillPath.lineTo(padLeft + gw, padTop + gh);
    fillPath.lineTo(padLeft, padTop + gh);
    fillPath.closeSubpath();

    QLinearGradient grad(0, padTop, 0, padTop + gh);
    grad.setColorAt(0.0, QColor(168, 140, 250, 90));
    grad.setColorAt(1.0, QColor(168, 140, 250, 0));
    painter->fillPath(fillPath, grad);

    // Stroke curve
    painter->setPen(QPen(QColor("#A88CFA"), 2.2));
    painter->drawPath(path);

    // Draw control nodes
    for (int i = 0; i < n; ++i) {
        bool isActive = (m_activeDragBand == i);
        qreal r = isActive ? 5.0 : 3.5;
        painter->setPen(QPen(isActive ? QColor("#FFFFFF") : QColor("#C2ABFF"), 1.5));
        painter->setBrush(isActive ? QColor("#A88CFA") : QColor("#1C1C20"));
        painter->drawEllipse(points[i], r, r);
    }
}
