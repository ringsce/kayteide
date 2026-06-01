// uicanvaswidget.cpp – KayteIDE
#include "uicanvaswidget.h"

#include <QPainter>
#include <QLinearGradient>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QMimeData>
#include <QAction>
#include <QMap>
#include <QXmlStreamWriter>
#include <QFile>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

static QSize defaultSizeForType(const QString &t)
{
    static const QMap<QString,QSize> tbl = {
        {QStringLiteral("QPushButton"),      {  96, 28}},
        {QStringLiteral("QToolButton"),      {  28, 28}},
        {QStringLiteral("QRadioButton"),     { 120, 22}},
        {QStringLiteral("QCheckBox"),        { 120, 22}},
        {QStringLiteral("QCommandLinkButton"),{200, 40}},
        {QStringLiteral("QDialogButtonBox"), { 240, 32}},
        {QStringLiteral("QLineEdit"),        { 160, 26}},
        {QStringLiteral("QTextEdit"),        { 200,120}},
        {QStringLiteral("QPlainTextEdit"),   { 200,120}},
        {QStringLiteral("QSpinBox"),         {  80, 26}},
        {QStringLiteral("QDoubleSpinBox"),   {  96, 26}},
        {QStringLiteral("QComboBox"),        { 140, 26}},
        {QStringLiteral("QDateEdit"),        { 110, 26}},
        {QStringLiteral("QTimeEdit"),        { 100, 26}},
        {QStringLiteral("QDateTimeEdit"),    { 160, 26}},
        {QStringLiteral("QSlider"),          { 160, 26}},
        {QStringLiteral("QDial"),            {  64, 64}},
        {QStringLiteral("QKeySequenceEdit"), { 140, 26}},
        {QStringLiteral("QLabel"),           { 120, 22}},
        {QStringLiteral("QLCDNumber"),       {  80, 40}},
        {QStringLiteral("QProgressBar"),     { 180, 22}},
        {QStringLiteral("QCalendarWidget"),  { 240,180}},
        {QStringLiteral("QGraphicsView"),    { 240,180}},
        {QStringLiteral("QOpenGLWidget"),    { 200,150}},
        {QStringLiteral("QWebEngineView"),   { 320,240}},
        {QStringLiteral("QListView"),        { 160,120}},
        {QStringLiteral("QListWidget"),      { 160,120}},
        {QStringLiteral("QTreeView"),        { 200,150}},
        {QStringLiteral("QTreeWidget"),      { 200,150}},
        {QStringLiteral("QTableView"),       { 240,150}},
        {QStringLiteral("QTableWidget"),     { 240,150}},
        {QStringLiteral("QColumnView"),      { 240,150}},
        {QStringLiteral("QUndoView"),        { 160,120}},
        {QStringLiteral("QGroupBox"),        { 200,120}},
        {QStringLiteral("QScrollArea"),      { 200,120}},
        {QStringLiteral("QToolBox"),         { 160,120}},
        {QStringLiteral("QTabWidget"),       { 240,160}},
        {QStringLiteral("QStackedWidget"),   { 200,140}},
        {QStringLiteral("QFrame"),           { 160, 80}},
        {QStringLiteral("QWidget"),          { 200,140}},
        {QStringLiteral("QDockWidget"),      { 200,140}},
        {QStringLiteral("QSplitter"),        { 200, 80}},
        {QStringLiteral("QVBoxLayout"),      {  80,120}},
        {QStringLiteral("QHBoxLayout"),      { 160, 50}},
        {QStringLiteral("QGridLayout"),      { 160,100}},
        {QStringLiteral("QFormLayout"),      { 200,100}},
        {QStringLiteral("QSpacerItem_H"),    { 100, 20}},
        {QStringLiteral("QSpacerItem_V"),    {  20,100}},
        {QStringLiteral("QMenuBar"),         { 300, 22}},
        {QStringLiteral("QToolBar"),         { 280, 36}},
        {QStringLiteral("QStatusBar"),       { 300, 22}},
    };
    return tbl.value(t, {120, 30});
}

