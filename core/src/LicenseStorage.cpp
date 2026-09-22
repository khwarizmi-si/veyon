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

#include "LicenseStorage.h"

LicenseActivation::LicenseActivation() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::System,
						  QStringLiteral("KhwarizmiLicense"))
{
}

LicenseCache::LicenseCache() :
	Configuration::Object(Configuration::Store::Backend::JsonFile, Configuration::Store::Scope::User,
						  QStringLiteral("KhwarizmiLicense"))
{
}
