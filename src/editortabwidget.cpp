#include "editortabwidget.h"

#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include "linenumberarea.h" // Make sure this is correctly implemented
#include "vbsyntaxhighlighter.h"
#include "cppsyntaxhighlighter.h"
#include "kaytesyntaxhighlighter.h"
#include "pascalsyntaxhighlighter.h"
#include "delphisyntaxhighlighter.h"
#include "markdownsyntaxhighlighter.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QTextCharFormat>
#include <QDebug>
#include <QFileInfo> // Added for QFileInfo::suffix()
#include <QButtonGroup>
#include <QDesktopServices>
#include <QLabel>
#include <QSettings>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>
#include <QTextBlock>
#include <QToolButton>
#include <QUrl>

EditorTabWidget::EditorTabWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent),
      // IMPORTANT: Initialize m_editor (QPlainTextEdit) FIRST
      m_editor(new QPlainTextEdit(this)), // m_editor is a QPlainTextEdit*
      // Now, correctly initialize m_lineNumberArea by passing m_editor AND this
      m_lineNumberArea(new LineNumberArea(m_editor, this)),
      m_currentHighlighter(nullptr),
      m_vbHighlighter(nullptr),
      m_cppHighlighter(nullptr),
      m_kayteHighlighter(nullptr),
      m_pascalHighlighter(nullptr),
      m_delphiHighlighter(nullptr),
      m_filePath(filePath)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // The line number area and the editor share one pane, which a Markdown
    // file's preview splitter can hold.
    m_editorPane = new QWidget(this);
    QHBoxLayout *editorLayout = new QHBoxLayout(m_editorPane);
    editorLayout->setContentsMargins(0,0,0,0);
    editorLayout->setSpacing(0); // No space between line number area and editor

    editorLayout->addWidget(m_lineNumberArea); // Add line number area first
    editorLayout->addWidget(m_editor);         // Then add the editor

    layout->addWidget(m_editorPane);

    // Connect signals from m_editor to line number area and other updates
    connect(m_editor, &QPlainTextEdit::blockCountChanged, this, &EditorTabWidget::updateLineNumberAreaWidth);
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, m_lineNumberArea, QOverload<>::of(&QWidget::update)); // Connect to scroll updates directly
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &EditorTabWidget::highlightCurrentLine);
    connect(m_editor->document(), &QTextDocument::contentsChanged, this, &EditorTabWidget::handleContentsChanged);

    // Gutter colours follow the editor's palette (light or dark theme).
    {
        const QPalette pal = m_editor->palette();
        const QColor base = pal.color(QPalette::Base);
        const bool dark = base.lightness() < 128;
        m_lineNumberArea->setBackgroundColor(dark ? base.darker(115) : base.darker(104));
        m_lineNumberArea->setForegroundColor(pal.color(QPalette::PlaceholderText));
        m_lineNumberArea->setCurrentLineColor(pal.color(QPalette::Text));
    }

    setupHighlighters();

    if (!m_filePath.isEmpty()) {
        // Use the loadFile method to load content and apply highlighter
        loadFile(m_filePath);
    } else {
        m_editor->setPlainText("");
        m_editor->document()->setModified(false);
        emit titleChanged(tr("Untitled"));
    }

    highlightCurrentLine();
    updateLineNumberAreaWidth(m_editor->blockCount()); // Ensure initial width is set
}

EditorTabWidget::~EditorTabWidget()
{
    // Highlighters are parented to QTextDocument. When m_editor->document() is deleted,
    // it will automatically delete its child highlighters. Explicit deletion is not needed here.
    qDebug() << "EditorTabWidget destroyed for file:" << m_filePath;
}

namespace {
// Files above this are read and decoded on a worker thread.
constexpr qint64 kAsyncLoadBytes = 512 * 1024;
// Syntax highlighting runs on the GUI thread over the whole document; above
// this size it would freeze the IDE, so big files open as plain text.
constexpr qint64 kHighlightMaxBytes = 1024 * 1024;
}

