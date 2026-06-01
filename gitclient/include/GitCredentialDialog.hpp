#pragma once
#include <QDialog>
#include <QLineEdit>

namespace Kayte {

/// Modal dialog shown when remote operations need credentials.
class GitCredentialDialog : public QDialog {
    Q_OBJECT
public:
    explicit GitCredentialDialog(const QString &url,
                                  const QString &usernameHint,
                                  QWidget *parent = nullptr);
    QString username() const;
    QString password() const;

private:
    QLineEdit *m_user {};
    QLineEdit *m_pass {};
};

} // namespace Kayte