static QColor accentFor(const QString &t)
{
    if (t.contains(QLatin1String("Button")))                      return {100,149,237};
    if (t.contains(QLatin1String("Edit"))  ||
        t.contains(QLatin1String("Spin"))  ||
        t.contains(QLatin1String("Slider"))|| t.contains(QLatin1String("Dial")))
                                                                  return { 72,199,142};
    if (t.contains(QLatin1String("Label")) ||
        t.contains(QLatin1String("LCD"))   || t.contains(QLatin1String("Progress")))
                                                                  return {255,200, 80};
    if (t.contains(QLatin1String("View"))  || t.contains(QLatin1String("Widget")))
                                                                  return {220,100,100};
    if (t.contains(QLatin1String("Layout"))|| t.contains(QLatin1String("Spacer")))
                                                                  return {160,100,220};
    if (t.contains(QLatin1String("Bar"))   || t.contains(QLatin1String("Menu")))
                                                                  return {255,140, 50};
    return {120,120,200};
}

// ─────────────────────────────────────────────────────────────────────────────
//  UiCanvasWidget
// ─────────────────────────────────────────────────────────────────────────────

UiCanvasWidget::UiCanvasWidget(QWidget *parent)
    : QWidget(parent)
    , m_undoStack(new QUndoStack(this))
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setMinimumSize(400, 300);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

// ── Mutation API ──────────────────────────────────────────────────────────────

void UiCanvasWidget::insertItem(const CanvasItem &item)
{
    m_items.append(item);
    selectItem(m_items.size() - 1);
    update();
    emit canvasModified();
}

void UiCanvasWidget::removeItem(int index)
{
    if (index < 0 || index >= m_items.size()) return;
    m_items.removeAt(index);
    if (m_selectedIndex >= m_items.size())
        m_selectedIndex = m_items.size() - 1;
    if (m_selectedIndex >= 0)
        m_items[m_selectedIndex].selected = true;
    update();
    emit canvasModified();
}

CanvasItem UiCanvasWidget::getItem(int index) const
{
    return m_items.value(index);
}

void UiCanvasWidget::moveItem(int index, QPoint topLeft)
{
    if (index < 0 || index >= m_items.size()) return;
    m_items[index].geometry.moveTopLeft(topLeft);
    update();
    emit canvasModified();
}

void UiCanvasWidget::clearCanvas()
{
    m_items.clear();
    m_selectedIndex = -1;
    m_undoStack->clear();
    update();
    emit canvasModified();
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void UiCanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf6,0xf7,0xf8));
    drawGrid(p);

    if (m_showDropPreview) {
        const QSize sz = defaultSizeForType(m_dropPreviewType);
        QRect pr(m_dropPreviewPt, sz);
        p.fillRect(pr, QColor(100,149,237,55));
        p.setPen(QPen(QColor(100,149,237), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(pr);
    }

    for (const CanvasItem &ci : m_items)
        drawItem(p, ci);

    if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
        drawSelectionHandles(p, m_items[m_selectedIndex]);
}

void UiCanvasWidget::drawGrid(QPainter &p) const
{
    p.setPen(QPen(QColor(210,212,218), 1, Qt::DotLine));
    for (int x = 0; x < width();  x += GRID * 4) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += GRID * 4) p.drawLine(0, y, width(), y);
}