bool EditorTabWidget::loadFile(const QString &filePath) {
    qDebug() << "EditorTabWidget::loadFile called for:" << filePath;

    const QFileInfo info(filePath);
    if (info.exists() && info.size() > kAsyncLoadBytes) {
        // Big file: read it off the GUI thread, then show it.
        m_filePath = filePath;
        m_editor->setReadOnly(true);
        m_editor->setPlainText(tr("Loading %1 (%2 MB)…")
                                   .arg(info.fileName())
                                   .arg(info.size() / (1024.0 * 1024.0), 0, 'f', 1));
        auto *watcher = new QFutureWatcher<QString>(this);
        connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, filePath] {
            watcher->deleteLater();
            m_editor->setReadOnly(false);
            finishLoad(filePath, watcher->result());
        });
        watcher->setFuture(QtConcurrent::run([filePath]() -> QString {
            QFile f(filePath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
            return QString::fromUtf8(f.readAll());
        }));
        emit titleChanged(info.fileName());
        return true;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_editor->setPlainText(tr("Could not open file: %1\n%2").arg(filePath, file.errorString()));
        setModified(false); // Not modified
        return false;
    }

    // Load the file content
    QString content = file.readAll();
    file.close();
    finishLoad(filePath, content);
    return true;
}

void EditorTabWidget::finishLoad(const QString &filePath, const QString &content)
{
    qDebug() << "Loaded" << content.length() << "characters from" << filePath;

    // Set the content (detach any highlighter first so it doesn't run on it)
    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(nullptr);
        m_currentHighlighter = nullptr;
    }
    m_editor->setPlainText(content);
    m_filePath = filePath;

    // CRITICAL: Clear modified flag AFTER setting text
    m_editor->document()->setModified(false);

    // Update line number area for the new content
    if (m_lineNumberArea) {
        qDebug() << "Updating line number area, block count:" << m_editor->blockCount();
        m_lineNumberArea->setupConnections();
        m_lineNumberArea->updateGeometry();
        m_lineNumberArea->update();
    } else {
        qWarning() << "Line number area is null!";
    }

    // Apply syntax highlighter based on file extension (not for huge files)
    if (content.size() <= kHighlightMaxBytes)
        applyHighlighterForFile(m_filePath);

    updateMarkdownMode();

    // Emit signals
    emit titleChanged(QFileInfo(m_filePath).fileName());
    emit modificationChanged(false);

    qDebug() << "File loaded successfully, modified state:" << m_editor->document()->isModified();
}

void EditorTabWidget::setupHighlighters()
{
    // Ensure m_editor is valid before trying to access its document
    if (!m_editor || !m_editor->document()) {
        qWarning() << "EditorTabWidget::setupHighlighters: Editor or document not available.";
        return;
    }

    QTextDocument *doc = m_editor->document(); // <--- Changed m_textEdit to m_editor

    // Initialize your highlighters with the document
    // Ensure you have included the headers for your highlighters
    m_vbHighlighter = new VBSyntaxHighlighter(doc);
    m_cppHighlighter = new CppSyntaxHighlighter(doc);
    m_kayteHighlighter = new KayteSyntaxHighlighter(doc);
    m_pascalHighlighter = new PascalSyntaxHighlighter(doc);
    m_delphiHighlighter = new DelphiSyntaxHighlighter(doc);
    m_markdownHighlighter = new MarkdownSyntaxHighlighter(doc);

    m_currentHighlighter = nullptr;
}

// --- REMOVED THE FOLLOWING DUPLICATE/OBSOLETE FUNCTIONS ---
// void EditorTabWidget::on_document_contentsChanged() { /* ... */ }
// void EditorTabWidget::on_editor_textChanged() { /* ... */ }
// --- END REMOVAL ---


