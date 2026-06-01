#pragma once
// SvnCommitDialog.h – Modal commit dialog with file list + message editor
// C++17, Qt 6

#include "SvnTypes.h"
#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QDialogButtonBox;
class QListWidget;
QT_END_NAMESPACE

namespace Kayte::Svn {

class SvnCommitDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SvnCommitDialog(const QStringList &paths, QWidget *parent = nullptr);
    [[nodiscard]] QString message() const;

private:
    QTextEdit         *m_msgEdit  { nullptr };
    QListWidget       *m_fileList { nullptr };
    QDialogButtonBox  *m_buttons  { nullptr };
};

} // namespace Kayte::Svn
