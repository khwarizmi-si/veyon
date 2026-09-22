/*
 * LicensePage.h - licence activation and status page
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "ConfigurationPage.h"

class QLabel;
class QLineEdit;
class QPushButton;

class LicensePage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit LicensePage(QWidget* parent = nullptr);

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void refresh();
	void activate();
	void checkNow();
	void setBusy(bool busy);

	QLabel* m_statusLabel{nullptr};
	QLabel* m_detailsLabel{nullptr};
	QLabel* m_hintLabel{nullptr};
	QLabel* m_resultLabel{nullptr};
	QLineEdit* m_codeEdit{nullptr};
	QPushButton* m_activateButton{nullptr};
	QPushButton* m_checkButton{nullptr};
};