void EditorTabWidget::applyHighlighterForFile(const QString &filePath)
{
    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(nullptr);
    }

    QString suffix = QFileInfo(filePath).suffix().toLower();
    QTextDocument *doc = m_editor->document(); // <--- Changed m_textEdit to m_editor

    if (suffix == "vb") {
        m_currentHighlighter = m_vbHighlighter;
    } else if (suffix == "cpp" || suffix == "h" || suffix == "cxx" || suffix == "hpp") { // Added more C++ extensions
        m_currentHighlighter = m_cppHighlighter;
    } else if (suffix == "kayte" || suffix == "kyt") {
        m_currentHighlighter = m_kayteHighlighter;
    } else if (suffix == "pas" || suffix == "pp" || suffix == "dpr") {
        m_currentHighlighter = m_pascalHighlighter;
    } else if (suffix == "dfm") {
        m_currentHighlighter = m_delphiHighlighter;
    } else if (isMarkdownFile(filePath)) {
        m_currentHighlighter = m_markdownHighlighter;
    } else {
        m_currentHighlighter = nullptr;
    }

    if (m_currentHighlighter) {
        m_currentHighlighter->setDocument(doc); // Attach new highlighter
        m_currentHighlighter->rehighlight(); // Re-apply highlighting
    }
}

bool EditorTabWidget::isModified() const
{
    return m_editor && m_editor->document() && m_editor->document()->isModified();
}

void EditorTabWidget::setModified(bool modified)
{
    if (!m_editor || !m_editor->document()) {
        qWarning() << "EditorTabWidget::setModified: Editor or document not available";
        return;
    }

    bool currentState = m_editor->document()->isModified();

    qDebug() << "setModified called: current =" << currentState << ", new =" << modified;

    // Only update and emit if the modification state actually changes
    if (currentState != modified) {
        m_editor->document()->setModified(modified);
        emit modificationChanged(modified);
        qDebug() << "Modification state changed to:" << modified;
    }
}

// NOTE: There was a duplicate loadFile function. I've kept the one from the top
// and removed this one, as the constructor now correctly calls the top one.
/*
bool EditorTabWidget::loadFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file %1: %2").arg(filePath, file.errorString()));
        return false;
    }

    QTextStream in(&file);
    m_textEdit->setText(in.readAll()); // <--- This would also need to be m_editor
    file.close();

    m_filePath = filePath;
    m_textEdit->document()->setModified(false); // <--- This would also need to be m_editor

    applyHighlighterForFile(m_filePath);

    emit modificationChanged(false);
    emit titleChanged(QFileInfo(m_filePath).fileName());

    return true;
}
*/

bool EditorTabWidget::saveFile(const QString &filePath)
{
    qDebug() << "EditorTabWidget::saveFile called for:" << filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not save file %1: %2").arg(filePath, file.errorString()));
        qDebug() << "Save failed:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    file.close();

    // Update file path if it changed (e.g., Save As)
    if (m_filePath != filePath) {
        m_filePath = filePath;
        applyHighlighterForFile(m_filePath);
        updateMarkdownMode();
        emit titleChanged(QFileInfo(m_filePath).fileName());
    }

    // CRITICAL: Clear modified flag after successful save
    m_editor->document()->setModified(false);

    // Emit modification state changed
    emit modificationChanged(false);

    qDebug() << "File saved successfully, modified state:" << m_editor->document()->isModified();

    return true;
}

void EditorTabWidget::resizeEvent(QResizeEvent *event)
{
    // The pane's layout positions the line number area and the editor.
    QWidget::resizeEvent(event);
}

void EditorTabWidget::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    if (m_lineNumberArea) {
        m_lineNumberArea->setFixedWidth(m_lineNumberArea->sizeHint().width());
    }
}

void EditorTabWidget::highlightCurrentLine()
{
    updateExtraSelections();
}

namespace {
bool isErrorSeverity(const QString &severity)
{
    const QString s = severity.toLower();
    return s.contains(QLatin1String("error")) || s == QLatin1String("fatal");
}
}

