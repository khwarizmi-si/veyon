/*
 * ChatFeaturePlugin.h - declaration of ChatFeaturePlugin class
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

#include <QPointer>
#include <QStringList>

#include "Feature.h"
#include "FeatureProviderInterface.h"

class ChatMasterDialog;
class ChatClientWindow;
class MessageContext;
class VeyonWorkerInterface;

class ChatFeaturePlugin : public QObject, FeatureProviderInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.Chat")
	Q_INTERFACES(PluginInterface FeatureProviderInterface)
public:
	enum class Argument {
		Text,
	};
	Q_ENUM(Argument)

	explicit ChatFeaturePlugin( QObject* parent = nullptr );
	~ChatFeaturePlugin() override = default;

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("3f2a1c6e-7b54-4d9a-9c21-0d8e5f6a1b22") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral("Chat");
	}

	QString description() const override
	{
		return tr( "Two-way chat between teacher and students" );
	}

	QString vendor() const override
	{
		return QStringLiteral("Sahid");
	}

	QString copyright() const override
	{
		return QStringLiteral("Sahid");
	}

	const FeatureList& featureList() const override;

	bool controlFeature( Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
						const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessage( VeyonServerInterface& server,
							   const MessageContext& messageContext,
							   const FeatureMessage& message ) override;

	bool handleFeatureMessageFromWorker( VeyonServerInterface& server,
										 const FeatureMessage& message ) override;

	void sendAsyncFeatureMessages( VeyonServerInterface& server,
								   const MessageContext& messageContext ) override;

	bool handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message ) override;

private:
	enum class FeatureCommand {
		ChatMessage,
	};

	void sendToComputer( const ComputerControlInterface::Pointer& computerControlInterface, const QString& text );
	void sendReplyToMaster( VeyonWorkerInterface& worker, const QString& text );

	const Feature m_chatFeature;
	const FeatureList m_features;

	// master-side conversation window (lives in Veyon Master process)
	QPointer<ChatMasterDialog> m_masterDialog;

	// client-side chat window (lives in student session worker process)
	QPointer<ChatClientWindow> m_clientWindow;
	VeyonWorkerInterface* m_worker = nullptr;

	// server-side queue of student replies awaiting delivery to the master(s)
	QStringList m_pendingReplies;

};
