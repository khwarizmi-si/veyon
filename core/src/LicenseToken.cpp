/*
 * LicenseToken.cpp - verification of Khwarizmi licence tokens
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <cmath>
#include <limits>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "LicenseToken.h"

// Named, not anonymous: the unity build merges translation units.
namespace LicenseTokenDetail
{

static std::optional<QByteArray> decodeSegment(QByteArray segment)
{
	if (segment.isEmpty() || segment.size() % 4 == 1)
	{
		return std::nullopt;
	}
	while (segment.size() % 4 != 0)
	{
		segment.append('=');
	}
	const auto result = QByteArray::fromBase64Encoding(
		segment, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
	if (result.decodingStatus != QByteArray::Base64DecodingStatus::Ok)
	{
		return std::nullopt;
	}
	return result.decoded;
}

static std::optional<QJsonObject> parseObject(const QByteArray& json)
{
	QJsonParseError error{};
	const auto document = QJsonDocument::fromJson(json, &error);
	if (error.error != QJsonParseError::NoError || document.isObject() == false)
	{
		return std::nullopt;
	}
	return document.object();
}

static QDateTime parseIsoUtc(const QJsonValue& value)
{
	if (value.isString() == false)
	{
		return {};
	}
	const auto dateTime = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
	// A string without a "Z" or explicit offset parses as local time, which
	// would make licence dates depend on the machine's timezone. The backend
	// always emits an offset, so reject anything that didn't carry one.
	if (!dateTime.isValid() || dateTime.timeSpec() == Qt::LocalTime)
	{
		return {};
	}
	return dateTime.toUTC();
}

// 9999-12-31T23:59:59Z: far beyond any sane licence expiry, and comfortably
// inside qint64's range so the cast below is never UB.
static constexpr double MaxUnixSeconds = 253402300799.0;

static QDateTime parseUnixSeconds(const QJsonValue& value)
{
	if (value.isDouble() == false)
	{
		return {};
	}
	const auto seconds = value.toDouble();
	if (seconds < 0 || seconds > MaxUnixSeconds || seconds != std::floor(seconds))
	{
		return {};
	}
	return QDateTime::fromSecsSinceEpoch(qint64(seconds)).toUTC();
}

static bool isWholeNonNegative(const QJsonValue& value, double maxValue)
{
	if (value.isDouble() == false)
	{
		return false;
	}
	const auto number = value.toDouble();
	return number >= 0 && number <= maxValue && number == std::floor(number);
}

}

std::optional<LicenseClaims> LicenseToken::verify(const QString& token,
												  const QMap<QString, CryptoCore::PublicKey>& keysByKid)
{
	using namespace LicenseTokenDetail;

	const auto parts = token.toLatin1().split('.');
	if (parts.size() != 3)
	{
		return std::nullopt;
	}

	const auto headerJson = decodeSegment(parts[0]);
	const auto payloadJson = decodeSegment(parts[1]);
	const auto signature = decodeSegment(parts[2]);
	if (!headerJson || !payloadJson || !signature)
	{
		return std::nullopt;
	}

	const auto header = parseObject(*headerJson);
	if (!header || header->value(QStringLiteral("alg")).toString() != QStringLiteral("RS512"))
	{
		return std::nullopt;
	}

	const auto kid = header->value(QStringLiteral("kid")).toString();
	if (kid.isEmpty() || keysByKid.contains(kid) == false)
	{
		return std::nullopt;
	}

	auto key = keysByKid.value(kid); // verifyMessage() is not const
	if (key.isNull())
	{
		return std::nullopt;
	}

	// The algorithm is fixed here, never taken from the header.
	const QByteArray signingInput = parts[0] + '.' + parts[1];
	if (key.verifyMessage(signingInput, *signature, CryptoCore::DefaultSignatureAlgorithm) == false)
	{
		return std::nullopt;
	}

	const auto payload = parseObject(*payloadJson);
	if (!payload)
	{
		return std::nullopt;
	}
	const auto& p = *payload;

	LicenseClaims claims;
	claims.issuer = p.value(QStringLiteral("iss")).toString();
	claims.masterId = p.value(QStringLiteral("sub")).toString();
	claims.tenantId = p.value(QStringLiteral("tenant")).toString();
	claims.plan = p.value(QStringLiteral("plan")).toString();

	if (claims.issuer != QStringLiteral("license.khwarizmi.co.id") ||
		claims.masterId.isEmpty() || claims.tenantId.isEmpty() ||
		(claims.plan != QStringLiteral("trial") && claims.plan != QStringLiteral("paid")) ||
		p.value(QStringLiteral("tenant_name")).isString() == false ||
		isWholeNonNegative(p.value(QStringLiteral("max_devices")), double(std::numeric_limits<int>::max())) == false ||
		p.contains(QStringLiteral("overage_since")) == false)
	{
		return std::nullopt;
	}

	claims.tenantName = p.value(QStringLiteral("tenant_name")).toString();
	claims.maxDevices = int(p.value(QStringLiteral("max_devices")).toDouble());

	claims.subscriptionEnd = parseIsoUtc(p.value(QStringLiteral("sub_end")));
	claims.issuedAt = parseUnixSeconds(p.value(QStringLiteral("iat")));
	claims.expiresAt = parseUnixSeconds(p.value(QStringLiteral("exp")));
	if (!claims.subscriptionEnd.isValid() || !claims.issuedAt.isValid() || !claims.expiresAt.isValid() ||
		claims.expiresAt <= claims.issuedAt)
	{
		return std::nullopt;
	}

	const auto overage = p.value(QStringLiteral("overage_since"));
	if (overage.isNull() == false)
	{
		claims.overageSince = parseIsoUtc(overage);
		if (!claims.overageSince.isValid())
		{
			return std::nullopt;
		}
	}

	return claims;
}

QMap<QString, CryptoCore::PublicKey> LicenseToken::productionKeys()
{
	return { { QStringLiteral("k1"), CryptoCore::PublicKey::fromPEMFile(QStringLiteral(":/core/license-public.pem")) } };
}
