// SvnStatusDelegate.cpp
// C++17, Qt 6

#include "SvnStatusDelegate.h"
#include "SvnModel.h"

#include <QPainter>
#include <QStyleOptionViewItem>

namespace Kayte::Svn {

static QColor badgeColor(StatusKind k) {
    switch (k) {
    case StatusKind::Added:       return { 0x4caf50 }; // green
    case StatusKind::Deleted:     return { 0xf44336 }; // red
    case StatusKind::Modified:    return { 0x2196f3 }; // blue
    case StatusKind::Conflicted:  return { 0xff9800 }; // orange
    case StatusKind::Missing:     return { 0xe91e63 }; // pink
    case StatusKind::Replaced:    return { 0x9c27b0 }; // purple
    case StatusKind::Unversioned: return { 0x9e9e9e }; // grey
    case StatusKind::Normal:      return { 0xbdbdbd }; // light grey
    default:                      return Qt::transparent;
    }
}

static QString badgeLetter(StatusKind k) {
    switch (k) {
    case StatusKind::Added:       return QStringLiteral("A");
    case StatusKind::Deleted:     return QStringLiteral("D");
    case StatusKind::Modified:    return QStringLiteral("M");
    case StatusKind::Conflicted:  return QStringLiteral("C");
    case StatusKind::Missing:     return QStringLiteral("!");
    case StatusKind::Replaced:    return QStringLiteral("R");
    case StatusKind::Unversioned: return QStringLiteral("?");
    case StatusKind::Normal:      return QStringLiteral("·");
    default:                      return {};
    }
}

void SvnStatusDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    const auto kind = index.data(SvnModel::StatusRole).value<StatusKind>();
    const QString letter = badgeLetter(kind);
    const QColor  color  = badgeColor(kind);

    painter->save();
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if (color != Qt::transparent && !letter.isEmpty()) {
        const QRect r = option.rect.adjusted(3, 3, -3, -3);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(color);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(r, 3, 3);

        painter->setPen(Qt::white);
        QFont f = painter->font();
        f.setBold(true);
        f.setPointSize(8);
        painter->setFont(f);
        painter->drawText(r, Qt::AlignCenter, letter);
    }
    painter->restore();
}

QSize SvnStatusDelegate::sizeHint(const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    Q_UNUSED(option); Q_UNUSED(index);
    return { 28, 22 };
}

} // namespace Kayte::Svn
