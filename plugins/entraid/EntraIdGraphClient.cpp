/*
 * EntraIdGraphClient.cpp - minimal Microsoft Graph client for Entra ID connector
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

#include <QDateTime>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>

#include "EntraIdConfiguration.h"
#include "EntraIdGraphClient.h"
#include "VeyonCore.h"

namespace
{

QUrl normalizedUrl( const QString& value )
{
	auto url = QUrl( value.trimmed() );
	if( url.isValid() == false || url.scheme() != QStringLiteral("https") )
	{
		return {};
	}

	return url;
}

QString graphErrorMessage( const QByteArray& body )
{
	const auto object = QJsonDocument::fromJson( body ).object();
	const auto error = object.value( QStringLiteral("error") ).toObject();
	const auto message = error.value( QStringLiteral("message") ).toString();
	if( message.isEmpty() == false )
	{
		return message;
	}

	const auto description = object.value( QStringLiteral("error_description") ).toString();
	if( description.isEmpty() == false )
	{
		return description;
	}

	return QString::fromUtf8( body.left( 300 ) );
}

}


EntraIdGraphClient::EntraIdGraphClient( const EntraIdConfiguration& configuration, QObject* parent ) :
	QObject( parent ),
	m_configuration( configuration )
{
}



bool EntraIdGraphClient::isConfigured() const
{
	return m_configuration.enabled() &&
		   m_configuration.tenantId().trimmed().isEmpty() == false &&
		   m_configuration.clientId().trimmed().isEmpty() == false &&
		   m_configuration.clientSecret().plainText().toByteArray().isEmpty() == false &&
		   normalizedUrl( m_configuration.authorityHost() ).isValid() &&
		   normalizedUrl( m_configuration.graphBaseUrl() ).isValid();
}



bool EntraIdGraphClient::testConnection( QString* errorString )
{
	const auto values = devices( errorString );
	return errorString == nullptr || errorString->isEmpty() || values.isEmpty() == false;
}



QJsonArray EntraIdGraphClient::devices( QString* errorString )
{
	QUrl url = graphUrl( QStringLiteral("/devices") );
	QUrlQuery query;
	query.addQueryItem( QStringLiteral("$select"),
						QStringLiteral("id,deviceId,displayName,physicalIds,trustType,operatingSystem,extensionAttributes") );
	query.addQueryItem( QStringLiteral("$top"), QStringLiteral("999") );
	url.setQuery( query );

	return requestPagedValues( url, errorString );
}



QJsonArray EntraIdGraphClient::securityGroups( QString* errorString )
{
	QUrl url = graphUrl( QStringLiteral("/groups") );
	QUrlQuery query;
	query.addQueryItem( QStringLiteral("$select"), QStringLiteral("id,displayName,securityIdentifier,securityEnabled") );
	query.addQueryItem( QStringLiteral("$filter"), QStringLiteral("securityEnabled eq true") );
	query.addQueryItem( QStringLiteral("$top"), QStringLiteral("999") );
	url.setQuery( query );

	return requestPagedValues( url, errorString );
}



QJsonArray EntraIdGraphClient::groupsOfUser( const QString& username, QString* errorString )
{
	const auto strippedUsername = VeyonCore::stripDomain( username ).trimmed();
	if( strippedUsername.isEmpty() )
	{
		return {};
	}

	QUrl url = graphUrl( QStringLiteral("/users/%1/transitiveMemberOf/microsoft.graph.group")
						 .arg( QString::fromUtf8( QUrl::toPercentEncoding( strippedUsername ) ) ) );
	QUrlQuery query;
	query.addQueryItem( QStringLiteral("$select"), QStringLiteral("id,displayName,securityIdentifier,securityEnabled") );
	query.addQueryItem( QStringLiteral("$top"), QStringLiteral("999") );
	url.setQuery( query );

	return requestPagedValues( url, errorString );
}



QString EntraIdGraphClient::jsonStringValue( const QJsonObject& object, const QString& dottedPath )
{
	QJsonValue value = object;
	const auto parts = dottedPath.split( QLatin1Char('.'), Qt::SkipEmptyParts );
	for( const auto& part : parts )
	{
		if( value.isObject() == false )
		{
			return {};
		}
		value = value.toObject().value( part );
	}

	if( value.isString() )
	{
		return value.toString().trimmed();
	}
	if( value.isDouble() )
	{
		return QString::number( value.toDouble() );
	}
	if( value.isBool() )
	{
		return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
	}

	return {};
}



bool EntraIdGraphClient::ensureAccessToken( QString* errorString )
{
	if( isConfigured() == false )
	{
		if( errorString )
		{
			*errorString = tr( "Microsoft Entra ID connector is not configured." );
		}
		return false;
	}

	if( m_accessToken.isEmpty() == false &&
		QDateTime::currentDateTimeUtc().secsTo( m_accessTokenExpiry ) > 60 )
	{
		return true;
	}

	auto authorityUrl = normalizedUrl( m_configuration.authorityHost() );
	authorityUrl.setPath( QStringLiteral("/%1/oauth2/v2.0/token").arg( m_configuration.tenantId().trimmed() ) );

	QNetworkRequest request( authorityUrl );
	request.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded") );

	QUrlQuery body;
	body.addQueryItem( QStringLiteral("client_id"), m_configuration.clientId().trimmed() );
	body.addQueryItem( QStringLiteral("client_secret"),
					   QString::fromUtf8( m_configuration.clientSecret().plainText().toByteArray() ) );
	body.addQueryItem( QStringLiteral("scope"), QStringLiteral("https://graph.microsoft.com/.default") );
	body.addQueryItem( QStringLiteral("grant_type"), QStringLiteral("client_credentials") );

	const auto response = this->request( request, body.toString( QUrl::FullyEncoded ).toUtf8(),
										QByteArrayLiteral("POST"), errorString );
	if( response.isEmpty() )
	{
		return false;
	}

	const auto object = QJsonDocument::fromJson( response ).object();
	const auto accessToken = object.value( QStringLiteral("access_token") ).toString();
	if( accessToken.isEmpty() )
	{
		if( errorString )
		{
			*errorString = tr( "Microsoft Entra ID token response did not include an access token." );
		}
		return false;
	}

	const auto expiresIn = object.value( QStringLiteral("expires_in") ).toInt( 3600 );
	m_accessToken = accessToken;
	m_accessTokenExpiry = QDateTime::currentDateTimeUtc().addSecs( expiresIn );

	return true;
}



QJsonObject EntraIdGraphClient::requestJsonObject( const QUrl& url, QString* errorString )
{
	if( ensureAccessToken( errorString ) == false )
	{
		return {};
	}

	QNetworkRequest request( url );
	request.setRawHeader( QByteArrayLiteral("Authorization"),
						  QByteArrayLiteral("Bearer ") + m_accessToken.toUtf8() );
	request.setHeader( QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json") );

	const auto response = this->request( request, {}, QByteArrayLiteral("GET"), errorString );
	if( response.isEmpty() )
	{
		return {};
	}

	QJsonParseError parseError;
	const auto document = QJsonDocument::fromJson( response, &parseError );
	if( parseError.error != QJsonParseError::NoError || document.isObject() == false )
	{
		if( errorString )
		{
			*errorString = tr( "Microsoft Graph returned invalid JSON: %1" ).arg( parseError.errorString() );
		}
		return {};
	}

	return document.object();
}



QJsonArray EntraIdGraphClient::requestPagedValues( const QUrl& url, QString* errorString )
{
	QJsonArray values;
	auto nextUrl = url;
	while( nextUrl.isValid() )
	{
		const auto object = requestJsonObject( nextUrl, errorString );
		if( object.isEmpty() )
		{
			break;
		}

		const auto pageValues = object.value( QStringLiteral("value") ).toArray();
		for( const auto& value : pageValues )
		{
			if( value.isObject() )
			{
				values.append( value );
			}
		}

		const auto nextLink = object.value( QStringLiteral("@odata.nextLink") ).toString();
		if( nextLink.isEmpty() )
		{
			nextUrl = QUrl();
			continue;
		}

		const auto nextLinkUrl = QUrl( nextLink );
		const auto graphBaseUrl = normalizedUrl( m_configuration.graphBaseUrl() );
		if( nextLinkUrl.scheme() != QStringLiteral("https") ||
			nextLinkUrl.host().compare( graphBaseUrl.host(), Qt::CaseInsensitive ) != 0 )
		{
			if( errorString )
			{
				*errorString = tr( "Microsoft Graph returned an unexpected paging URL." );
			}
			break;
		}

		nextUrl = nextLinkUrl;
	}

	return values;
}



QByteArray EntraIdGraphClient::request( const QNetworkRequest& request,
										const QByteArray& body,
										const QByteArray& method,
										QString* errorString )
{
	QNetworkReply* reply = nullptr;
	if( method == QByteArrayLiteral("POST") )
	{
		reply = m_networkAccessManager.post( request, body );
	}
	else
	{
		reply = m_networkAccessManager.get( request );
	}

	QEventLoop eventLoop;
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot( true );
	timeoutTimer.setInterval( qBound( 5, m_configuration.queryTimeout(), 300 ) * 1000 );
	connect( &timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort );
	connect( reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit );
	timeoutTimer.start();
	eventLoop.exec();

	const auto data = reply->readAll();
	const auto statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
	const auto error = reply->error();
	reply->deleteLater();

	if( error != QNetworkReply::NoError || statusCode >= 400 )
	{
		if( errorString )
		{
			*errorString = tr( "Microsoft Graph request failed (%1): %2" )
						   .arg( statusCode ? statusCode : static_cast<int>( error ) )
						   .arg( graphErrorMessage( data ) );
		}
		return {};
	}

	return data;
}



QUrl EntraIdGraphClient::graphUrl( const QString& path ) const
{
	auto url = normalizedUrl( m_configuration.graphBaseUrl() );
	const auto basePath = url.path().endsWith( QLatin1Char('/') ) ?
							  url.path().left( url.path().size() - 1 ) :
							  url.path();
	url.setPath( basePath + path );
	return url;
}
