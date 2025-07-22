#include "myjsontranslator.h"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

MyJsonTranslator::MyJsonTranslator(QObject *parent) : QTranslator(parent)
{
}

bool MyJsonTranslator::load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open JSON translation file:" << fileName;
        return false;
    }

    QByteArray jsonData = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON translation file:" << fileName;
        return false;
    }

    translations.clear(); // Clear previous translations
    QJsonObject obj = doc.object();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        translations.insert(it.key(), it.value().toString());
    }

    return true;
}

QString MyJsonTranslator::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n) const
{
    Q_UNUSED(context); // Context isn't used in this simple JSON structure
    Q_UNUSED(disambiguation);
    Q_UNUSED(n);

    QString key = QString::fromUtf8(sourceText); // Use the sourceText as the key
    if (translations.contains(key)) {
        return translations.value(key);
    }
    return QString::fromUtf8(sourceText); // Fallback to sourceText if no translation found
}//
