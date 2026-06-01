#pragma once
// SvnStatusDelegate.h – Paints a colored badge in the Status column
// C++17, Qt 6

#include "SvnTypes.h"
#include <QStyledItemDelegate>

namespace Kayte::Svn {

class SvnStatusDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const override;
};

} // namespace Kayte::Svn
