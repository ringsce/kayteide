// widgetpalettedock.cpp – KayteIDE
#include "widgetpalettedock.h"

#if __has_include(<IconsFontAwesome6.h>)
#  include <IconsFontAwesome6.h>
#else
// Minimal FA6 fallback glyphs (UTF-8 encoded)
#  define ICON_FA_SQUARE_CHECK    "\xef\x85\x8c"
#  define ICON_FA_KEYBOARD        "\xef\x84\x9c"
#  define ICON_FA_IMAGE           "\xef\x80\xbe"
#  define ICON_FA_TABLE_CELLS     "\xef\x80\x8a"
#  define ICON_FA_OBJECT_GROUP    "\xef\x89\x87"
#  define ICON_FA_BARS            "\xef\x83\x89"
#  define ICON_FA_MAGNIFYING_GLASS "\xef\x80\x82"
#  define ICON_FA_SLIDERS         "\xef\x87\x9e"
#endif

#include <QApplication>
#include <QPixmap>
#include <QPainter>

// ─────────────────────────────────────────────────────────────────────────────
// PaletteListWidget – drag-enabled
// ─────────────────────────────────────────────────────────────────────────────

PaletteListWidget::PaletteListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setIconSize(QSize(20, 20));
    setSpacing(2);
    setStyleSheet(QStringLiteral(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 4px 6px; border-radius: 4px; }"
        "QListWidget::item:hover { background: rgba(100,140,255,0.15); }"
        "QListWidget::item:selected { background: rgba(100,140,255,0.35); }"
    ));
}

void PaletteListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QListWidget::mousePressEvent(event);
}

void PaletteListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) return;
    if ((event->pos() - m_dragStartPos).manhattanLength()
            < QApplication::startDragDistance()) return;

    QListWidgetItem *item = currentItem();
    if (!item) return;

    const QString widgetType = item->data(Qt::UserRole).toString();
    if (widgetType.isEmpty()) return;

    auto *drag     = new QDrag(this);
    auto *mimeData = new QMimeData;
    mimeData->setText(widgetType); // canvas reads this
    mimeData->setData(QStringLiteral("application/x-kayteide-widget"), widgetType.toUtf8());

    // Thumbnail pixmap
    QPixmap pix(80, 28);
    pix.fill(QColor(80, 120, 220, 200));
    QPainter pm(&pix);
    pm.setPen(Qt::white);
    pm.setFont(QFont(QStringLiteral("sans-serif"), 9));
    pm.drawText(pix.rect(), Qt::AlignCenter, item->text());
    pm.end();

    drag->setMimeData(mimeData);
    drag->setPixmap(pix);
    drag->setHotSpot({ pix.width() / 2, pix.height() / 2 });
    drag->exec(Qt::CopyAction);
}

// ─────────────────────────────────────────────────────────────────────────────
// WidgetPaletteDock
// ─────────────────────────────────────────────────────────────────────────────

WidgetPaletteDock::WidgetPaletteDock(const QFont &faFont, QWidget *parent)
    : QDockWidget(tr("Widget Palette"), parent)
    , m_faFont(faFont)
{
    setObjectName(QStringLiteral("WidgetPaletteDock"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetMovable   |
                QDockWidget::DockWidgetFloatable  |
                QDockWidget::DockWidgetClosable);
    setMinimumWidth(180);
    setMaximumWidth(300);

    // ── Custom title bar ──────────────────────────────────────────────────────
    auto *titleBar = new QWidget(this);
    auto *titleLay = new QHBoxLayout(titleBar);
    titleLay->setContentsMargins(6, 2, 6, 2);
    auto *iconLbl = new QLabel(QString::fromUtf8(ICON_FA_OBJECT_GROUP), titleBar);
    iconLbl->setFont(m_faFont);
    auto *txtLbl  = new QLabel(tr("  Widget Palette"), titleBar);
    QFont bold = txtLbl->font(); bold.setBold(true); txtLbl->setFont(bold);
    titleLay->addWidget(iconLbl);
    titleLay->addWidget(txtLbl);
    titleLay->addStretch();
    setTitleBarWidget(titleBar);

    // ── Body ──────────────────────────────────────────────────────────────────
    auto *body = new QWidget(this);
    auto *lay  = new QVBoxLayout(body);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(4);

    // Search bar
    m_searchEdit = new QLineEdit(body);
    m_searchEdit->setPlaceholderText(
        QString::fromUtf8(ICON_FA_MAGNIFYING_GLASS) + tr("  Filter widgets…"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { border-radius: 4px; padding: 4px 8px; }"
    ));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &WidgetPaletteDock::onSearchChanged);
    lay->addWidget(m_searchEdit);

    // Toolbox
    m_toolBox = new QToolBox(body);
    m_toolBox->setStyleSheet(QStringLiteral(
        "QToolBox::tab { background: palette(button); border-radius: 4px; "
        "                padding: 4px; font-weight: bold; }"
        "QToolBox::tab:selected { background: palette(highlight); color: palette(highlighted-text); }"
    ));
    lay->addWidget(m_toolBox, 1);

    setWidget(body);
    buildPalette();
}

