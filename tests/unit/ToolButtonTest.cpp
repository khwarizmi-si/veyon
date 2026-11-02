#include <QButtonGroup>
#include <QTest>
#include <QTranslator>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolTip>
#include <cmath>
#include "DesktopTheme.h"
#include "ToolButton.h"

class ToolButtonTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
	void themeContrast_data()
	{
		QTest::addColumn<bool>("dark");
		QTest::newRow("light") << false;
		QTest::newRow("dark") << true;
	}

	void themeContrast()
	{
		QFETCH(bool, dark);
		const auto oldStyle = qApp->styleSheet();
		const auto oldPalette = qApp->palette();
		const auto oldFont = qApp->font();
		const auto oldTooltipPalette = QToolTip::palette();
		DesktopTheme::apply(qApp, dark);
		const auto style = qApp->styleSheet();
		const auto palette = qApp->palette();
		qApp->setStyleSheet(oldStyle);
		qApp->setPalette(oldPalette);
		qApp->setFont(oldFont);
		QToolTip::setPalette(oldTooltipPalette);
		const QRegularExpression focusRule(QStringLiteral("QLineEdit:focus[^\\{]+\\{ border-color: (#[0-9a-fA-F]{6});"));
		const QColor focus(focusRule.match(style).captured(1));
		QVERIFY(focus.isValid());
		auto luminance = [](const QColor& color) {
			auto linear = [](double channel) {
				return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
			};
			return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
		};
		auto contrast = [&luminance](const QColor& a, const QColor& b) {
			const auto first = luminance(a);
			const auto second = luminance(b);
			return (qMax(first, second) + 0.05) / (qMin(first, second) + 0.05);
		};
		QVERIFY(contrast(focus, palette.color(QPalette::Base)) >= 3.0);
		QVERIFY(contrast(palette.color(QPalette::Highlight), palette.color(QPalette::HighlightedText)) >= 4.5);
	}

	void featureButtonsAreKeyboardReachable()
	{
		ToolButton button({}, QStringLiteral("Pantau"), {}, {}, {});
		QVERIFY(button.focusPolicy() & Qt::TabFocus);
	}

	void standardButtonsUseIndonesian()
	{
		const auto path = qEnvironmentVariable("VEYON_TEST_ID_QM");
		if (path.isEmpty())
		{
			QSKIP("Set VEYON_TEST_ID_QM to verify translated standard buttons");
		}
		QTranslator translator;
		QVERIFY(translator.load(path));
		QCoreApplication::installTranslator(&translator);
		QDialogButtonBox buttons(QDialogButtonBox::Close | QDialogButtonBox::Cancel);
		const auto close = buttons.button(QDialogButtonBox::Close)->text().remove(QLatin1Char('&'));
		const auto cancel = buttons.button(QDialogButtonBox::Cancel)->text().remove(QLatin1Char('&'));
		QCoreApplication::removeTranslator(&translator);
		QCOMPARE(close, QStringLiteral("Tutup"));
		QCOMPARE(cancel, QStringLiteral("Batal"));
	}

	void activeLabelTracksMode()
	{
		ToolButton audio({}, QStringLiteral("Bisukan audio"), QStringLiteral("Aktifkan audio"), {}, {});
		ToolButton monitor({}, QStringLiteral("Pantau"), {}, {}, {});
		audio.setCheckable(true);
		monitor.setCheckable(true);
		QButtonGroup modes;
		modes.addButton(&audio);
		modes.addButton(&monitor);
		audio.setChecked(true);
		QCOMPARE(audio.text(), QStringLiteral("Aktifkan audio"));
		monitor.setChecked(true);
		QCOMPARE(audio.text(), QStringLiteral("Bisukan audio"));
	}
};

QTEST_MAIN(ToolButtonTest)
#include "ToolButtonTest.moc"
