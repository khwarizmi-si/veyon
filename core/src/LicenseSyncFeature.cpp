/*
 * LicenseSyncFeature.cpp - licence check-in through the local server
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#include <QFutureWatcher>
#include <QPointer>
#include <QSet>
#include <QTcpSocket>
#include <QtConcurrent>

#include "LicenseSelector.h"
#include "LicenseStorage.h"
#include "LicenseSyncFeature.h"
#include "VeyonServerInterface.h"

namespace LicenseSyncFeatureDetail
{
constexpr int MaxDevicesPerCheckIn = 2000;
}

LicenseSyncFeature::LicenseSyncFeature(QObject* parent) :
	QObject(parent),
	m_feature(QStringLiteral("LicenseSync"), Feature::Flag::Service | Feature::Flag::Builtin,
			  Feature::Uid(QStringLiteral("79b77526-a825-4ead-9c33-4a09d33859e9")),
			  Feature::Uid(), {}, {}, {}),
	m_features({m_feature})
{
}

QVariantList LicenseSyncFeature::encodeDevices(const QList<LicenseDevice>& devices)
{
	QVariantList result;
	result.reserve(devices.size() < LicenseSyncFeatureDetail::MaxDevicesPerCheckIn
				   ? devices.size() : LicenseSyncFeatureDetail::MaxDevicesPerCheckIn);
	for (const auto& device : devices)
	{
		if (result.size() >= LicenseSyncFeatureDetail::MaxDevicesPerCheckIn)
		{
			break;
		}
		result.append(QVariantMap{{QStringLiteral("mac"), device.mac},
							  {QStringLiteral("hostname"), device.hostname}});
	}
	return result;
}

QList<LicenseDevice> LicenseSyncFeature::decodeDevices(const QVariant& value)
{
	QList<LicenseDevice> result;
	if (value.canConvert<QVariantList>() == false)
	{
		return result;
	}
	for (const auto& entry : value.toList())
	{
		if (result.size() >= LicenseSyncFeatureDetail::MaxDevicesPerCheckIn)
		{
			break;
		}
		if (entry.canConvert<QVariantMap>() == false)
		{
			continue;
		}
		const auto map = entry.toMap();
		const auto mac = map.value(QStringLiteral("mac")).toString().trimmed();
		const auto hostname = map.value(QStringLiteral("hostname")).toString().trimmed();
		if (!mac.isEmpty() || !hostname.isEmpty())
		{
			result.append({mac, hostname});
		}
	}
	return result;
}

QList<LicenseDevice> LicenseSyncFeature::deduplicate(const QList<LicenseDevice>& devices)
{
	QList<LicenseDevice> result;
	QSet<QString> seen;
	for (const auto& device : devices)
	{
		const auto mac = device.mac.trimmed().toLower();
		const auto hostname = device.hostname.trimmed().toLower();
		const auto key = mac.isEmpty() ? hostname : mac;
		if (key.isEmpty() || seen.contains(key))
		{
			continue;
		}
		seen.insert(key);
		result.append({mac, device.hostname});
	}
	return result;
}

QList<LicenseDevice> LicenseSyncFeature::devicesFrom(const ComputerControlInterfaceList& interfaces)
{
	QList<LicenseDevice> devices;
	for (const auto& controlInterface : interfaces)
	{
		if (!controlInterface.isNull())
		{
			devices.append({controlInterface->computer().macAddress(), controlInterface->computer().hostName()});
		}
	}
	return deduplicate(devices);
}

void LicenseSyncFeature::requestCheckIn(ComputerControlInterface::Pointer localServer,
									   const QList<LicenseDevice>& devices)
{
	if (localServer.isNull())
	{
		Q_EMIT tokenReceived(LicenseActionResult::NetworkError);
		return;
	}
	sendFeatureMessage(FeatureMessage{m_feature.uid(), Command::CheckIn}
						   .addArgument(Argument::Devices, encodeDevices(devices)), {localServer});
}

bool LicenseSyncFeature::handleFeatureMessage(VeyonServerInterface& server, const MessageContext& context,
											  const FeatureMessage& message)
{
	if (message.featureUid() != m_feature.uid() || message.command<Command>() != Command::CheckIn)
	{
		return false;
	}
	const auto* socket = qobject_cast<QTcpSocket*>(context.ioDevice());
	if (!socket || !socket->peerAddress().isLoopback())
	{
		return false;
	}
	const auto devices = decodeDevices(message.argument(Argument::Devices));
	auto future = QtConcurrent::run([devices]() {
		const auto result = LicenseService::checkIn(devices);
		const LicensePublic publicStore;
		const LicenseCache cache;
		const auto selected = LicenseSelector::select(publicStore.signedToken(), cache.refreshedToken(),
												   publicStore.masterId(), LicenseToken::productionKeys());
		return qMakePair(result, result == LicenseActionResult::Ok && selected ? selected->token : QString{});
	});
	auto* watcher = new QFutureWatcher<QPair<LicenseActionResult, QString>>(this);
	QPointer<VeyonServerInterface> target(&server);
	connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, target, context]() {
		const auto reply = watcher->result();
		if (target)
		{
			target->sendFeatureMessageReply(context,
				FeatureMessage{m_feature.uid(), Command::TokenReply}
					.addArgument(Argument::Token, reply.second)
					.addArgument(Argument::Result, int(reply.first)));
		}
		watcher->deleteLater();
	});
	watcher->setFuture(future);
	return true;
}

bool LicenseSyncFeature::handleFeatureMessage(ComputerControlInterface::Pointer controlInterface,
											  const FeatureMessage& message)
{
	Q_UNUSED(controlInterface)
	if (message.featureUid() != m_feature.uid() || message.command<Command>() != Command::TokenReply)
	{
		return false;
	}
	const auto token = message.argument(Argument::Token).toString();
	if (token.isEmpty())
	{
		Q_EMIT tokenReceived(LicenseActionResult(message.argument(Argument::Result).toInt()));
		return true;
	}
	const LicensePublic publicStore;
	LicenseCache cache;
	const auto result = LicenseService::acceptToken(token, cache.refreshedToken(), publicStore.masterId(),
												QDateTime::currentDateTimeUtc(), LicenseToken::productionKeys());
	if (result == LicenseActionResult::Ok)
	{
		const auto claims = LicenseToken::verify(token, LicenseToken::productionKeys());
		if (!claims)
		{
			Q_EMIT tokenReceived(LicenseActionResult::TokenRejected);
			return true;
		}
		cache.setRefreshedToken(token);
		const auto storedTime = QDateTime::fromString(cache.lastServerTime(), Qt::ISODateWithMs);
		if (!storedTime.isValid() || claims->issuedAt > storedTime)
		{
			cache.setLastServerTime(claims->issuedAt.toString(Qt::ISODateWithMs));
		}
		cache.flushStore();
		const LicenseCache reloaded;
		if (reloaded.refreshedToken() != token || reloaded.lastServerTime() != cache.lastServerTime())
		{
			Q_EMIT tokenReceived(LicenseActionResult::NotWritable);
			return true;
		}
	}
	Q_EMIT tokenReceived(result);
	return true;
}