void EditorTabWidget::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    const bool dark = m_editor->palette().color(QPalette::Base).lightness() < 128;

    // Lines with build errors / warnings, tinted red / amber.
    for (const Diagnostic &d : std::as_const(m_diagnostics)) {
        const QTextBlock block = m_editor->document()->findBlockByNumber(d.line - 1);
        if (!block.isValid()) continue;
        QTextEdit::ExtraSelection selection;
        QColor tint = isErrorSeverity(d.severity) ? QColor(0xe5, 0x48, 0x4d) : QColor(0xd9, 0xa4, 0x00);
        tint.setAlpha(dark ? 46 : 34);
        selection.format.setBackground(tint);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = QTextCursor(block);
        extraSelections.append(selection);
    }

    if (!m_editor->isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        // A faint tint of the theme's highlight colour, readable on light
        // and dark backgrounds alike.
        QColor lineColor = m_editor->palette().color(QPalette::Highlight);
        lineColor.setAlpha(dark ? 48 : 32);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = m_editor->textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    m_editor->setExtraSelections(extraSelections);
}

void EditorTabWidget::setDiagnostics(const QVector<Diagnostic> &diagnostics)
{
    m_diagnostics = diagnostics;

    // One marker per line: an error wins over warnings; the tooltip lists all.
    QMap<int, LineNumberArea::Marker> markers;
    for (const Diagnostic &d : diagnostics) {
        if (d.line <= 0) continue;
        LineNumberArea::Marker &m = markers[d.line - 1];
        const bool error = isErrorSeverity(d.severity);
        if (m.text.isEmpty()) m.error = error; else m.error = m.error || error;
        const QString where = d.column > 0 ? tr("line %1, column %2").arg(d.line).arg(d.column)
                                           : tr("line %1").arg(d.line);
        const QString entry = QStringLiteral("%1 (%2): %3")
                                  .arg(d.severity.isEmpty() ? tr("error") : d.severity, where, d.message);
        m.text = m.text.isEmpty() ? entry : m.text + QLatin1Char('\n') + entry;
    }
    m_lineNumberArea->setMarkers(markers);
    updateExtraSelections();
}

void EditorTabWidget::updateLineNumberArea(int /*value*/)
{
    if (m_lineNumberArea) {
        m_lineNumberArea->update();
    }
}

void EditorTabWidget::handleContentsChanged()
{
    if (!m_editor || !m_editor->document()) {
        return;
    }

    bool modified = m_editor->document()->isModified();
    qDebug() << "Content changed, document modified:" << modified;
    emit modificationChanged(modified);
}

// ─────────────────────────────────────────────────────────────────────────────
// Markdown: live preview
// ─────────────────────────────────────────────────────────────────────────────

namespace {
const char *kMarkdownViewKey = "editor/markdownView";
}

