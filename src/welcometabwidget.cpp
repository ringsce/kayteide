#include "welcometabwidget.h"

#include <QEvent>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// A clickable card: bold title over a dimmer hint, like the old HTML links.
QPushButton *makeCard(QWidget *parent, const QString &title, const QString &hint)
{
    auto *card = new QPushButton(parent);
    card->setObjectName(QStringLiteral("welcomeCard"));
    card->setCursor(Qt::PointingHandCursor);
    card->setFixedSize(236, 74);
    card->setAccessibleName(title);
    card->setAccessibleDescription(hint);

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(12, 10, 12, 10);
    lay->setSpacing(2);

    auto *t = new QLabel(title, card);
    t->setObjectName(QStringLiteral("welcomeCardTitle"));
    t->setAlignment(Qt::AlignCenter);
    auto *h = new QLabel(hint, card);
    h->setObjectName(QStringLiteral("welcomeCardHint"));
    h->setAlignment(Qt::AlignCenter);
    for (QLabel *l : { t, h })
        l->setAttribute(Qt::WA_TransparentForMouseEvents);   // clicks go to the card

    lay->addWidget(t);
    lay->addWidget(h);
    return card;
}

} // namespace

WelcomeTabWidget::WelcomeTabWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("welcomePage"));
    setAttribute(Qt::WA_StyledBackground);

    auto *outer = new QVBoxLayout(this);
    outer->addStretch(1);

    auto *logo = new QLabel(this);
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(QIcon(QStringLiteral(":/app-icon")).pixmap(QSize(112, 112)));

    auto *title = new QLabel(tr("KayteIDE"), this);
    title->setObjectName(QStringLiteral("welcomeTitle"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(tr("The Kayte Language IDE"), this);
    subtitle->setObjectName(QStringLiteral("welcomeSubtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    outer->addWidget(logo);
    outer->addSpacing(14);
    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addSpacing(28);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);

    struct Action { QString title, hint; void (WelcomeTabWidget::*signal)(); };
    const Action actions[] = {
        { tr("New File"),               tr("Start an empty document"),    &WelcomeTabWidget::newFileRequested },
        { tr("Open File…"),             tr("Open an existing file"),      &WelcomeTabWidget::openFileRequested },
        { tr("New Project…"),           tr("Create a Kayte project"),     &WelcomeTabWidget::newProjectRequested },
        { tr("Open Project Folder…"),   tr("Browse an existing project"), &WelcomeTabWidget::openProjectRequested },
    };
    for (int i = 0; i < 4; ++i) {
        QPushButton *card = makeCard(this, actions[i].title, actions[i].hint);
        connect(card, &QPushButton::clicked, this, actions[i].signal);
        grid->addWidget(card, i / 2, i % 2);
    }

    auto *gridRow = new QHBoxLayout;
    gridRow->addStretch(1);
    gridRow->addLayout(grid);
    gridRow->addStretch(1);
    outer->addLayout(gridRow);
    outer->addStretch(1);

    applyStyle();
}

void WelcomeTabWidget::changeEvent(QEvent *event)
{
    // Re-derive colours when the system switches light/dark.
    if (event->type() == QEvent::PaletteChange && !m_styling)
        applyStyle();
    QWidget::changeEvent(event);
}

void WelcomeTabWidget::applyStyle()
{
    m_styling = true;
    const QPalette pal = palette();
    const QColor text  = pal.color(QPalette::WindowText);
    const QColor base  = pal.color(QPalette::Base);
    QColor dim = text;   dim.setAlphaF(0.6);
    // Cards sit slightly lifted from the page, in either theme.
    const QColor card   = base.lightness() < 128 ? base.lighter(125) : base.darker(104);
    const QColor hover  = base.lightness() < 128 ? base.lighter(145) : base.darker(110);
    const QColor border = base.lightness() < 128 ? base.lighter(170) : base.darker(125);
    const QColor accent = pal.color(QPalette::Highlight);

    auto rgba = [](const QColor &c) {
        return QStringLiteral("rgba(%1,%2,%3,%4)")
            .arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alphaF(), 0, 'f', 2);
    };

    setStyleSheet(QStringLiteral(R"(
        #welcomePage       { background: %1; }
        #welcomeTitle      { color: %2; font-size: 30px; font-weight: 500; }
        #welcomeSubtitle   { color: %3; font-size: 15px; }
        #welcomeCard       { background: %4; border: 1px solid %5; border-radius: 8px; }
        #welcomeCard:hover { background: %6; border-color: %7; }
        #welcomeCard:focus { border-color: %7; }
        #welcomeCard:pressed { background: %5; }
        #welcomeCardTitle  { color: %2; font-size: 14px; font-weight: 600; background: transparent; }
        #welcomeCardHint   { color: %3; font-size: 12px; background: transparent; }
    )")
        .arg(base.name(), text.name(), rgba(dim), card.name(), border.name(),
             hover.name(), accent.name()));
    m_styling = false;
}
