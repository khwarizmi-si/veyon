/*
 * InternetAccessControlPlugin.cpp - implementation of InternetAccessControlPlugin class
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

#include "InternetAccessControlPlugin.h"
#include "FirewallController.h"
#include "ComputerControlInterface.h"
#include "VeyonServerInterface.h"


InternetAccessControlPlugin::InternetAccessControlPlugin( QObject* parent ) :
	QObject( parent ),
	m_internetAccessControlFeature( Feature( QStringLiteral( "InternetAccessControl" ),
							Feature::Flag::Mode | Feature::Flag::Master | Feature::Flag::Service,
							Feature::Uid( "a4d1f0b2-6e57-4c39-8b1a-2f9c7d3e5a16" ),
							Feature::Uid(),
							tr( "Block internet" ),
							tr( "Allow internet" ),
							tr( "Use this function to block or allow internet access on "
								"the selected computers, e.g. during exams." ),
							QStringLiteral(":/internetaccesscontrol/internet-blocked.png") ) ),
	m_features( { m_internetAccessControlFeature } )
{
}



const FeatureList& InternetAccessControlPlugin::featureList() const
{
	return m_features;
}



bool InternetAccessControlPlugin::controlFeature( Feature::Uid featureUid,
												  Operation operation,
												  const QVariantMap& arguments,
												  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( featureUid != m_internetAccessControlFeature.uid() )
	{
		return false;
	}

	// never block the teacher's own computer: it may be listed as a room computer,
	// but cutting its internet would also cut its link to the managed computers
	auto targets = computerControlInterfaces;
	targets.removeLocalHostInterfaces();

	if( operation == Operation::Start )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::BlockInternet }, targets );
		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage( FeatureMessage{ featureUid, FeatureCommand::UnblockInternet }, targets );
		return true;
	}

	return false;
}



bool InternetAccessControlPlugin::handleFeatureMessage( VeyonServerInterface& server,
														const MessageContext& messageContext,
														const FeatureMessage& message )
{
	Q_UNUSED(server)
	Q_UNUSED(messageContext)

	if( message.featureUid() != m_internetAccessControlFeature.uid() )
	{
		return false;
	}

	// the Veyon Service runs with system privileges, so the firewall change happens here
	switch( message.command<FeatureCommand>() )
	{
	case FeatureCommand::BlockInternet:
		FirewallController::blockInternet();
		return true;
	case FeatureCommand::UnblockInternet:
		FirewallController::unblockInternet();
		return true;
	}

	return true;
}
