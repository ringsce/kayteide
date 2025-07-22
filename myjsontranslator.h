#ifndef MYJSONTRANSLATOR_H
#define MYJSONTRANSLATOR_H

#include <QTranslator>
#include <QJsonObject>
#include <QHash> // For faster lookups

class MyJsonTranslator : public QTranslator
{
    Q_OBJECT
public:
    explicit MyJsonTranslator(QObject *parent = nullptr);

    bool load(const QString &fileName); // Load JSON from file

    // Override the translate method
    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;

private:
    QHash<QString, QString> translations; // Store sourceText -> translation
};

#endif // MYJSONTRANSLATOR_H