void UiCanvasWidget::drawItem(QPainter &p, const CanvasItem &item) const
{
    const QRect  &r = item.geometry;
    const QColor  a = accentFor(item.widgetType);

    // Drop shadow
    p.fillRect(r.adjusted(3,3,3,3), QColor(0,0,0,25));

    // Body gradient
    QLinearGradient g(r.topLeft(), r.bottomLeft());
    g.setColorAt(0, QColor(255,255,255,235));
    g.setColorAt(1, QColor(228,230,238,235));
    p.fillRect(r, g);

    // Left accent stripe
    p.fillRect(QRect(r.left(), r.top(), 4, r.height()), a);

    // Border
    p.setPen(item.selected ? QPen(a, 2) : QPen(QColor(190,192,200), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(r);

    // Label
    QFont lf = p.font(); lf.setPointSize(8); lf.setBold(false); lf.setItalic(false);
    p.setFont(lf);
    p.setPen(QColor(30,30,40));
    p.drawText(r.adjusted(8,0,-2,0), Qt::AlignVCenter | Qt::AlignLeft, item.labelText);

    // Type tag
    QFont tf = lf; tf.setPointSize(7); tf.setItalic(true);
    p.setFont(tf);
    p.setPen(QColor(140,142,155));
    p.drawText(r.adjusted(2,0,-4,0), Qt::AlignVCenter | Qt::AlignRight, item.widgetType);
}

void UiCanvasWidget::drawSelectionHandles(QPainter &p, const CanvasItem &item) const
{
    constexpr int H = 6;
    const QRect &r  = item.geometry;
    const QColor a  = accentFor(item.widgetType);

    const QList<QPoint> pts = {
        r.topLeft(),
        {r.center().x(), r.top()},
        r.topRight(),
        {r.left(),       r.center().y()},
        {r.right(),      r.center().y()},
        r.bottomLeft(),
        {r.center().x(), r.bottom()},
        r.bottomRight(),
    };
    p.setPen(QPen(a.darker(140), 1));
    p.setBrush(Qt::white);
    for (const QPoint &pt : pts)
        p.drawRect(pt.x()-H/2, pt.y()-H/2, H, H);
}

// ── Drag-and-drop ─────────────────────────────────────────────────────────────

void UiCanvasWidget::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasFormat(
            QStringLiteral("application/x-kayteide-widget"))) {
        e->acceptProposedAction();
        m_dropPreviewType = QString::fromUtf8(
            e->mimeData()->data(QStringLiteral("application/x-kayteide-widget")));
        m_showDropPreview = true;
        update();
    }
}

void UiCanvasWidget::dragMoveEvent(QDragMoveEvent *e)
{
    QPoint pt = e->position().toPoint();
    snapToGrid(pt);
    m_dropPreviewPt = pt;
    update();
    e->acceptProposedAction();
}

void UiCanvasWidget::dropEvent(QDropEvent *e)
{
    m_showDropPreview = false;
    const QString type = QString::fromUtf8(
        e->mimeData()->data(QStringLiteral("application/x-kayteide-widget")));
    if (type.isEmpty()) return;

    QPoint pos = e->position().toPoint();
    snapToGrid(pos);

    CanvasItem item;
    item.widgetType = type;
    item.objectName = uniqueName(type);
    item.labelText  = defaultLabel(type);
    item.geometry   = QRect(pos, defaultSizeForType(type));

    m_undoStack->push(new AddItemCmd(this, item));
    e->acceptProposedAction();
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void UiCanvasWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    const int hit = itemAt(e->pos());
    if (hit != m_selectedIndex) selectItem(hit);
    if (hit >= 0) {
        m_dragOffset    = e->pos() - m_items[hit].geometry.topLeft();
        m_moveStartPos  = m_items[hit].geometry.topLeft();
        m_draggingItem  = true;
    }
    update();
}

void UiCanvasWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_draggingItem || m_selectedIndex < 0) return;
    if (!(e->buttons() & Qt::LeftButton)) return;
    QPoint tl = e->pos() - m_dragOffset;
    snapToGrid(tl);
    tl.setX(qMax(0, tl.x())); tl.setY(qMax(0, tl.y()));
    m_items[m_selectedIndex].geometry.moveTopLeft(tl);
    update();
}

void UiCanvasWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && m_draggingItem && m_selectedIndex >= 0) {
        const QPoint newPos = m_items[m_selectedIndex].geometry.topLeft();
        if (newPos != m_moveStartPos)
            m_undoStack->push(new MoveItemCmd(this, m_selectedIndex,
                                              m_moveStartPos, newPos));
        m_draggingItem = false;
    }
}

void UiCanvasWidget::keyPressEvent(QKeyEvent *e)
{
    if ((e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) && m_selectedIndex >= 0) {
        deleteSelected(); return;
    }
    if (e->matches(QKeySequence::Undo)) { m_undoStack->undo(); return; }
    if (e->matches(QKeySequence::Redo)) { m_undoStack->redo(); return; }

    if (m_selectedIndex < 0) { QWidget::keyPressEvent(e); return; }
    const int step = (e->modifiers() & Qt::ShiftModifier) ? GRID : 1;
    QPoint d;
    switch (e->key()) {
        case Qt::Key_Left:  d = {-step,  0   }; break;
        case Qt::Key_Right: d = { step,  0   }; break;
        case Qt::Key_Up:    d = {  0,   -step}; break;
        case Qt::Key_Down:  d = {  0,    step}; break;
        default: QWidget::keyPressEvent(e); return;
    }
    m_items[m_selectedIndex].geometry.translate(d);
    update();
    emit canvasModified();
}

