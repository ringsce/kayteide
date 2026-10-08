#include "modebar.h"

#include <QAction>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QtMath>

namespace {
constexpr int kWidth      = 72;   // bar width
constexpr int kItemHeight = 58;   // labelled mode button
constexpr int kKitHeight  = 76;   // project / target indicator
constexpr int kBtnHeight  = 44;   // icon-only bottom button
constexpr int kIconSize   = 24;
constexpr int kTopMargin  = 6;
constexpr int kAccent     = 3;    // selection bar on the left edge

const QColor kAccentColor(0x5c, 0xb8, 0x3c);   // Qt Creator green
const QColor kRunColor(0x63, 0xc0, 0x4a);

bool isDark(const QPalette &pal)
{
    return pal.color(QPalette::Window).lightness() < 128;
}
} // namespace

ModeBar::ModeBar(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAttribute(Qt::WA_Hover);
}

int ModeBar::addMode(Icon icon, const QString &label, const QString &toolTip)
{
    m_items.append({icon, label, toolTip, true, nullptr});
    updateGeometry();
    update();
    return int(m_items.size()) - 1;
}

int ModeBar::addButton(Icon icon, const QString &label, const QString &toolTip)
{
    m_items.append({icon, label, toolTip, false, nullptr});
    updateGeometry();
    update();
    return int(m_items.size()) - 1;
}

void ModeBar::addBottomAction(QAction *action, Icon icon)
{
    m_bottom.append({icon, action->text(), QString(), false, action});
    connect(action, &QAction::changed, this, qOverload<>(&QWidget::update));
    updateGeometry();
    update();
}

void ModeBar::setKit(const QString &project, const QString &target)
{
    m_kitProject = project;
    m_kitTarget  = target;
    update();
}

QRect ModeBar::kitGlobalRect() const
{
    const QRect r = slotRect(kitSlot());
    return QRect(mapToGlobal(r.topLeft()), r.size());
}

void ModeBar::setCurrentMode(int index)
{
    if (index == m_current) return;
    m_current = index;
    update();
}

QSize ModeBar::sizeHint() const
{
    const int h = kTopMargin + int(m_items.size()) * kItemHeight
                + kKitHeight + int(m_bottom.size()) * kBtnHeight + 8;
    return {kWidth, h};
}

QRect ModeBar::slotRect(int slot) const
{
    if (slot < 0) return {};
    if (slot < kitSlot())
        return {0, kTopMargin + slot * kItemHeight, width(), kItemHeight};

    // Bottom block is anchored to the bottom edge.
    const int bottomTop = height() - int(m_bottom.size()) * kBtnHeight - 6;
    if (slot == kitSlot())
        return {0, bottomTop - kKitHeight, width(), kKitHeight};

    const int i = slot - firstBottomSlot();
    if (i >= m_bottom.size()) return {};
    return {0, bottomTop + i * kBtnHeight, width(), kBtnHeight};
}

int ModeBar::slotAt(const QPoint &pos) const
{
    const int total = firstBottomSlot() + int(m_bottom.size());
    for (int s = 0; s < total; ++s)
        if (slotRect(s).contains(pos)) return s;
    return -1;
}

// ─── Painting ────────────────────────────────────────────────────────────────

void ModeBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QPalette pal = palette();
    const bool dark    = isDark(pal);
    const QColor win   = pal.color(QPalette::Window);
    const QColor bg    = dark ? win.darker(118) : win.darker(106);
    const QColor fg    = pal.color(QPalette::WindowText);
    const QColor dim   = pal.color(QPalette::Disabled, QPalette::WindowText);
    const QColor sel   = dark ? QColor(255, 255, 255, 26) : QColor(0, 0, 0, 22);
    const QColor hov   = dark ? QColor(255, 255, 255, 14) : QColor(0, 0, 0, 12);
    const QColor edge  = dark ? win.darker(150) : win.darker(125);

    p.fillRect(rect(), bg);
    p.setPen(edge);
    p.drawLine(QPointF(width() - 0.5, 0), QPointF(width() - 0.5, height()));

    QFont labelFont = font();
    labelFont.setPixelSize(11);
    labelFont.setWeight(QFont::DemiBold);

    // ── Top: labelled modes / buttons ────────────────────────────────────────
    for (int i = 0; i < m_items.size(); ++i) {
        const Item &it = m_items[i];
        const QRect r = slotRect(i);
        const bool selected = it.isMode && i == m_current;

        if (selected) {
            p.fillRect(r.adjusted(0, 0, -1, 0), sel);
            p.fillRect(QRect(r.left(), r.top(), kAccent, r.height()), kAccentColor);
        } else if (i == m_hover) {
            p.fillRect(r.adjusted(0, 0, -1, 0), m_pressed == i ? sel : hov);
        }

        const QRectF iconRect(r.center().x() - kIconSize / 2.0, r.top() + 8,
                              kIconSize, kIconSize);
        drawIcon(p, it.icon, iconRect, fg, bg);

        p.setFont(labelFont);
        p.setPen(fg);
        const QRect textRect(r.left() + 2, r.top() + 36, r.width() - 4, 16);
        p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop,
                   QFontMetrics(labelFont).elidedText(it.label, Qt::ElideRight,
                                                      textRect.width()));
    }

    // ── Kit: project name, monitor icon, target ──────────────────────────────
    {
        const QRect r = slotRect(kitSlot());
        if (m_hover == kitSlot())
            p.fillRect(r.adjusted(0, 0, -1, 0), m_pressed == kitSlot() ? sel : hov);

        QFont small = labelFont;
        small.setPixelSize(10);
        small.setWeight(QFont::Normal);
        p.setFont(small);
        p.setPen(fg);
        const QFontMetrics fm(small);
        p.drawText(QRect(r.left() + 3, r.top() + 4, r.width() - 6, 14),
                   Qt::AlignHCenter | Qt::AlignTop,
                   fm.elidedText(m_kitProject, Qt::ElideMiddle, r.width() - 6));

        const QRectF iconRect(r.center().x() - kIconSize / 2.0, r.top() + 22,
                              kIconSize, kIconSize);
        drawIcon(p, Kit, iconRect, fg, bg);

        // Small arrow hinting at a chooser, as in Qt Creator.
        QPainterPath arrow;
        const QPointF a(iconRect.right() + 6, iconRect.center().y() + 2);
        arrow.moveTo(a + QPointF(0, -3));
        arrow.lineTo(a + QPointF(3, 0));
        arrow.lineTo(a + QPointF(0, 3));
        arrow.closeSubpath();
        p.fillPath(arrow, fg);

        p.setFont(labelFont);
        p.drawText(QRect(r.left() + 2, r.top() + 52, r.width() - 4, 16),
                   Qt::AlignHCenter | Qt::AlignTop,
                   QFontMetrics(labelFont).elidedText(m_kitTarget, Qt::ElideRight,
                                                      r.width() - 4));
    }

    // ── Bottom: Run / Debug / Build ──────────────────────────────────────────
    for (int i = 0; i < m_bottom.size(); ++i) {
        const Item &it = m_bottom[i];
        const int slot = firstBottomSlot() + i;
        const QRect r = slotRect(slot);
        const bool enabled = it.action && it.action->isEnabled();

        if (enabled && slot == m_hover)
            p.fillRect(r.adjusted(0, 0, -1, 0), m_pressed == slot ? sel : hov);

        const QRectF iconRect(r.center().x() - kIconSize / 2.0,
                              r.center().y() - kIconSize / 2.0, kIconSize, kIconSize);
        drawIcon(p, it.icon, iconRect, enabled ? fg : dim, bg);
        if (!enabled) {
            // Wash out the coloured run icons too.
            p.fillRect(iconRect.adjusted(-2, -2, 2, 2),
                       QColor(bg.red(), bg.green(), bg.blue(), 120));
        }
    }
}

