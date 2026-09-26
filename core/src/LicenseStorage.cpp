/*
 * LicenseStorage.cpp - where licence data lives
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFile>

#include "LicenseStorage.h"
#include "PlatformFilesystemFunctions.h"
#include "PlatformPluginInterface.h"
#include "VeyonCore.h"

LicenseActivation::LicenseActivation() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::System,
						  QStringLiteral("KhwarizmiLicense"))
{
}

bool LicenseActivation::flushAndProtect()
{
	const auto path = storeFilePath();
	if (path.isEmpty())
	{
		return false;
	}
	if (!QFile::exists(path))
	{
		QFile file(path);
		if (!file.open(QFile::WriteOnly))
		{
			return false;
		}
	}
#if defined(Q_OS_WIN)
	// The Windows adapter maps Group flags to owner rights and always grants
	// the local Administrators group full access.
	const auto permissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::WriteGroup;
#else
	const auto permissions = QFile::ReadOwner | QFile::WriteOwner;
#endif
	if (!VeyonCore::platform().filesystemFunctions().setFileOwnerGroupPermissions(path, permissions))
	{
		return false;
	}
	flushStore();
	return VeyonCore::platform().filesystemFunctions().setFileOwnerGroupPermissions(path, permissions);
}

LicensePublic::LicensePublic() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::System,
					  QStringLiteral("KhwarizmiLicensePublic"))
{
}

LicenseCache::LicenseCache() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::User,
						  QStringLiteral("KhwarizmiLicense"))
{
}
