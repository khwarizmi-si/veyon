#include <QButtonGroup>
#include <QTest>
#include "ToolButton.h"

class ToolButtonTest : public QObject
{
	Q_OBJECT
private Q_SLOTS:
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