void ModeBar::drawIcon(QPainter &p, Icon icon, const QRectF &r,
                       const QColor &fg, const QColor &bg)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(r.topLeft());
    p.scale(r.width() / 24.0, r.height() / 24.0);

    QPen pen(fg, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (icon) {
    case Home: {
        QPainterPath roof;
        roof.moveTo(3.5, 11.5);
        roof.lineTo(12, 4);
        roof.lineTo(20.5, 11.5);
        p.drawPath(roof);
        QPainterPath body;
        body.moveTo(6, 10);
        body.lineTo(6, 20);
        body.lineTo(10, 20);
        body.lineTo(10, 14.5);
        body.lineTo(14, 14.5);
        body.lineTo(14, 20);
        body.lineTo(18, 20);
        body.lineTo(18, 10);
        p.drawPath(body);
        break;
    }
    case Edit: {
        QPen thick = pen;
        thick.setWidthF(2.0);
        thick.setCapStyle(Qt::FlatCap);
        p.setPen(thick);
        p.drawLine(QPointF(5, 5),  QPointF(19, 5));
        p.drawLine(QPointF(5, 9.5),  QPointF(19, 9.5));
        p.drawLine(QPointF(5, 14), QPointF(19, 14));
        p.drawLine(QPointF(5, 18.5), QPointF(14, 18.5));
        break;
    }
    case Design: {
        // Fountain-pen nib, tilted.
        p.translate(12, 12);
        p.rotate(40);
        QPainterPath nib;
        nib.moveTo(0, -9);
        nib.lineTo(5, -2);
        nib.lineTo(2.5, 7);
        nib.lineTo(-2.5, 7);
        nib.lineTo(-5, -2);
        nib.closeSubpath();
        p.drawPath(nib);
        p.drawLine(QPointF(0, -9), QPointF(0, -1));
        p.drawEllipse(QPointF(0, 0.6), 1.3, 1.3);
        p.drawLine(QPointF(-2.5, 9.5), QPointF(2.5, 9.5));
        break;
    }
    case Debug: {
        p.drawEllipse(QPointF(12, 14.5), 5, 6);
        QPainterPath head;
        head.moveTo(8.8, 9.8);
        head.cubicTo(9, 5.5, 15, 5.5, 15.2, 9.8);
        p.drawPath(head);
        p.drawLine(QPointF(12, 10.5), QPointF(12, 20.5));
        p.drawLine(QPointF(7, 12), QPointF(3.5, 10));
        p.drawLine(QPointF(17, 12), QPointF(20.5, 10));
        p.drawLine(QPointF(7, 15), QPointF(3, 15));
        p.drawLine(QPointF(17, 15), QPointF(21, 15));
        p.drawLine(QPointF(7.5, 18.5), QPointF(4, 21));
        p.drawLine(QPointF(16.5, 18.5), QPointF(20, 21));
        p.drawLine(QPointF(10, 6.5), QPointF(8.5, 4));
        p.drawLine(QPointF(14, 6.5), QPointF(15.5, 4));
        break;
    }
    case Projects: {
        // Wrench, as Qt Creator uses for project settings.
        p.translate(12, 12);
        p.rotate(45);
        QPainterPath w;
        w.moveTo(-1.8, 1.5);                                  // handle, left side
        w.lineTo(-1.8, 9.5);
        w.arcTo(QRectF(-1.8, 7.7, 3.6, 3.6), 180, 180);       // rounded end
        w.lineTo(1.8, 1.5);
        w.cubicTo(6.5, 0, 6.5, -7, 3, -9);                    // head, right jaw
        w.lineTo(2, -5);
        w.lineTo(-2, -5);
        w.lineTo(-3, -9);                                     // left jaw
        w.cubicTo(-6.5, -7, -6.5, 0, -1.8, 1.5);
        p.drawPath(w);
        break;
    }
    case Git: {
        p.drawEllipse(QPointF(7, 5), 2.2, 2.2);
        p.drawEllipse(QPointF(7, 19), 2.2, 2.2);
        p.drawEllipse(QPointF(17, 7), 2.2, 2.2);
        p.drawLine(QPointF(7, 7.2), QPointF(7, 16.8));
        QPainterPath branch;
        branch.moveTo(17, 9.2);
        branch.cubicTo(17, 14, 7, 12, 7, 16.8);
        p.drawPath(branch);
        break;
    }
    case Help: {
        p.drawEllipse(QPointF(12, 12), 9, 9);
        QPainterPath q;
        q.moveTo(9.2, 9.5);
        q.cubicTo(9.2, 5.8, 14.8, 5.8, 14.8, 9.3);
        q.cubicTo(14.8, 11.6, 12, 11.6, 12, 14.2);
        p.drawPath(q);
        p.setPen(Qt::NoPen);
        p.setBrush(fg);
        p.drawEllipse(QPointF(12, 17.2), 1.1, 1.1);
        break;
    }
    case Settings: {
        QPen tooth = pen;
        tooth.setWidthF(3.0);
        tooth.setCapStyle(Qt::FlatCap);
        p.setPen(tooth);
        for (int i = 0; i < 8; ++i) {
            const qreal a = qDegreesToRadians(i * 45.0);
            p.drawLine(QPointF(12 + 6.5 * qCos(a), 12 + 6.5 * qSin(a)),
                       QPointF(12 + 9.5 * qCos(a), 12 + 9.5 * qSin(a)));
        }
        p.setPen(pen);
        p.drawEllipse(QPointF(12, 12), 6.5, 6.5);
        p.drawEllipse(QPointF(12, 12), 2.6, 2.6);
        break;
    }
    case Run: {
        p.setPen(QPen(kRunColor, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tri;
        tri.moveTo(6.5, 4);
        tri.lineTo(19, 12);
        tri.lineTo(6.5, 20);
        tri.closeSubpath();
        p.drawPath(tri);
        break;
    }
    case DebugRun: {
        p.setPen(QPen(kRunColor, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath tri;
        tri.moveTo(4.5, 3);
        tri.lineTo(16, 10.5);
        tri.lineTo(4.5, 18);
        tri.closeSubpath();
        p.drawPath(tri);
        // Small bug badge, bottom-right, punched out of the background.
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(QPointF(17.5, 17.5), 6, 6);
        p.restore();
        p.save();
        drawIcon(p, Debug, QRectF(r.left() + r.width() * 0.5, r.top() + r.height() * 0.5,
                                  r.width() * 0.5, r.height() * 0.5), fg, bg);
        break;
    }
    case Build: {
        p.translate(12, 12);
        p.rotate(-45);
        QPen thick = pen;
        thick.setWidthF(2.2);
        p.setPen(thick);
        p.drawLine(QPointF(0, -3), QPointF(0, 10));          // handle
        p.setPen(pen);
        QPainterPath head;
        head.moveTo(-7, -8);
        head.lineTo(5, -8);
        head.lineTo(7, -6);
        head.lineTo(7, -3.5);
        head.lineTo(-7, -3.5);
        head.closeSubpath();
        p.drawPath(head);
        break;
    }
    case Kit: {
        p.drawRoundedRect(QRectF(3, 4, 18, 12.5), 1.5, 1.5);
        p.drawLine(QPointF(12, 16.5), QPointF(12, 19.5));
        p.drawLine(QPointF(8, 20), QPointF(16, 20));
        // Green "ready" dot.
        p.setPen(QPen(bg, 1.5));
        p.setBrush(kAccentColor);
        p.drawEllipse(QPointF(19, 18), 3.2, 3.2);
        break;
    }
    }
    p.restore();
}

// ─── Interaction ─────────────────────────────────────────────────────────────

void ModeBar::mouseMoveEvent(QMouseEvent *e)
{
    const int s = slotAt(e->position().toPoint());
    if (s != m_hover) {
        m_hover = s;
        update();
    }
}

void ModeBar::leaveEvent(QEvent *)
{
    m_hover = -1;
    m_pressed = -1;
    update();
}

void ModeBar::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    m_pressed = slotAt(e->position().toPoint());
    update();
}

void ModeBar::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    const int s = slotAt(e->position().toPoint());
    const int pressed = m_pressed;
    m_pressed = -1;
    update();
    if (s < 0 || s != pressed) return;

    if (s < kitSlot()) {
        if (m_items[s].isMode) setCurrentMode(s);
        emit itemActivated(s);
    } else if (s == kitSlot()) {
        emit kitClicked();
    } else {
        const Item &it = m_bottom[s - firstBottomSlot()];
        if (it.action && it.action->isEnabled())
            it.action->trigger();
    }
}

bool ModeBar::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        auto *he = static_cast<QHelpEvent *>(e);
        const int s = slotAt(he->pos());
        QString tip;
        if (s >= 0 && s < kitSlot()) {
            tip = m_items[s].toolTip;
        } else if (s == kitSlot()) {
            tip = tr("Project: %1\nTarget: %2").arg(m_kitProject, m_kitTarget);
        } else if (s > kitSlot()) {
            const Item &it = m_bottom[s - firstBottomSlot()];
            if (it.action) {
                tip = it.action->toolTip();
                const QKeySequence ks = it.action->shortcut();
                if (!ks.isEmpty())
                    tip += QStringLiteral("  (%1)").arg(ks.toString(QKeySequence::NativeText));
            }
        }
        if (tip.isEmpty()) {
            QToolTip::hideText();
            e->ignore();
        } else {
            QToolTip::showText(he->globalPos(), tip, this, slotRect(s));
        }
        return true;
    }
    return QWidget::event(e);
}
