/*
 * EntraIdGraphClient.h - minimal Microsoft Graph client for Entra ID connector
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

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>

class EntraIdConfiguration;

class EntraIdGraphClient : public QObject
{
	Q_OBJECT
public:
	explicit EntraIdGraphClient( const EntraIdConfiguration& configuration, QObject* parent = nullptr );

	bool isConfigured() const;
	bool testConnection( QString* errorString = nullptr );

	QJsonArray devices( QString* errorString = nullptr );
	QJsonArray securityGroups( QString* errorString = nullptr );
	QJsonArray groupsOfUser( const QString& username, QString* errorString = nullptr );

	static QString jsonStringValue( const QJsonObject& object, const QString& dottedPath );

private:
	bool ensureAccessToken( QString* errorString );
	QJsonObject requestJsonObject( const QUrl& url, QString* errorString );
	QJsonArray requestPagedValues( const QUrl& url, QString* errorString );
	QByteArray request( const QNetworkRequest& request,
						const QByteArray& body,
						const QByteArray& method,
						QString* errorString );
	QUrl graphUrl( const QString& path ) const;

	const EntraIdConfiguration& m_configuration;
	QNetworkAccessManager m_networkAccessManager;
	QString m_accessToken;
	QDateTime m_accessTokenExpiry;

};
