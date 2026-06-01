// SvnCommitDialog.cpp
// C++17, Qt 6

#include "SvnCommitDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QSplitter>

namespace Kayte::Svn {

SvnCommitDialog::SvnCommitDialog(const QStringList &paths, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Commit"));
    setMinimumSize(520, 400);

    auto *lay = new QVBoxLayout(this);

    // File list
    lay->addWidget(new QLabel(tr("Files to commit:"), this));
    m_fileList = new QListWidget(this);
    for (const QString &p : paths)
        m_fileList->addItem(p);
    m_fileList->setMaximumHeight(120);
    lay->addWidget(m_fileList);

    // Commit message
    lay->addWidget(new QLabel(tr("Commit message:"), this));
    m_msgEdit = new QTextEdit(this);
    m_msgEdit->setPlaceholderText(tr("Enter commit message…"));
    lay->addWidget(m_msgEdit, 1);

    // Buttons
    m_buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Commit"));
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(m_buttons);

    // Disable OK until there's a message
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(m_msgEdit, &QTextEdit::textChanged, this, [this] {
        m_buttons->button(QDialogButtonBox::Ok)
            ->setEnabled(!m_msgEdit->toPlainText().trimmed().isEmpty());
    });
}

QString SvnCommitDialog::message() const
{
    return m_msgEdit->toPlainText().trimmed();
}

} // namespace Kayte::Svn
