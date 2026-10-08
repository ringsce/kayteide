#ifndef WELCOMETABWIDGET_H
#define WELCOMETABWIDGET_H

#include <QWidget>

// ─── WelcomeTabWidget ──────────────────────────────────────────────────────
// Start-page tab shown when KayteIDE opens: logo, title and four quick-action
// cards. Plain Qt widgets (no web engine), so it costs a few hundred KB of
// memory instead of a Chromium renderer process, and follows the app palette
// in light and dark mode.
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

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyStyle();
    bool m_styling { false };   // setStyleSheet() can itself raise PaletteChange
};

#endif // WELCOMETABWIDGET_H