PaletteListWidget *WidgetPaletteDock::addCategory(
        const QString &title,
        const char    *titleIcon,
        const QList<PaletteItem> &items)
{
    auto *list = new PaletteListWidget(m_toolBox);

    for (const PaletteItem &pi : items) {
        auto *item = new QListWidgetItem(list);

        // FA glyph as left decoration
        QString display;
        if (pi.icon && *pi.icon) {
            QLabel lbl;
            lbl.setFont(m_faFont);
            display = QString::fromUtf8(pi.icon) + QStringLiteral("  ") + pi.label;
        } else {
            display = pi.label;
        }
        item->setText(display);
        item->setData(Qt::UserRole, pi.type);
        item->setToolTip(pi.type);
    }

    QString tabLabel = QString::fromUtf8(titleIcon) +
                       QStringLiteral("  ") + title;
    m_toolBox->addItem(list, tabLabel);
    return list;
}

void WidgetPaletteDock::buildPalette()
{
    // ── Buttons ───────────────────────────────────────────────────────────────
    addCategory(tr("Buttons"), ICON_FA_SQUARE_CHECK, {
        { tr("Push Button"),    QStringLiteral("QPushButton"),   ICON_FA_SQUARE_CHECK },
        { tr("Tool Button"),    QStringLiteral("QToolButton"),   ICON_FA_SQUARE_CHECK },
        { tr("Radio Button"),   QStringLiteral("QRadioButton"),  ICON_FA_SQUARE_CHECK },
        { tr("Check Box"),      QStringLiteral("QCheckBox"),     ICON_FA_SQUARE_CHECK },
        { tr("Command Link"),   QStringLiteral("QCommandLinkButton"), ICON_FA_SQUARE_CHECK },
        { tr("Dialog Buttons"), QStringLiteral("QDialogButtonBox"),   ICON_FA_SQUARE_CHECK },
    });

    // ── Input ────────────────────────────────────────────────────────────────
    addCategory(tr("Input"), ICON_FA_KEYBOARD, {
        { tr("Line Edit"),      QStringLiteral("QLineEdit"),     ICON_FA_KEYBOARD },
        { tr("Text Edit"),      QStringLiteral("QTextEdit"),     ICON_FA_KEYBOARD },
        { tr("Plain Text Edit"),QStringLiteral("QPlainTextEdit"),ICON_FA_KEYBOARD },
        { tr("Spin Box"),       QStringLiteral("QSpinBox"),      ICON_FA_KEYBOARD },
        { tr("Double Spin Box"),QStringLiteral("QDoubleSpinBox"),ICON_FA_KEYBOARD },
        { tr("Combo Box"),      QStringLiteral("QComboBox"),     ICON_FA_KEYBOARD },
        { tr("Date Edit"),      QStringLiteral("QDateEdit"),     ICON_FA_KEYBOARD },
        { tr("Time Edit"),      QStringLiteral("QTimeEdit"),     ICON_FA_KEYBOARD },
        { tr("Date/Time Edit"), QStringLiteral("QDateTimeEdit"), ICON_FA_KEYBOARD },
        { tr("Slider"),         QStringLiteral("QSlider"),       ICON_FA_SLIDERS  },
        { tr("Dial"),           QStringLiteral("QDial"),         ICON_FA_SLIDERS  },
        { tr("Key Sequence Edit"),QStringLiteral("QKeySequenceEdit"), ICON_FA_KEYBOARD },
    });

    // ── Display ──────────────────────────────────────────────────────────────
    addCategory(tr("Display"), ICON_FA_IMAGE, {
        { tr("Label"),          QStringLiteral("QLabel"),        ICON_FA_IMAGE },
        { tr("Pixmap"),         QStringLiteral("QLabel"),        ICON_FA_IMAGE },
        { tr("LCD Number"),     QStringLiteral("QLCDNumber"),    ICON_FA_IMAGE },
        { tr("Progress Bar"),   QStringLiteral("QProgressBar"),  ICON_FA_BARS  },
        { tr("Calendar"),       QStringLiteral("QCalendarWidget"),ICON_FA_IMAGE },
        { tr("Graphics View"),  QStringLiteral("QGraphicsView"), ICON_FA_IMAGE },
        { tr("OpenGL Widget"),  QStringLiteral("QOpenGLWidget"), ICON_FA_IMAGE },
        { tr("Web Engine View"),QStringLiteral("QWebEngineView"),ICON_FA_IMAGE },
    });

    // ── Item Views ───────────────────────────────────────────────────────────
    addCategory(tr("Item Views"), ICON_FA_TABLE_CELLS, {
        { tr("List View"),      QStringLiteral("QListView"),     ICON_FA_BARS  },
        { tr("List Widget"),    QStringLiteral("QListWidget"),   ICON_FA_BARS  },
        { tr("Tree View"),      QStringLiteral("QTreeView"),     ICON_FA_TABLE_CELLS },
        { tr("Tree Widget"),    QStringLiteral("QTreeWidget"),   ICON_FA_TABLE_CELLS },
        { tr("Table View"),     QStringLiteral("QTableView"),    ICON_FA_TABLE_CELLS },
        { tr("Table Widget"),   QStringLiteral("QTableWidget"),  ICON_FA_TABLE_CELLS },
        { tr("Column View"),    QStringLiteral("QColumnView"),   ICON_FA_TABLE_CELLS },
        { tr("Undo View"),      QStringLiteral("QUndoView"),     ICON_FA_TABLE_CELLS },
    });

    // ── Containers ───────────────────────────────────────────────────────────
    addCategory(tr("Containers"), ICON_FA_OBJECT_GROUP, {
        { tr("Group Box"),      QStringLiteral("QGroupBox"),     ICON_FA_OBJECT_GROUP },
        { tr("Scroll Area"),    QStringLiteral("QScrollArea"),   ICON_FA_OBJECT_GROUP },
        { tr("Tool Box"),       QStringLiteral("QToolBox"),      ICON_FA_OBJECT_GROUP },
        { tr("Tab Widget"),     QStringLiteral("QTabWidget"),    ICON_FA_OBJECT_GROUP },
        { tr("Stacked Widget"), QStringLiteral("QStackedWidget"),ICON_FA_OBJECT_GROUP },
        { tr("Frame"),          QStringLiteral("QFrame"),        ICON_FA_OBJECT_GROUP },
        { tr("Widget"),         QStringLiteral("QWidget"),       ICON_FA_OBJECT_GROUP },
        { tr("Dock Widget"),    QStringLiteral("QDockWidget"),   ICON_FA_OBJECT_GROUP },
        { tr("Splitter"),       QStringLiteral("QSplitter"),     ICON_FA_OBJECT_GROUP },
    });

    // ── Layouts ──────────────────────────────────────────────────────────────
    addCategory(tr("Layouts"), ICON_FA_TABLE_CELLS, {
        { tr("Vertical Layout"),    QStringLiteral("QVBoxLayout"),  ICON_FA_BARS },
        { tr("Horizontal Layout"),  QStringLiteral("QHBoxLayout"),  ICON_FA_BARS },
        { tr("Grid Layout"),        QStringLiteral("QGridLayout"),  ICON_FA_TABLE_CELLS },
        { tr("Form Layout"),        QStringLiteral("QFormLayout"),  ICON_FA_TABLE_CELLS },
        { tr("Spacer – H"),         QStringLiteral("QSpacerItem_H"),ICON_FA_BARS },
        { tr("Spacer – V"),         QStringLiteral("QSpacerItem_V"),ICON_FA_BARS },
    });

    // ── Menu / Toolbar ───────────────────────────────────────────────────────
    addCategory(tr("Menus & Bars"), ICON_FA_BARS, {
        { tr("Menu Bar"),       QStringLiteral("QMenuBar"),      ICON_FA_BARS },
        { tr("Tool Bar"),       QStringLiteral("QToolBar"),      ICON_FA_BARS },
        { tr("Status Bar"),     QStringLiteral("QStatusBar"),    ICON_FA_BARS },
    });
}

void WidgetPaletteDock::onSearchChanged(const QString &text)
{
    const QString lower = text.toLower().trimmed();

    for (int i = 0; i < m_toolBox->count(); ++i) {
        auto *list = qobject_cast<PaletteListWidget *>(m_toolBox->widget(i));
        if (!list) continue;

        int visible = 0;
        for (int j = 0; j < list->count(); ++j) {
            auto *item  = list->item(j);
            const bool match = lower.isEmpty() ||
                               item->text().toLower().contains(lower) ||
                               item->data(Qt::UserRole).toString().toLower().contains(lower);
            item->setHidden(!match);
            if (match) ++visible;
        }
        // Hide the whole tab when nothing matches
        m_toolBox->setItemEnabled(i, visible > 0);
    }
}
