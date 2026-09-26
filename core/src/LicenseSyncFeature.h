/*
 * LicenseSyncFeature.h - licence check-in through the local server
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 */

#pragma once

#include "FeatureProviderInterface.h"
#include "LicenseClient.h"
#include "LicenseService.h"

class VEYON_CORE_EXPORT LicenseSyncFeature : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	enum class Argument { Devices, Token, Result };
	Q_ENUM(Argument)
	enum class Command { CheckIn, TokenReply };
	Q_ENUM(Command)

	explicit LicenseSyncFeature(QObject* parent = nullptr);

	Plugin::Uid uid() const override { return Plugin::Uid{QStringLiteral("712251c1-87bc-4079-84de-3844a0c4b391")}; }
	QVersionNumber version() const override { return QVersionNumber(1, 0); }
	QString name() const override { return QStringLiteral("LicenseSync"); }
	QString description() const override { return tr("Licence check-in"); }
	QString vendor() const override { return QStringLiteral("Khwarizmi"); }
	QString copyright() const override { return QStringLiteral("Khwarizmi"); }
	const FeatureList& featureList() const override { return m_features; }
	const Feature& feature() const { return m_feature; }

	bool controlFeature(Feature::Uid, Operation, const QVariantMap&, const ComputerControlInterfaceList&) override
	{
		return false;
	}
	bool handleFeatureMessage(VeyonServerInterface& server, const MessageContext& context,
							  const FeatureMessage& message) override;
	bool handleFeatureMessage(ComputerControlInterface::Pointer controlInterface,
							  const FeatureMessage& message) override;

	void requestCheckIn(ComputerControlInterface::Pointer localServer, const QList<LicenseDevice>& devices);
	static QVariantList encodeDevices(const QList<LicenseDevice>& devices);
	static QList<LicenseDevice> decodeDevices(const QVariant& value);
	static QList<LicenseDevice> deduplicate(const QList<LicenseDevice>& devices);
	static QList<LicenseDevice> devicesFrom(const ComputerControlInterfaceList& interfaces);

Q_SIGNALS:
	void tokenReceived(LicenseActionResult result);

private:
	Feature m_feature;
	FeatureList m_features;
};
