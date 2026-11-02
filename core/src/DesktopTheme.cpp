#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QPalette>
#include <QToolTip>

#include "DesktopTheme.h"

void DesktopTheme::apply(QApplication* application, bool dark)
{
	const auto canvas = dark ? QStringLiteral("#18181b") : QStringLiteral("#f8fafc");
	const auto surface = dark ? QStringLiteral("#2c2c2e") : QStringLiteral("#ffffff");
	const auto ink = dark ? QStringLiteral("#f2f2f7") : QStringLiteral("#1d1d1f");
	const auto border = dark ? QStringLiteral("#45454a") : QStringLiteral("#d5d5d9");
	const auto accent = QStringLiteral("#15803d");
	const auto selected = dark ? QStringLiteral("#173c2b") : QStringLiteral("#dcfce7");
	const auto muted = dark ? QStringLiteral("#b8b8be") : QStringLiteral("#64646a");
	const auto sidebar = dark ? QStringLiteral("#242426") : QStringLiteral("#ffffff");
	const auto focus = dark ? QStringLiteral("#4ade80") : accent;

	auto font = application->font();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	const auto families = QFontDatabase::families();
#else
	const auto families = QFontDatabase().families();
#endif
	for (const auto& family : {QStringLiteral("DIN Rounded"), QStringLiteral("Nunito"), QStringLiteral("Segoe UI"),
							 QStringLiteral("Inter"), QStringLiteral("Noto Sans")})
	{
		if (families.contains(family))
		{
			font.setFamily(family);
			break;
		}
	}
	font.setPointSize(10);
	application->setFont(font);

	auto palette = application->palette();
	palette.setColor(QPalette::Window, QColor(canvas));
	palette.setColor(QPalette::WindowText, QColor(ink));
	palette.setColor(QPalette::Base, QColor(surface));
	palette.setColor(QPalette::AlternateBase, QColor(canvas));
	palette.setColor(QPalette::Text, QColor(ink));
	palette.setColor(QPalette::Button, QColor(surface));
	palette.setColor(QPalette::ButtonText, QColor(ink));
	palette.setColor(QPalette::Highlight, QColor(accent));
	palette.setColor(QPalette::HighlightedText, Qt::white);
	palette.setColor(QPalette::Disabled, QPalette::Text, QColor(muted));
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(muted));
	palette.setColor(QPalette::ToolTipBase, QColor(surface));
	palette.setColor(QPalette::ToolTipText, QColor(ink));
	application->setPalette(palette);
	QToolTip::setPalette(palette);

	application->setStyleSheet(QStringLiteral(R"(
QWidget { color: %3; }
QMainWindow, QDialog, QWidget#centralWidget, QWidget#centralwidget { background: %1; }
QToolBar { background: %8; border: 0; border-bottom: 1px solid %4; padding: 8px 12px; spacing: 6px; }
QToolBar::separator { width: 1px; background: %4; margin: 6px 8px; }
QToolBar QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 5px 9px; min-height: 24px; font-weight: 500; }
QToolBar QToolButton:hover, QToolBar QToolButton:pressed { background: %2; border-color: %4; }
QToolBar QToolButton:checked { background: %6; color: %3; border-color: %6; }
QToolBar QToolButton#qt_toolbar_ext_button { min-width: 24px; }
QMenuBar { background: %8; border: 0; padding: 2px 6px; }
QMenuBar::item { padding: 5px 10px; background: transparent; }
QMenuBar::item:selected { background: %6; border-radius: 5px; }
QMenu { background: %2; border: 1px solid %4; border-radius: 8px; padding: 5px; }
QMenu::item { padding: 6px 24px 6px 12px; border-radius: 4px; }
QMenu::item:selected { background: %5; color: white; }
QMenu::item:disabled { color: %7; }
QMenu::separator { height: 1px; background: %4; margin: 5px 8px; }
QStatusBar { background: %8; border-top: 1px solid %4; padding: 4px 8px; }
QStatusBar::item { border: 0; }
QToolButton { padding: 4px; border: 1px solid transparent; border-radius: 5px; background: transparent; }
QToolButton:hover { background: %6; }
QToolButton:checked { background: %6; color: %3; }
QWidget#panelButtons { background: %1; border: 1px solid %4; border-radius: 6px; }
QWidget#panelButtons QToolButton { padding: 4px 8px; }
QWidget#panelButtons QToolButton:checked { background: %2; border-color: %4; color: %3; }
QPushButton { background: %2; border: 1px solid %4; border-radius: 16px; padding: 7px 14px; min-height: 20px; font-weight: 600; }
QPushButton:hover { border-color: %7; }
QPushButton:pressed { background: %6; padding-top: 9px; padding-bottom: 5px; }
QPushButton:default { background: %5; color: white; border-color: %5; font-weight: 600; }
QPushButton:disabled { color: %7; background: %1; }
QPushButton:focus, QToolButton:focus { border-color: %9; border-style: dashed; }
QLineEdit, QComboBox, QAbstractSpinBox, QTextEdit, QPlainTextEdit { background: %2; color: %3; border: 1px solid %4; border-radius: 16px; padding: 7px 10px; selection-background-color: %5; selection-color: white; }
QLineEdit:focus, QComboBox:focus, QAbstractSpinBox:focus, QTextEdit:focus, QPlainTextEdit:focus { border-color: %9; }
QLineEdit:disabled, QComboBox:disabled, QAbstractSpinBox:disabled { background: %1; color: %7; }
QComboBox { padding-right: 24px; }
QComboBox QAbstractItemView { background: %2; selection-background-color: %5; selection-color: white; }
QLabel:disabled, QCheckBox:disabled, QRadioButton:disabled { color: %7; }
QCheckBox, QRadioButton { spacing: 7px; }
QGroupBox { background: transparent; border: 0; border-top: 1px solid %4; border-radius: 0; margin-top: 22px; padding: 16px 0 8px; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 4px; padding: 0 6px; }
QGroupBox QLabel, QGroupBox QCheckBox, QGroupBox QRadioButton { font-weight: 400; }
QListView, QTreeView, QTableView { background: %2; alternate-background-color: %1; border: 0; outline: 0; selection-background-color: %6; selection-color: %3; }
QTreeView::item, QListView::item { padding: 4px; border-radius: 4px; }
QTreeView::item:hover, QListView::item:hover { background: %1; }
QTreeView::item:selected, QListView::item:selected { background: %6; color: %3; }
QHeaderView::section { background: %1; color: %7; border: 0; border-bottom: 1px solid %4; padding: 8px; font-weight: 600; }
QListWidget#pageSelector { background: %8; border: 0; border-right: 1px solid %4; padding: 12px 8px; }
QListWidget#pageSelector::item { padding: 5px 8px; margin: 1px 0; min-height: 24px; border-radius: 6px; }
QListWidget#pageSelector::item:hover { background: %1; }
QListWidget#pageSelector::item:selected { background: %6; color: %3; font-weight: 600; }
QLabel#settingsHeading { font-size: 21px; font-weight: 600; padding: 8px 0 6px; }
QLabel#navigationTitle { font-size: 18px; font-weight: 600; padding: 0 16px; }
QLabel#sectionHeading { font-size: 14px; font-weight: 600; }
QLabel#workspaceTitle { font-size: 20px; font-weight: 600; }
QWidget#workspaceHeader { background: %2; border-bottom: 1px solid %4; }
QToolBar QToolButton#MonitoringMode:!focus { background: %6; border-color: %5; }
QToolBar QToolButton#RemoteView:!focus { background: %2; border-color: #60a5fa; }
QToolBar QToolButton#RemoteControl:!focus { background: %2; border-color: #a78bfa; }
QToolBar QToolButton#Chat:!focus { background: %2; border-color: #34d399; }
QToolBar QToolButton#Screenshot:!focus { background: %2; border-color: #fb923c; }
QToolBar QToolButton { border-radius: 16px; padding: 7px 12px; }
QToolBar QToolButton:focus { background: %6; border-color: %9; border-style: dashed; }
QWidget#targetBar { background: %6; border-bottom: 1px solid %4; }
QLabel#settingsBrand { color: %7; font-size: 11px; padding: 8px 12px; }
QScrollArea#scrollArea, QWidget#scrollAreaWidgetContents, QStackedWidget#configPages { background: %1; border: 0; }
QWidget#ComputerSelectPanel, QWidget#ScreenshotManagementPanel { background: %8; }
QWidget#ComputerSelectPanel QTreeView { background: %8; }
QListView#computerMonitoringWidget { background: %1; border: 0; }
QSplitter::handle { background: %4; }
QSplitter::handle:hover { background: %5; }
QTabWidget::pane { background: %2; border: 1px solid %4; border-radius: 6px; }
QTabBar::tab { background: %8; border: 1px solid transparent; padding: 6px 12px; }
QTabBar::tab:selected { background: %2; color: %3; border-color: %4; border-radius: 5px; }
QProgressBar { background: %8; border: 1px solid %4; border-radius: 4px; text-align: center; }
QProgressBar::chunk { background: %5; border-radius: 3px; }
QSlider::groove:horizontal { height: 4px; background: %4; border-radius: 2px; }
QSlider::sub-page:horizontal { background: %5; border-radius: 2px; }
QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0; background: %2; border: 1px solid %7; border-radius: 7px; }
QToolTip { background: %2; color: %3; border: 1px solid %4; border-radius: 5px; padding: 6px 9px; }
)").arg(canvas, surface, ink, border, accent, selected, muted, sidebar, focus));
}

QIcon DesktopTheme::icon(const QIcon& source, const QColor& tint)
{
	if (source.isNull() || !tint.isValid())
	{
		return source;
	}
	QIcon result;
	const auto dark = VeyonCore::useDarkMode();
	for (const auto size : {16, 20, 24, 32, 40, 48})
	{
		for (const auto mode : {QIcon::Normal, QIcon::Active, QIcon::Selected, QIcon::Disabled})
		{
			for (const auto state : {QIcon::Off, QIcon::On})
			{
				auto pixmap = source.pixmap(size, size);
				const auto color = mode == QIcon::Selected ? QColor(dark ? QStringLiteral("#86efac") : QStringLiteral("#166534")) :
					mode == QIcon::Disabled ? QColor(QStringLiteral("#8e8e93")) :
					state == QIcon::On ? QColor(dark ? QStringLiteral("#86efac") : QStringLiteral("#15803d")) :
					tint.isValid() ? tint :
					QColor(dark ? QStringLiteral("#dedee3") : QStringLiteral("#515157"));
				QPainter painter(&pixmap);
				painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
				painter.fillRect(pixmap.rect(), color);
				painter.end();
				result.addPixmap(pixmap, mode, state);
			}
		}
	}
	return result;
}