void UiCanvasWidget::contextMenuEvent(QContextMenuEvent *e)
{
    const int hit = itemAt(e->pos());
    QMenu menu(this);
    if (hit >= 0) {
        selectItem(hit); update();
        auto *del = menu.addAction(tr("Delete \"%1\"").arg(m_items[hit].objectName));
        connect(del, &QAction::triggered, this, &UiCanvasWidget::deleteSelected);
        menu.addSeparator();
    }
    auto *clr = menu.addAction(tr("Clear canvas"));
    connect(clr, &QAction::triggered, this, &UiCanvasWidget::clearCanvas);
    menu.addSeparator();
    auto *u = menu.addAction(tr("Undo  Ctrl+Z")); u->setEnabled(m_undoStack->canUndo());
    connect(u, &QAction::triggered, m_undoStack, &QUndoStack::undo);
    auto *r = menu.addAction(tr("Redo  Ctrl+Y")); r->setEnabled(m_undoStack->canRedo());
    connect(r, &QAction::triggered, m_undoStack, &QUndoStack::redo);
    menu.exec(e->globalPos());
}

// ── Private helpers ───────────────────────────────────────────────────────────

int UiCanvasWidget::itemAt(const QPoint &pos) const
{
    for (int i = m_items.size()-1; i >= 0; --i)
        if (m_items[i].geometry.contains(pos)) return i;
    return -1;
}

void UiCanvasWidget::selectItem(int index)
{
    if (m_selectedIndex >= 0 && m_selectedIndex < m_items.size())
        m_items[m_selectedIndex].selected = false;
    m_selectedIndex = index;
    if (index >= 0 && index < m_items.size())
        m_items[index].selected = true;
    emit itemSelectionChanged(index >= 0 ? &m_items[index] : nullptr);
}

void UiCanvasWidget::deleteSelected()
{
    if (m_selectedIndex < 0) return;
    m_undoStack->push(new DeleteItemCmd(this, m_selectedIndex));
}

void UiCanvasWidget::snapToGrid(QPoint &pt) const
{
    pt.setX((pt.x()/GRID)*GRID);
    pt.setY((pt.y()/GRID)*GRID);
}

QString UiCanvasWidget::uniqueName(const QString &type) const
{
    QString base = type.startsWith(QLatin1Char('Q')) ? type.mid(1) : type;
    base[0] = base[0].toLower();
    for (int n = 1; ; ++n) {
        QString cand = base + QLatin1Char('_') + QString::number(n);
        bool used = false;
        for (const CanvasItem &ci : m_items) if (ci.objectName == cand) { used=true; break; }
        if (!used) return cand;
    }
}

QString UiCanvasWidget::defaultLabel(const QString &type) const
{
    static const QMap<QString,QString> lbl = {
        {QStringLiteral("QPushButton"),   QStringLiteral("Push Button")},
        {QStringLiteral("QLabel"),        QStringLiteral("Label")},
        {QStringLiteral("QCheckBox"),     QStringLiteral("Check Box")},
        {QStringLiteral("QRadioButton"),  QStringLiteral("Radio Button")},
        {QStringLiteral("QLineEdit"),     QStringLiteral("Line Edit")},
        {QStringLiteral("QTextEdit"),     QStringLiteral("Text Edit")},
        {QStringLiteral("QGroupBox"),     QStringLiteral("Group Box")},
        {QStringLiteral("QTabWidget"),    QStringLiteral("Tab Widget")},
        {QStringLiteral("QComboBox"),     QStringLiteral("Combo Box")},
        {QStringLiteral("QSlider"),       QStringLiteral("Slider")},
        {QStringLiteral("QProgressBar"),  QStringLiteral("Progress Bar")},
        {QStringLiteral("QToolBar"),      QStringLiteral("Tool Bar")},
        {QStringLiteral("QMenuBar"),      QStringLiteral("Menu Bar")},
        {QStringLiteral("QStatusBar"),    QStringLiteral("Status Bar")},
    };
    return lbl.value(type, type);
}

