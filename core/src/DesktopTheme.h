#pragma once

#include <QIcon>
#include <QColor>

#include "VeyonCore.h"

class QApplication;

namespace DesktopTheme
{
VEYON_CORE_EXPORT void apply(QApplication* application, bool dark);
VEYON_CORE_EXPORT QIcon icon(const QIcon& source, const QColor& tint = {});
}
