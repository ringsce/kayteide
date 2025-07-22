#ifndef DELPHISYNTAXHIGHLIGHTER_H
#define DELPHISYNTAXHIGHLIGHTER_H

#include "pascalsyntaxhighlighter.h" // Inherit from PascalSyntaxHighlighter
#include <utility> // For std::as_const

class DelphiSyntaxHighlighter : public PascalSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit DelphiSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat delphiKeywordFormat; // For Delphi-specific keywords
    QTextCharFormat classFormat;         // For class names
    QTextCharFormat interfaceFormat;     // For interface names
    QTextCharFormat attributeFormat;     // For attributes or annotations

    void setupDelphiFormats();
    void setupDelphiRules();
};

#endif // DELPHISYNTAXHIGHLIGHTER_H