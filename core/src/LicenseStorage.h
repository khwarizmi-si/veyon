/*
 * LicenseStorage.h - where licence data lives
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "Configuration/Object.h"
#include "Configuration/Property.h"

// Deliberately NOT part of VeyonConfiguration: "Save settings to file" exports
// VeyonCore::config() wholesale, which would put the master secret in every
// exported file and deployment template. These stores are never exported.

#define FOREACH_LICENSE_ACTIVATION_PROPERTY(OP) \
	OP(LicenseActivation, LicenseActivation(), QString, masterId, setMasterId, "MasterId", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, secret, setSecret, "Secret", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, activationToken, setActivationToken, "ActivationToken", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseActivation, LicenseActivation(), QString, serverUrl, setServerUrl, "ServerUrl", "License", QStringLiteral("https://license.khwarizmi.co.id"), Configuration::Property::Flag::Hidden)

#define FOREACH_LICENSE_CACHE_PROPERTY(OP) \
	OP(LicenseCache, LicenseCache(), QString, refreshedToken, setRefreshedToken, "RefreshedToken", "License", QString(), Configuration::Property::Flag::Hidden) \
	OP(LicenseCache, LicenseCache(), QString, lastServerTime, setLastServerTime, "LastServerTime", "License", QString(), Configuration::Property::Flag::Hidden)

// clazy:excludeall=ctor-missing-parent-argument,copyable-polymorphic

// System scope: written by the Configurator (administrator) at activation.
class VEYON_CORE_EXPORT LicenseActivation : public Configuration::Object
{
	Q_OBJECT
public:
	LicenseActivation();
	FOREACH_LICENSE_ACTIVATION_PROPERTY(DECLARE_CONFIG_PROPERTY)
};

// User scope: the master runs as the teacher and cannot write system config,
// so refreshed tokens from daily check-ins live here.
class VEYON_CORE_EXPORT LicenseCache : public Configuration::Object
{
	Q_OBJECT
public:
	LicenseCache();
	FOREACH_LICENSE_CACHE_PROPERTY(DECLARE_CONFIG_PROPERTY)
};
