/*
 * LicensePage.cpp - licence activation and status page
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent>

#include "LicensePage.h"
#include "LicenseService.h"

LicensePage::LicensePage(QWidget* parent) :
	ConfigurationPage(parent)
{
	setWindowTitle(tr("Lisensi"));
	setWindowIcon(QIcon(QStringLiteral(":/core/license.png")));

	auto layout = new QVBoxLayout(this);

	auto statusBox = new QGroupBox(tr("Status lisensi"), this);
	auto statusLayout = new QVBoxLayout(statusBox);
	m_statusLabel = new QLabel(statusBox);
	m_statusLabel->setWordWrap(true);
	m_detailsLabel = new QLabel(statusBox);
	m_detailsLabel->setWordWrap(true);
	m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	// I4: the Configurator runs elevated as the administrator, so this page
	// can only ever show status as of the last check-in, not live.
	m_hintLabel = new QLabel(tr("Status sesuai pemeriksaan terakhir. Periksa lagi untuk memperbaruinya."), statusBox);
	m_hintLabel->setWordWrap(true);
	m_checkButton = new QPushButton(tr("Periksa sekarang"), statusBox);
	statusLayout->addWidget(m_statusLabel);
	statusLayout->addWidget(m_detailsLabel);
	statusLayout->addWidget(m_hintLabel);
	statusLayout->addWidget(m_checkButton, 0, Qt::AlignLeft);

	auto activationBox = new QGroupBox(tr("Aktivasi"), this);
	auto activationLayout = new QFormLayout(activationBox);
	m_codeEdit = new QLineEdit(activationBox);
	m_codeEdit->setPlaceholderText(QStringLiteral("KHW-..."));
	m_activateButton = new QPushButton(tr("Aktifkan lisensi"), activationBox);
	activationLayout->addRow(tr("Kode aktivasi"), m_codeEdit);
	activationLayout->addRow(QString(), m_activateButton);

	m_resultLabel = new QLabel(this);
	m_resultLabel->setWordWrap(true);

	layout->addWidget(statusBox);
	layout->addWidget(activationBox);
	layout->addWidget(m_resultLabel);
	layout->addStretch();

	connect(m_activateButton, &QPushButton::clicked, this, &LicensePage::activate);
	connect(m_checkButton, &QPushButton::clicked, this, &LicensePage::checkNow);

	refresh();
}

void LicensePage::resetWidgets()
{
	refresh();
}

void LicensePage::connectWidgetsToProperties()
{
}

void LicensePage::applyConfiguration()
{
}

void LicensePage::refresh()
{
	const auto snapshot = LicenseService::snapshot();
	m_statusLabel->setText(LicenseService::describe(snapshot.state));

	if (!snapshot.claims)
	{
		m_detailsLabel->clear();
		return;
	}

	const auto& c = *snapshot.claims;
	const auto format = [](const QDateTime& dateTime) -> QString {
		return QLocale().toString(dateTime.toLocalTime(), QLocale::ShortFormat);
	};
	QString details = tr("Lembaga: %1").arg(c.tenantName);
	details += QLatin1Char('\n');
	details += tr("Kuota PC: %1").arg(c.maxDevices);
	details += QLatin1Char('\n');
	details += tr("Berlaku hingga: %1").arg(format(c.subscriptionEnd));
	details += QLatin1Char('\n');
	details += tr("Perlu verifikasi online sebelum: %1").arg(format(c.expiresAt));
	details += QLatin1Char('\n');
	details += tr("ID Master: %1").arg(snapshot.masterId);
	m_detailsLabel->setText(details);
}

void LicensePage::activate()
{
	const QString code = m_codeEdit->text().trimmed();
	if (code.isEmpty())
	{
		return;
	}
	setBusy(true);
	auto* watcher = new QFutureWatcher<LicenseActionResult>(this);
	connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
		const auto result = watcher->result();
		setBusy(false);
		m_resultLabel->setText(LicenseService::describe(result));
		if (result == LicenseActionResult::Ok)
		{
			m_codeEdit->clear();
		}
		refresh();
		watcher->deleteLater();
	});
	watcher->setFuture(QtConcurrent::run([code]() { return LicenseService::activate(code); }));
}

void LicensePage::checkNow()
{
	setBusy(true);
	auto* watcher = new QFutureWatcher<LicenseActionResult>(this);
	connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
		setBusy(false);
		m_resultLabel->setText(LicenseService::describe(watcher->result()));
		refresh();
		watcher->deleteLater();
	});
	watcher->setFuture(QtConcurrent::run([]() { return LicenseService::checkIn({}); }));
}

void LicensePage::setBusy(bool busy)
{
	m_activateButton->setEnabled(!busy);
	m_checkButton->setEnabled(!busy);
	m_codeEdit->setEnabled(!busy);
}
