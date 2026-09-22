/*
 * LicenseClient.cpp - HTTP client for the Khwarizmi licence backend
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <optional>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include "LicenseClient.h"

namespace LicenseClientDetail
{

static std::optional<QJsonObject> parseObject(const QByteArray& body)
{
	QJsonParseError error{};
	const auto document = QJsonDocument::fromJson(body, &error);
	if (error.error != QJsonParseError::NoError || document.isObject() == false)
	{
		return std::nullopt;
	}
	return document.object();
}

static LicenseCallStatus statusFor(int httpStatus)
{
	if (httpStatus == 0)
	{
		return LicenseCallStatus::NetworkError;
	}
	if (httpStatus == 401 || httpStatus == 403)
	{
		return LicenseCallStatus::Unauthorized;
	}
	return LicenseCallStatus::ServerError;
}

}

LicenseClient::LicenseClient(const QString& serverUrl) :
	m_serverUrl(serverUrl)
{
}

QUrl LicenseClient::endpoint(const QString& serverUrl, const QString& path)
{
	QUrl url(serverUrl);
	if (url.isValid() == false || url.scheme() != QStringLiteral("https") || url.host().isEmpty())
	{
		return {};
	}
	QString basePath = url.path();
	while (basePath.endsWith(QLatin1Char('/')))
	{
		basePath.chop(1);
	}
	url.setPath(basePath + path);
	return url;
}

QByteArray LicenseClient::buildActivateBody(const QString& code, const QString& hostname, const QString& version)
{
	return QJsonDocument(QJsonObject{
		{ QStringLiteral("code"), code },
		{ QStringLiteral("hostname"), hostname },
		{ QStringLiteral("version"), version },
	}).toJson(QJsonDocument::Compact);
}

QByteArray LicenseClient::buildCheckInBody(const QString& masterId, const QList<LicenseDevice>& devices,
										   const QString& hostname, const QString& version)
{
	QJsonArray deviceArray;
	for (const auto& device : devices)
	{
		deviceArray.append(QJsonObject{
			{ QStringLiteral("mac"), device.mac.isEmpty() ? QJsonValue() : QJsonValue(device.mac) },
			{ QStringLiteral("hostname"), device.hostname },
		});
	}
	// INVARIANT: only licence fields. Never monitoring data.
	return QJsonDocument(QJsonObject{
		{ QStringLiteral("master_id"), masterId },
		{ QStringLiteral("devices"), deviceArray },
		{ QStringLiteral("hostname"), hostname },
		{ QStringLiteral("version"), version },
	}).toJson(QJsonDocument::Compact);
}

ActivationResponse LicenseClient::parseActivateResponse(int httpStatus, const QByteArray& body)
{
	using namespace LicenseClientDetail;
	ActivationResponse response;

	if (httpStatus == 200)
	{
		const auto object = parseObject(body);
		const QString masterId = object ? object->value(QStringLiteral("master_id")).toString() : QString();
		const QString secret = object ? object->value(QStringLiteral("secret")).toString() : QString();
		const QString token = object ? object->value(QStringLiteral("token")).toString() : QString();
		if (masterId.isEmpty() || secret.isEmpty() || token.isEmpty())
		{
			response.status = LicenseCallStatus::InvalidResponse;
			return response;
		}
		response.status = LicenseCallStatus::Ok;
		response.masterId = masterId;
		response.secret = secret;
		response.token = token;
		return response;
	}

	if (httpStatus == 400)
	{
		const auto object = parseObject(body);
		response.status = object && object->value(QStringLiteral("error")).toString() == QStringLiteral("invalid_or_used_code")
							  ? LicenseCallStatus::InvalidCode
							  : LicenseCallStatus::ServerError;
		return response;
	}

	response.status = statusFor(httpStatus);
	return response;
}

CheckInResponse LicenseClient::parseCheckInResponse(int httpStatus, const QByteArray& body)
{
	using namespace LicenseClientDetail;
	CheckInResponse response;

	if (httpStatus == 200)
	{
		const auto object = parseObject(body);
		const QString token = object ? object->value(QStringLiteral("token")).toString() : QString();
		response.status = token.isEmpty() ? LicenseCallStatus::InvalidResponse : LicenseCallStatus::Ok;
		response.token = token;
		return response;
	}

	response.status = statusFor(httpStatus);
	return response;
}

ActivationResponse LicenseClient::activate(const QString& code, const QString& hostname, const QString& version)
{
	QByteArray body;
	const auto httpStatus = post(QStringLiteral("/v1/activate"), buildActivateBody(code, hostname, version), {}, body);
	return parseActivateResponse(httpStatus, body);
}

CheckInResponse LicenseClient::checkIn(const QString& masterId, const QString& secret,
									   const QList<LicenseDevice>& devices,
									   const QString& hostname, const QString& version)
{
	QByteArray body;
	const auto httpStatus = post(QStringLiteral("/v1/checkin"), buildCheckInBody(masterId, devices, hostname, version),
								 secret.toUtf8(), body);
	return parseCheckInResponse(httpStatus, body);
}

QNetworkRequest LicenseClient::buildRequest(const QUrl& url, const QByteArray& bearer)
{
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
	if (bearer.isEmpty() == false)
	{
		request.setRawHeader(QByteArrayLiteral("Authorization"), QByteArrayLiteral("Bearer ") + bearer);
	}
	// I3: Qt 6 follows redirects by default (NoLessSafeRedirectPolicy) and the
	// request may carry the master secret in the Authorization header. Never
	// follow a redirect; a 3xx response is then mapped to ServerError.
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
	return request;
}

int LicenseClient::post(const QString& path, const QByteArray& payload, const QByteArray& bearer, QByteArray& body)
{
	const auto url = endpoint(m_serverUrl, path);
	if (url.isValid() == false)
	{
		return 0;
	}

	const auto request = buildRequest(url, bearer);

	QNetworkAccessManager networkAccessManager;
	auto reply = networkAccessManager.post(request, payload);

	QEventLoop eventLoop;
	QTimer timeoutTimer;
	timeoutTimer.setSingleShot(true);
	timeoutTimer.setInterval(RequestTimeoutMsecs);
	QObject::connect(&timeoutTimer, &QTimer::timeout, reply, &QNetworkReply::abort);
	QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
	timeoutTimer.start();
	eventLoop.exec();

	body = reply->readAll();
	const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	reply->deleteLater();
	return httpStatus;
}