// ── .ui Export ────────────────────────────────────────────────────────────────

bool UiCanvasWidget::exportUiFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[UiCanvas] Cannot write:" << filePath;
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(1);
    xml.writeStartDocument();

    xml.writeStartElement(QStringLiteral("ui"));
    xml.writeAttribute(QStringLiteral("version"), QStringLiteral("4.0"));
    xml.writeTextElement(QStringLiteral("class"), QStringLiteral("Form"));

    // Root QWidget
    xml.writeStartElement(QStringLiteral("widget"));
    xml.writeAttribute(QStringLiteral("class"), QStringLiteral("QWidget"));
    xml.writeAttribute(QStringLiteral("name"),  QStringLiteral("Form"));

    auto writeRect = [&](const char *prop, int x, int y, int w, int h) {
        xml.writeStartElement(QStringLiteral("property"));
        xml.writeAttribute(QStringLiteral("name"), QLatin1String(prop));
        xml.writeStartElement(QStringLiteral("rect"));
        xml.writeTextElement(QStringLiteral("x"),      QString::number(x));
        xml.writeTextElement(QStringLiteral("y"),      QString::number(y));
        xml.writeTextElement(QStringLiteral("width"),  QString::number(w));
        xml.writeTextElement(QStringLiteral("height"), QString::number(h));
        xml.writeEndElement();
        xml.writeEndElement();
    };
    writeRect("geometry", 0, 0, width(), height());

    auto writeString = [&](const char *prop, const QString &val) {
        xml.writeStartElement(QStringLiteral("property"));
        xml.writeAttribute(QStringLiteral("name"), QLatin1String(prop));
        xml.writeTextElement(QStringLiteral("string"), val);
        xml.writeEndElement();
    };
    writeString("windowTitle", QStringLiteral("Form"));

    // Child widgets (skip layout placeholders – they're parent-level concepts)
    const QStringList layoutTypes = {
        QStringLiteral("QVBoxLayout"), QStringLiteral("QHBoxLayout"),
        QStringLiteral("QGridLayout"), QStringLiteral("QFormLayout"),
        QStringLiteral("QSpacerItem_H"), QStringLiteral("QSpacerItem_V"),
    };

    // Widgets that carry a "text" property
    const QStringList textWidgets = {
        QStringLiteral("QPushButton"), QStringLiteral("QToolButton"),
        QStringLiteral("QLabel"),      QStringLiteral("QCheckBox"),
        QStringLiteral("QRadioButton"),QStringLiteral("QCommandLinkButton"),
        QStringLiteral("QGroupBox"),
    };

    for (const CanvasItem &ci : m_items) {
        if (layoutTypes.contains(ci.widgetType)) continue;

        xml.writeStartElement(QStringLiteral("widget"));
        xml.writeAttribute(QStringLiteral("class"), ci.widgetType);
        xml.writeAttribute(QStringLiteral("name"),  ci.objectName);

        writeRect("geometry",
                  ci.geometry.x(), ci.geometry.y(),
                  ci.geometry.width(), ci.geometry.height());

        if (textWidgets.contains(ci.widgetType))
            writeString("text", ci.labelText);

        xml.writeEndElement(); // widget
    }

    xml.writeEndElement(); // root widget

    // tabstops (optional – generated for all non-layout items)
    if (!m_items.isEmpty()) {
        xml.writeStartElement(QStringLiteral("tabstops"));
        for (const CanvasItem &ci : m_items) {
            if (!layoutTypes.contains(ci.widgetType))
                xml.writeTextElement(QStringLiteral("tabstop"), ci.objectName);
        }
        xml.writeEndElement();
    }

    xml.writeStartElement(QStringLiteral("resources"));
    xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("connections"));
    xml.writeEndElement();

    xml.writeEndElement(); // ui
    xml.writeEndDocument();
    return true;
}
