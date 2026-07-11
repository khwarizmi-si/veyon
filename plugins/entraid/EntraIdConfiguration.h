/*
 * EntraIdConfiguration.h - configuration values for Microsoft Entra ID connector
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

#include "Configuration/Proxy.h"

#define FOREACH_ENTRA_ID_CONFIG_PROPERTY(OP) \
	OP( EntraIdConfiguration, m_configuration, bool, enabled, setEnabled, "Enabled", "EntraID", false, Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, tenantId, setTenantId, "TenantId", "EntraID", QString(), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, clientId, setClientId, "ClientId", "EntraID", QString(), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, Configuration::Password, clientSecret, setClientSecret, "ClientSecret", "EntraID", QString(), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, authorityHost, setAuthorityHost, "AuthorityHost", "EntraID", QStringLiteral("https://login.microsoftonline.com"), Configuration::Property::Flag::Advanced ) \
	OP( EntraIdConfiguration, m_configuration, QString, graphBaseUrl, setGraphBaseUrl, "GraphBaseUrl", "EntraID", QStringLiteral("https://graph.microsoft.com/v1.0"), Configuration::Property::Flag::Advanced ) \
	OP( EntraIdConfiguration, m_configuration, int, queryTimeout, setQueryTimeout, "QueryTimeout", "EntraID", 30, Configuration::Property::Flag::Advanced ) \
	OP( EntraIdConfiguration, m_configuration, QString, deviceDisplayNameField, setDeviceDisplayNameField, "DeviceDisplayNameField", "EntraID", QStringLiteral("displayName"), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, deviceHostAddressField, setDeviceHostAddressField, "DeviceHostAddressField", "EntraID", QStringLiteral("displayName"), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, deviceMacAddressField, setDeviceMacAddressField, "DeviceMacAddressField", "EntraID", QString(), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, deviceLocationField, setDeviceLocationField, "DeviceLocationField", "EntraID", QStringLiteral("extensionAttributes.extensionAttribute1"), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, QString, fallbackLocationName, setFallbackLocationName, "FallbackLocationName", "EntraID", QStringLiteral("Entra ID devices"), Configuration::Property::Flag::Standard ) \
	OP( EntraIdConfiguration, m_configuration, bool, useSecurityGroupsForAccessControl, setUseSecurityGroupsForAccessControl, "UseSecurityGroupsForAccessControl", "EntraID", true, Configuration::Property::Flag::Standard ) \

DECLARE_CONFIG_PROXY(EntraIdConfiguration, FOREACH_ENTRA_ID_CONFIG_PROPERTY)
