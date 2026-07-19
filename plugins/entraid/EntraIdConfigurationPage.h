/*
 * EntraIdConfigurationPage.h - configuration page for Microsoft Entra ID connector
 *
 * Copyright (c) 2026 Al-Khwarizmi
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "ConfigurationPage.h"

class EntraIdConfiguration;
class QCheckBox;
class QFormLayout;
class QLineEdit;
class QSpinBox;

class EntraIdConfigurationPage : public ConfigurationPage
{
	Q_OBJECT
public:
	explicit EntraIdConfigurationPage( EntraIdConfiguration& configuration, QWidget* parent = nullptr );
	~EntraIdConfigurationPage() override = default;

	void resetWidgets() override;
	void connectWidgetsToProperties() override;
	void applyConfiguration() override;

private:
	void testConnection();
	void addTextRow( QFormLayout* formLayout, const QString& label, QLineEdit* lineEdit );

	EntraIdConfiguration& m_configuration;
	QCheckBox* m_enabled = nullptr;
	QLineEdit* m_tenantId = nullptr;
	QLineEdit* m_clientId = nullptr;
	QLineEdit* m_clientSecret = nullptr;
	QLineEdit* m_authorityHost = nullptr;
	QLineEdit* m_graphBaseUrl = nullptr;
	QSpinBox* m_queryTimeout = nullptr;
	QLineEdit* m_deviceDisplayNameField = nullptr;
	QLineEdit* m_deviceHostAddressField = nullptr;
	QLineEdit* m_deviceMacAddressField = nullptr;
	QLineEdit* m_deviceLocationField = nullptr;
	QLineEdit* m_fallbackLocationName = nullptr;
	QCheckBox* m_useSecurityGroupsForAccessControl = nullptr;

};
