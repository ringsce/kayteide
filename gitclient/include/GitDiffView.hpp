#pragma once
#include <QPlainTextEdit>
#include "GitRepository.hpp"

namespace Kayte {

/// Read-only diff viewer with line-level colour coding.
class GitDiffView : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit GitDiffView(QWidget *parent = nullptr);
    void showDiff(const QVector<DiffLine> &lines);
    void clear();
};

} // namespace Kayte
