#include "buildconfigdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

BuildConfigDialog::BuildConfigDialog(const BuildConfigurations &configs, QWidget *parent)
    : QDialog(parent)
    , m_source(configs)
    , m_configs(configs.configurations())
    , m_active(configs.activeIndex())
{
    setWindowTitle(tr("Build Configurations"));
    resize(820, 460);

    // ── Left: configuration list ─────────────────────────────────────────────
    m_list = new QListWidget(this);
    m_list->setMinimumWidth(170);

    auto *btnAdd    = new QPushButton(tr("Add"), this);
    auto *btnClone  = new QPushButton(tr("Clone"), this);
    m_btnRemove     = new QPushButton(tr("Remove"), this);
    m_btnActive     = new QPushButton(tr("Make Active"), this);
    auto *btnReset  = new QPushButton(tr("Reset to Detected…"), this);
    btnReset->setToolTip(tr("Replace all configurations with the defaults detected "
                            "from the project's files"));

    auto *listButtons = new QHBoxLayout;
    listButtons->addWidget(btnAdd);
    listButtons->addWidget(btnClone);
    listButtons->addWidget(m_btnRemove);

    auto *left = new QVBoxLayout;
    left->addWidget(m_list, 1);
    left->addLayout(listButtons);
    left->addWidget(m_btnActive);
    left->addWidget(btnReset);

    // ── Right: the selected configuration ────────────────────────────────────
    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const auto field = [this, &mono](const QString &placeholder) {
        auto *e = new QLineEdit(this);
        e->setFont(mono);
        e->setPlaceholderText(placeholder);
        e->setClearButtonEnabled(true);
        connect(e, &QLineEdit::textEdited, this, [this] { storeField(); });
        return e;
    };
    m_name     = field(tr("e.g. Debug"));
    m_name->setFont(font());
    m_buildDir = field(QStringLiteral("build/${Config}"));
    m_build    = field(tr("command that builds the project"));
    m_clean    = field(tr("command that removes build output"));
    m_run      = field(tr("command that runs the program"));
    m_debug    = field(tr("command that starts the debugger"));

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(tr("Name:"),            m_name);
    form->addRow(tr("Build directory:"), m_buildDir);
    form->addRow(tr("Build:"),           m_build);
    form->addRow(tr("Clean:"),           m_clean);
    form->addRow(tr("Run:"),             m_run);
    form->addRow(tr("Debug:"),           m_debug);
    m_buildFirst = new QCheckBox(tr("Build before running (Run and Debug start only if it succeeds)"), this);
    connect(m_buildFirst, &QCheckBox::toggled, this, [this] { storeField(); });
    form->addRow(QString(), m_buildFirst);

    m_preview = new QLabel(this);
    m_preview->setWordWrap(true);
    m_preview->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_preview->setFont(mono);

    const QString kind = m_source.detectedKind();
    auto *help = new QLabel(
        tr("Commands run with <code>/bin/sh</code> in <code>%1</code>.<br>"
           "Variables: <code>${ProjectDir}</code>, <code>${ProjectName}</code>, "
           "<code>${BuildDir}</code>, <code>${Config}</code>,<br>"
           "<code>${File}</code>, <code>${FileBaseName}</code> (the file in the editor),<br>"
           "<code>${KayteTools}</code> (the FPC, Lazarus and QEMU bundled with KayteIDE).%2")
            .arg(m_source.projectDir().toHtmlEscaped(),
                 kind.isEmpty() ? QString()
                                : tr("<br>Detected project type: <b>%1</b>.").arg(kind)),
        this);
    help->setWordWrap(true);
    help->setTextFormat(Qt::RichText);

    auto *right = new QVBoxLayout;
    right->addLayout(form);
    right->addSpacing(6);
    right->addWidget(new QLabel(tr("Build command after substitution:"), this));
    right->addWidget(m_preview);
    right->addStretch();
    right->addWidget(help);

    auto *columns = new QHBoxLayout;
    columns->addLayout(left);
    columns->addLayout(right, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        for (const BuildConfiguration &c : m_configs) {
            if (c.name.trimmed().isEmpty()) {
                QMessageBox::warning(this, windowTitle(), tr("Every configuration needs a name."));
                return;
            }
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *root = new QVBoxLayout(this);
    root->addLayout(columns, 1);
    root->addWidget(buttons);

    connect(m_list, &QListWidget::currentRowChanged, this, &BuildConfigDialog::showConfig);
    connect(btnAdd,      &QPushButton::clicked, this, &BuildConfigDialog::addConfig);
    connect(btnClone,    &QPushButton::clicked, this, &BuildConfigDialog::cloneConfig);
    connect(m_btnRemove, &QPushButton::clicked, this, &BuildConfigDialog::removeConfig);
    connect(btnReset,    &QPushButton::clicked, this, &BuildConfigDialog::resetToDetected);
    connect(m_btnActive, &QPushButton::clicked, this, [this] {
        if (m_current < 0) return;
        m_active = m_current;
        populateList();
    });

    populateList();
}

void BuildConfigDialog::populateList()
{
    const int keep = m_current < 0 ? m_active : m_current;
    m_loading = true;
    m_list->clear();
    for (int i = 0; i < m_configs.size(); ++i) {
        auto *item = new QListWidgetItem(m_configs[i].name, m_list);
        if (i == m_active) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            item->setText(tr("%1  (active)").arg(m_configs[i].name));
        }
    }
    m_loading = false;
    m_current = -1;
    m_list->setCurrentRow(qBound(0, keep, int(m_configs.size()) - 1));
}

void BuildConfigDialog::showConfig(int row)
{
    if (m_loading) return;
    m_current = row;
    const bool valid = row >= 0 && row < m_configs.size();
    const BuildConfiguration c = valid ? m_configs[row] : BuildConfiguration{};

    m_loading = true;
    m_name->setText(c.name);
    m_buildDir->setText(c.buildDir);
    m_build->setText(c.build);
    m_clean->setText(c.clean);
    m_run->setText(c.run);
    m_debug->setText(c.debug);
    m_buildFirst->setChecked(c.buildBeforeRun);
    for (QLineEdit *e : {m_name, m_buildDir, m_build, m_clean, m_run, m_debug})
        e->setCursorPosition(0);   // show the start of long commands
    m_loading = false;

    for (QLineEdit *e : {m_name, m_buildDir, m_build, m_clean, m_run, m_debug})
        e->setEnabled(valid);
    m_buildFirst->setEnabled(valid);
    m_btnRemove->setEnabled(valid && m_configs.size() > 1);
    m_btnActive->setEnabled(valid && row != m_active);
    updatePreview();
}

void BuildConfigDialog::storeField()
{
    if (m_loading || m_current < 0 || m_current >= m_configs.size()) return;
    BuildConfiguration &c = m_configs[m_current];
    c.name     = m_name->text();
    c.buildDir = m_buildDir->text();
    c.build    = m_build->text();
    c.clean    = m_clean->text();
    c.run      = m_run->text();
    c.debug    = m_debug->text();
    c.buildBeforeRun = m_buildFirst->isChecked();

    if (QListWidgetItem *item = m_list->item(m_current))
        item->setText(m_current == m_active ? tr("%1  (active)").arg(c.name) : c.name);
    updatePreview();
}

void BuildConfigDialog::updatePreview()
{
    if (m_current < 0 || m_current >= m_configs.size()) {
        m_preview->clear();
        return;
    }
    const BuildConfiguration &c = m_configs[m_current];
    m_preview->setText(c.build.isEmpty() ? tr("(no build command)")
                                         : m_source.expand(c.build, c));
}

QString BuildConfigDialog::uniqueName(const QString &base) const
{
    const auto taken = [this](const QString &n) {
        for (const BuildConfiguration &c : m_configs)
            if (c.name == n) return true;
        return false;
    };
    if (!taken(base)) return base;
    for (int i = 2;; ++i)
        if (!taken(QStringLiteral("%1 %2").arg(base).arg(i)))
            return QStringLiteral("%1 %2").arg(base).arg(i);
}

void BuildConfigDialog::addConfig()
{
    BuildConfiguration c;
    c.name     = uniqueName(tr("New Configuration"));
    c.buildDir = QStringLiteral("build/${Config}");
    m_configs << c;
    m_current = int(m_configs.size()) - 1;
    populateList();
    m_name->setFocus();
    m_name->selectAll();
}

void BuildConfigDialog::cloneConfig()
{
    if (m_current < 0) return;
    BuildConfiguration c = m_configs[m_current];
    c.name = uniqueName(tr("%1 Copy").arg(c.name));
    m_configs << c;
    m_current = int(m_configs.size()) - 1;
    populateList();
    m_name->setFocus();
    m_name->selectAll();
}

void BuildConfigDialog::removeConfig()
{
    if (m_current < 0 || m_configs.size() <= 1) return;
    m_configs.removeAt(m_current);
    if (m_active == m_current)     m_active = 0;
    else if (m_active > m_current) --m_active;
    m_current = qMin(m_current, int(m_configs.size()) - 1);
    populateList();
}

void BuildConfigDialog::resetToDetected()
{
    if (QMessageBox::question(this, windowTitle(),
            tr("Replace all configurations with the defaults detected from the "
               "project's files?")) != QMessageBox::Yes)
        return;
    m_configs = BuildConfigurations::detect(m_source.projectDir(), m_source.projectName());
    m_active  = 0;
    m_current = -1;
    populateList();
}
