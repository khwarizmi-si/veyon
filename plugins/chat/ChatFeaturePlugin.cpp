/*
 * ChatFeaturePlugin.cpp - implementation of ChatFeaturePlugin class
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

#include "ChatFeaturePlugin.h"
#include "ChatMasterDialog.h"
#include "ChatClientWindow.h"
#include <QIODevice>

#include "FeatureWorkerManager.h"
#include "ComputerControlInterface.h"
#include "MessageContext.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VeyonWorkerInterface.h"

namespace
{
// per-master-connection property tracking how many queued replies were already delivered
const char* chatDeliveredCountProperty = "chatDeliveredCount";
}


ChatFeaturePlugin::ChatFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_chatFeature( Feature( QStringLiteral( "Chat" ),
							Feature::Flag::Action | Feature::Flag::AllComponents,
							Feature::Uid( "1d6c0d4a-3e8b-4f2a-9a7c-2b5e9f0a4c31" ),
							Feature::Uid(),
							tr( "Chat" ), {},
							tr( "Use this function to open a two-way chat with the "
								"selected students." ),
							QStringLiteral(":/chat/chat.png") ) ),
	m_features( { m_chatFeature } )
{
}



const FeatureList& ChatFeaturePlugin::featureList() const
{
	return m_features;
}



bool ChatFeaturePlugin::controlFeature( Feature::Uid featureUid,
										Operation operation,
										const QVariantMap& arguments,
										const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( operation != Operation::Start || featureUid != m_chatFeature.uid() )
	{
		return false;
	}

	const auto text = arguments.value( argToString( Argument::Text ) ).toString();
	if( text.isEmpty() )
	{
		return false;
	}

	for( const auto& computerControlInterface : computerControlInterfaces )
	{
		sendToComputer( computerControlInterface, text );
	}

	return true;
}



bool ChatFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
									  const ComputerControlInterfaceList& computerControlInterfaces )
{
	if( feature.uid() != m_chatFeature.uid() )
	{
		return false;
	}

	if( m_masterDialog.isNull() )
	{
		m_masterDialog = new ChatMasterDialog( master.mainWindow() );
		connect( m_masterDialog, &ChatMasterDialog::messageSubmitted,
				 this, &ChatFeaturePlugin::sendToComputer );
	}

	m_masterDialog->addComputers( computerControlInterfaces );
	m_masterDialog->show();
	m_masterDialog->raise();
	m_masterDialog->activateWindow();

	return true;
}



bool ChatFeaturePlugin::handleFeatureMessage( ComputerControlInterface::Pointer computerControlInterface,
											  const FeatureMessage& message )
{
	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	// incoming reply from a student session
	if( m_masterDialog.isNull() == false )
	{
		m_masterDialog->appendIncoming( computerControlInterface,
										message.argument( Argument::Text ).toString() );
	}

	return true;
}



bool ChatFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
											  const MessageContext& messageContext,
											  const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	// forward the message to the worker running in the user session
	server.featureWorkerManager().sendMessageToUnmanagedSessionWorker( message );

	return true;
}



bool ChatFeaturePlugin::handleFeatureMessageFromWorker( VeyonServerInterface& server, const FeatureMessage& message )
{
	Q_UNUSED(server)

	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	// queue the student reply; it is pushed to the connected master(s) in sendAsyncFeatureMessages()
	m_pendingReplies.append( message.argument( Argument::Text ).toString() );

	return true;
}



void ChatFeaturePlugin::sendAsyncFeatureMessages( VeyonServerInterface& server, const MessageContext& messageContext )
{
	auto ioDevice = messageContext.ioDevice();
	if( ioDevice == nullptr )
	{
		return;
	}

	// deliver only replies this particular master connection has not seen yet
	const int delivered = ioDevice->property( chatDeliveredCountProperty ).toInt();
	for( int i = delivered; i < m_pendingReplies.size(); ++i )
	{
		server.sendFeatureMessageReply( messageContext,
										FeatureMessage{ m_chatFeature.uid(), FeatureCommand::ChatMessage }
											.addArgument( Argument::Text, m_pendingReplies.at( i ) ) );
	}

	ioDevice->setProperty( chatDeliveredCountProperty, m_pendingReplies.size() );
}



bool ChatFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	if( message.featureUid() != m_chatFeature.uid() )
	{
		return false;
	}

	m_worker = &worker;

	if( m_clientWindow.isNull() )
	{
		m_clientWindow = new ChatClientWindow;
		connect( m_clientWindow, &ChatClientWindow::messageSubmitted, this,
				 [this]( const QString& text ) {
					 if( m_worker )
					 {
						 sendReplyToMaster( *m_worker, text );
					 }
				 } );
	}

	m_clientWindow->appendFromTeacher( message.argument( Argument::Text ).toString() );

	return true;
}



void ChatFeaturePlugin::sendToComputer( const ComputerControlInterface::Pointer& computerControlInterface, const QString& text )
{
	sendFeatureMessage( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::ChatMessage }
							.addArgument( Argument::Text, text ),
						{ computerControlInterface } );
}



void ChatFeaturePlugin::sendReplyToMaster( VeyonWorkerInterface& worker, const QString& text )
{
	worker.sendFeatureMessageReply( FeatureMessage{ m_chatFeature.uid(), FeatureCommand::ChatMessage }
										.addArgument( Argument::Text, text ) );
}