bool EditorTabWidget::isMarkdownFile(const QString &path)
{
    static const QStringList suffixes{
        QStringLiteral("md"), QStringLiteral("markdown"), QStringLiteral("mdown"),
        QStringLiteral("mkd"), QStringLiteral("mkdn")};
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

// Builds the preview when the file is Markdown, and tears it down when a
// Save As gives the file another type.
void EditorTabWidget::updateMarkdownMode()
{
    const bool markdown = isMarkdownFile(m_filePath);
    if (markdown == (m_preview != nullptr)) {
        if (markdown) renderPreview();   // e.g. reloaded or renamed .md → .markdown
        return;
    }

    auto *layout = static_cast<QVBoxLayout *>(this->layout());

    if (!markdown) {
        m_previewTimer->stop();
        layout->removeWidget(m_markdownBar);
        layout->removeWidget(m_splitter);
        m_editorPane->setParent(this);
        layout->addWidget(m_editorPane);
        m_editorPane->show();
        m_markdownBar->deleteLater();
        m_splitter->deleteLater();       // takes the preview with it
        m_previewTimer->deleteLater();
        m_markdownBar = nullptr;
        m_splitter = nullptr;
        m_preview = nullptr;
        m_previewTimer = nullptr;
        m_viewButtons = nullptr;
        return;
    }

    // ── View switch: Editor | Split | Preview ────────────────────────────────
    m_markdownBar = new QWidget(this);
    auto *bar = new QHBoxLayout(m_markdownBar);
    bar->setContentsMargins(8, 2, 6, 2);
    bar->setSpacing(2);
    auto *title = new QLabel(tr("Markdown"), m_markdownBar);
    QFont small = title->font();
    small.setPointSizeF(small.pointSizeF() - 1);
    title->setFont(small);
    title->setEnabled(false);
    bar->addWidget(title);
    bar->addStretch();

    m_viewButtons = new QButtonGroup(m_markdownBar);
    m_viewButtons->setExclusive(true);
    const struct { MarkdownView view; const char *text; const char *tip; } views[] = {
        { MarkdownView::Editor,  QT_TR_NOOP("Editor"),  QT_TR_NOOP("Show only the Markdown source") },
        { MarkdownView::Split,   QT_TR_NOOP("Split"),   QT_TR_NOOP("Source and live preview side by side") },
        { MarkdownView::Preview, QT_TR_NOOP("Preview"), QT_TR_NOOP("Show only the rendered preview") },
    };
    for (const auto &v : views) {
        auto *b = new QToolButton(m_markdownBar);
        b->setText(tr(v.text));
        b->setToolTip(tr(v.tip));
        b->setCheckable(true);
        b->setAutoRaise(true);
        b->setFont(small);
        m_viewButtons->addButton(b, int(v.view));
        bar->addWidget(b);
    }
    connect(m_viewButtons, &QButtonGroup::idClicked, this, [this](int id) {
        setMarkdownView(MarkdownView(id));
        QSettings().setValue(QLatin1String(kMarkdownViewKey), id);
    });

    // ── Preview ──────────────────────────────────────────────────────────────
    m_preview = new QTextBrowser(this);
    m_preview->setOpenLinks(false);          // handled in onPreviewLinkClicked
    m_preview->setFrameShape(QFrame::NoFrame);
    m_preview->document()->setDocumentMargin(18);
    connect(m_preview, &QTextBrowser::anchorClicked, this, &EditorTabWidget::onPreviewLinkClicked);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    layout->removeWidget(m_editorPane);
    m_splitter->addWidget(m_editorPane);
    m_splitter->addWidget(m_preview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
    layout->addWidget(m_markdownBar);
    layout->addWidget(m_splitter, 1);

    // Re-render shortly after typing stops, and follow the editor's scrolling.
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(250);
    connect(m_previewTimer, &QTimer::timeout, this, &EditorTabWidget::renderPreview);
    connect(m_editor->document(), &QTextDocument::contentsChanged, m_previewTimer,
            qOverload<>(&QTimer::start));
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged, this,
            &EditorTabWidget::syncPreviewScroll);

    const int saved = QSettings().value(QLatin1String(kMarkdownViewKey),
                                        int(MarkdownView::Split)).toInt();
    setMarkdownView(MarkdownView(qBound(0, saved, 2)));
    renderPreview();
}

void EditorTabWidget::setMarkdownView(MarkdownView view)
{
    if (!m_preview) return;
    m_editorPane->setVisible(view != MarkdownView::Preview);
    m_preview->setVisible(view != MarkdownView::Editor);
    if (QAbstractButton *b = m_viewButtons->button(int(view)))
        b->setChecked(true);
    if (view == MarkdownView::Preview)
        m_preview->setFocus();
    else
        m_editor->setFocus();
    syncPreviewScroll();
}

void EditorTabWidget::renderPreview()
{
    if (!m_preview) return;

    // Relative images and links resolve against the file's folder.
    const QString dir = QFileInfo(m_filePath).absolutePath();
    m_preview->setSearchPaths({dir});
    m_preview->document()->setBaseUrl(QUrl::fromLocalFile(dir + QLatin1Char('/')));

    // Keep the reader's place across re-renders.
    QScrollBar *sb = m_preview->verticalScrollBar();
    const double ratio = sb->maximum() > 0 ? double(sb->value()) / sb->maximum() : 0.0;
    m_preview->setMarkdown(m_editor->toPlainText());
    stylePreview();
    sb->setValue(qRound(ratio * sb->maximum()));
    syncPreviewScroll();
}

// Qt's Markdown import gives code blocks and quotes no visual treatment:
// shade code blocks, tint and indent quotes, and space out headings.
void EditorTabWidget::stylePreview()
{
    QTextDocument *doc = m_preview->document();
    const QColor base = m_preview->palette().color(QPalette::Base);
    const bool dark = base.lightness() < 128;
    const QColor codeBg  = dark ? base.lighter(150) : base.darker(106);
    const QColor quoteBg = dark ? base.lighter(125) : base.darker(103);
    const QColor quoteFg = m_preview->palette().color(QPalette::PlaceholderText);

    // A block is code when it is a fenced block, or all of its text is
    // monospaced (indented code blocks).
    const auto isCode = [](const QTextBlock &b) {
        if (b.blockFormat().hasProperty(QTextFormat::BlockCodeFence)) return true;
        if (b.text().trimmed().isEmpty()) return false;
        for (auto it = b.begin(); !it.atEnd(); ++it)
            if (!it.fragment().charFormat().fontFixedPitch()) return false;
        return true;
    };

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock b = doc->begin(); b.isValid(); b = b.next()) {
        QTextBlockFormat f = b.blockFormat();
        QTextCursor c(b);
        if (isCode(b) && !b.textList()) {   // list items keep Qt's layout
            f.setBackground(codeBg);
            f.setLeftMargin(f.leftMargin() + 4);
            f.setTopMargin(0);
            f.setBottomMargin(0);
            // Breathing room before / after a run of code lines.
            if (!b.previous().isValid() || !isCode(b.previous())) f.setTopMargin(6);
            if (!b.next().isValid() || !isCode(b.next())) f.setBottomMargin(10);
            c.setBlockFormat(f);
        } else if (f.hasProperty(QTextFormat::BlockQuoteLevel)) {
            f.setBackground(quoteBg);
            f.setLeftMargin(16.0 * f.intProperty(QTextFormat::BlockQuoteLevel));
            c.setBlockFormat(f);
            c.select(QTextCursor::BlockUnderCursor);
            QTextCharFormat cf;
            cf.setForeground(quoteFg);
            c.mergeCharFormat(cf);
        } else if (f.headingLevel() > 0) {
            f.setTopMargin(f.headingLevel() <= 2 ? 16 : 12);
            f.setBottomMargin(6);
            c.setBlockFormat(f);
        }
    }
    cursor.endEditBlock();
    doc->setModified(false);
}

// Proportional scroll sync: the preview shows the same part of the document
// as the editor (exact when the two have similar heights, close otherwise).
void EditorTabWidget::syncPreviewScroll()
{
    if (!m_preview || !m_preview->isVisible() || !m_editorPane->isVisible()) return;
    const QScrollBar *src = m_editor->verticalScrollBar();
    QScrollBar *dst = m_preview->verticalScrollBar();
    if (src->maximum() <= 0) {
        dst->setValue(0);
        return;
    }
    dst->setValue(qRound(double(src->value()) / src->maximum() * dst->maximum()));
}

void EditorTabWidget::onPreviewLinkClicked(const QUrl &url)
{
    if (url.scheme().isEmpty() && url.path().isEmpty() && url.hasFragment()) {
        m_preview->scrollToAnchor(url.fragment());   // #section
        return;
    }
    const QUrl resolved = m_preview->document()->baseUrl().resolved(url);
    if (resolved.isLocalFile()) {
        const QString path = resolved.toLocalFile();
        if (QFileInfo(path).isFile()) {
            emit openFileRequested(path);            // open it in the IDE
            return;
        }
    }
    QDesktopServices::openUrl(resolved);             // web links, mail, folders
}
