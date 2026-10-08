#ifndef BUILDCONFIGDIALOG_H
#define BUILDCONFIGDIALOG_H

#include <QDialog>
#include <QVector>

#include "buildconfig.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

// Edits the build configurations of the current project: a list of
// configurations on the left, the selected one's directory and commands on
// the right.
class BuildConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BuildConfigDialog(const BuildConfigurations &configs, QWidget *parent = nullptr);

    QVector<BuildConfiguration> configurations() const { return m_configs; }
    int activeIndex() const { return m_active; }

private:
    void populateList();
    void showConfig(int row);
    void storeField();
    void updatePreview();
    void addConfig();
    void cloneConfig();
    void removeConfig();
    void resetToDetected();
    QString uniqueName(const QString &base) const;

    const BuildConfigurations &m_source;
    QVector<BuildConfiguration> m_configs;
    int m_active  = 0;
    int m_current = -1;
    bool m_loading = false;

    QListWidget *m_list       { nullptr };
    QPushButton *m_btnActive  { nullptr };
    QPushButton *m_btnRemove  { nullptr };
    QLineEdit   *m_name       { nullptr };
    QLineEdit   *m_buildDir   { nullptr };
    QLineEdit   *m_build      { nullptr };
    QLineEdit   *m_clean      { nullptr };
    QLineEdit   *m_run        { nullptr };
    QLineEdit   *m_debug      { nullptr };
    QCheckBox   *m_buildFirst { nullptr };
    QLabel      *m_preview    { nullptr };
};

#endif // BUILDCONFIGDIALOG_H
