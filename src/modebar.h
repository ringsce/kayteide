#ifndef MODEBAR_H
#define MODEBAR_H

#include <QWidget>
#include <QVector>
#include <QPointer>

class QAction;

// Vertical mode selector in the style of Qt Creator's left bar: labelled
// mode buttons at the top, a project ("kit") indicator and icon-only
// Run / Debug / Build buttons at the bottom. Icons are drawn as thin
// outlines in the palette's text colour, so they work on light and dark
// themes alike.
class ModeBar : public QWidget
{
    Q_OBJECT
public:
    enum Icon { Home, Edit, Design, Debug, Projects, Git, Help, Settings,
                Run, DebugRun, Build, Kit };

    explicit ModeBar(QWidget *parent = nullptr);

    // Selectable mode (stays highlighted). Returns its index.
    int  addMode(Icon icon, const QString &label, const QString &toolTip);
    // Labelled button that only emits itemActivated (no selection).
    int  addButton(Icon icon, const QString &label, const QString &toolTip);
    // Icon-only button at the bottom that triggers `action`.
    void addBottomAction(QAction *action, Icon icon);

    void setKit(const QString &project, const QString &target);
    QRect kitGlobalRect() const;
    void setCurrentMode(int index);
    int  currentMode() const { return m_current; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

    // Paints one of the outline icons into `r` (exposed for reuse).
    static void drawIcon(QPainter &p, Icon icon, const QRectF &r,
                         const QColor &fg, const QColor &bg);

signals:
    void itemActivated(int index);
    void kitClicked();

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;
    bool event(QEvent *) override;

private:
    struct Item {
        Icon icon;
        QString label;
        QString toolTip;
        bool isMode = false;
        QPointer<QAction> action;   // bottom buttons only
    };

    // Hit-test slots: top items are 0..n-1, then kit, then bottom actions.
    int   slotAt(const QPoint &pos) const;
    QRect slotRect(int slot) const;
    int   kitSlot() const { return int(m_items.size()); }
    int   firstBottomSlot() const { return kitSlot() + 1; }

    QVector<Item> m_items;
    QVector<Item> m_bottom;
    QString m_kitProject;
    QString m_kitTarget;
    int m_current = -1;
    int m_hover   = -1;
    int m_pressed = -1;
};

#endif // MODEBAR_H
