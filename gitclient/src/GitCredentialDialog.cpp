#include "GitCredentialDialog.hpp"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace Kayte {

GitCredentialDialog::GitCredentialDialog(const QString &url,
                                          const QString &usernameHint,
                                          QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Git Credentials");
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("Authentication required for:\n%1").arg(url)));

    auto *form = new QFormLayout;
    m_user = new QLineEdit(usernameHint, this);
    m_pass = new QLineEdit(this);
    m_pass->setEchoMode(QLineEdit::Password);
    form->addRow("Username:", m_user);
    form->addRow("Password:", m_pass);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    resize(360, 150);
}

QString GitCredentialDialog::username() const { return m_user->text(); }
QString GitCredentialDialog::password() const { return m_pass->text(); }

} // namespace Kayte
