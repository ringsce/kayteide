#include "welcometabwidget.h"

#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QUrl>

namespace {

// Intercepts clicks on the welcome page's "kayteide:" action links so they
// can be routed back to native Qt slots instead of being navigated to.
class WelcomeEnginePage : public QWebEnginePage
{
public:
    explicit WelcomeEnginePage(WelcomeTabWidget *owner, QObject *parent = nullptr)
        : QWebEnginePage(parent), m_owner(owner) {}

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        Q_UNUSED(type);
        Q_UNUSED(isMainFrame);
        if (url.scheme() == QLatin1String("kayteide")) {
            emit m_owner->actionTriggered(url.host());
            return false;
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

private:
    WelcomeTabWidget *m_owner;
};

QString welcomeHtml()
{
    return QStringLiteral(R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
    body {
        margin: 0;
        background: #1e1f22;
        color: #d6d6d6;
        font-family: -apple-system, "Segoe UI", sans-serif;
        display: flex;
        align-items: center;
        justify-content: center;
        height: 100vh;
    }
    .card {
        text-align: center;
        max-width: 520px;
    }
    img.logo { width: 96px; height: 96px; margin-bottom: 16px; }
    h1 { font-weight: 500; margin: 0 0 4px 0; }
    p.subtitle { color: #9a9a9a; margin: 0 0 32px 0; }
    .actions {
        display: flex;
        flex-wrap: wrap;
        gap: 12px;
        justify-content: center;
    }
    a.action {
        display: block;
        width: 180px;
        padding: 14px 10px;
        border-radius: 8px;
        background: #2b2d30;
        border: 1px solid #3a3c3f;
        color: #d6d6d6;
        text-decoration: none;
        font-size: 14px;
        transition: background 0.15s ease, border-color 0.15s ease;
    }
    a.action:hover { background: #34363a; border-color: #5a9bff; }
    a.action .title { font-weight: 600; display: block; margin-bottom: 2px; }
    a.action .hint { color: #9a9a9a; font-size: 12px; }
</style>
</head>
<body>
    <div class="card">
        <img class="logo" src="qrc:/app-icon">
        <h1>KayteIDE</h1>
        <p class="subtitle">The Kayte Language IDE</p>
        <div class="actions">
            <a class="action" href="kayteide://new-file">
                <span class="title">New File</span>
                <span class="hint">Start an empty document</span>
            </a>
            <a class="action" href="kayteide://open-file">
                <span class="title">Open File...</span>
                <span class="hint">Open an existing file</span>
            </a>
            <a class="action" href="kayteide://new-project">
                <span class="title">New Project...</span>
                <span class="hint">Create a Kayte project</span>
            </a>
            <a class="action" href="kayteide://open-project">
                <span class="title">Open Project Folder...</span>
                <span class="hint">Browse an existing project</span>
            </a>
        </div>
    </div>
</body>
</html>
)HTML");
}

} // namespace

WelcomeTabWidget::WelcomeTabWidget(QWidget *parent)
    : QWidget(parent)
    , m_view(new QWebEngineView(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    m_view->setPage(new WelcomeEnginePage(this, m_view));
    m_view->setHtml(welcomeHtml(), QUrl(QStringLiteral("qrc:/")));

    connect(this, &WelcomeTabWidget::actionTriggered, this, &WelcomeTabWidget::handleAction);
}

void WelcomeTabWidget::handleAction(const QString &action)
{
    if (action == QLatin1String("new-file"))
        emit newFileRequested();
    else if (action == QLatin1String("open-file"))
        emit openFileRequested();
    else if (action == QLatin1String("new-project"))
        emit newProjectRequested();
    else if (action == QLatin1String("open-project"))
        emit openProjectRequested();
}
