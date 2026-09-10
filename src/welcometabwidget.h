#ifndef WELCOMETABWIDGET_H
#define WELCOMETABWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QWebEngineView;
QT_END_NAMESPACE

// ─── WelcomeTabWidget ──────────────────────────────────────────────────────
// Start-page tab shown when KayteIDE opens. Renders a small local HTML page
// inside a QWebEngineView (Qt WebEngine – the "qtwebbrowser" module) with
// quick-action links. Links use the custom "kayteide:" URL scheme so the
// embedded page can trigger native actions (New File, Open Project, ...)
// without a JS↔C++ bridge; navigation to that scheme is intercepted and
// turned into one of the signals below instead of actually navigating.
class WelcomeTabWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeTabWidget(QWidget *parent = nullptr);

signals:
    void newFileRequested();
    void openFileRequested();
    void newProjectRequested();
    void openProjectRequested();

    // Internal: emitted by the page when a "kayteide:" link is clicked;
    // translated into one of the signals above by handleAction().
    void actionTriggered(const QString &action);

private slots:
    void handleAction(const QString &action);

private:
    QWebEngineView *m_view { nullptr };
};

#endif // WELCOMETABWIDGET_H
