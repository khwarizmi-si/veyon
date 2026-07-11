/*
 * FirewallController.cpp - OS-specific internet block/unblock helper
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

#include <QDebug>
#include <QProcess>
#include <QStringList>

#include "FirewallController.h"

namespace
{

// the private ranges and loopback that must stay reachable while internet is blocked
const auto allowedDestinations = QStringList{
	QStringLiteral("127.0.0.0/8"),    // loopback
	QStringLiteral("10.0.0.0/8"),     // private LAN
	QStringLiteral("172.16.0.0/12"),  // private LAN
	QStringLiteral("192.168.0.0/16"), // private LAN
	QStringLiteral("169.254.0.0/16"), // link-local
};

const auto kChainName = QStringLiteral("VEYON_IAC");
const auto kRulePrefix = QStringLiteral("VeyonIAC-Allow-");

// run one shell command line, return true on success (exit code 0)
bool runShell( const QString& commandLine )
{
#if defined(Q_OS_WIN)
	const auto program = QStringLiteral("cmd");
	const auto args = QStringList{ QStringLiteral("/c"), commandLine };
#else
	const auto program = QStringLiteral("sh");
	const auto args = QStringList{ QStringLiteral("-c"), commandLine };
#endif
	const int exitCode = QProcess::execute( program, args );
	if( exitCode != 0 )
	{
		qWarning() << "FirewallController: command failed (" << exitCode << "):" << commandLine;
		return false;
	}
	return true;
}


bool runAll( const QStringList& commandLines )
{
	bool ok = true;
	for( const auto& commandLine : commandLines )
	{
		ok = runShell( commandLine ) && ok;
	}
	return ok;
}


#if defined(Q_OS_WIN)

QStringList buildBlockCommands()
{
	// switch the default outbound policy to block, then punch holes for the
	// loopback and local networks; return traffic of the established Veyon
	// connection is permitted by the firewall's stateful filtering
	QStringList commands{
		QStringLiteral("netsh advfirewall set allprofiles firewallpolicy blockinbound,blockoutbound"),
	};
	int index = 0;
	for( const auto& destination : allowedDestinations )
	{
		commands += QStringLiteral("netsh advfirewall firewall add rule name=\"%1%2\" "
								   "dir=out action=allow remoteip=%3")
						.arg( kRulePrefix ).arg( index++ ).arg( destination );
	}
	return commands;
}


QStringList buildUnblockCommands()
{
	QStringList commands{
		QStringLiteral("netsh advfirewall set allprofiles firewallpolicy blockinbound,allowoutbound"),
	};
	for( int index = 0; index < allowedDestinations.size(); ++index )
	{
		commands += QStringLiteral("netsh advfirewall firewall delete rule name=\"%1%2\"")
						.arg( kRulePrefix ).arg( index );
	}
	return commands;
}

#else

QStringList buildBlockCommands()
{
	// (re)create a dedicated chain so unblocking can remove exactly our rules
	QStringList commands{
		QStringLiteral("iptables -N %1 2>/dev/null; iptables -F %1").arg( kChainName ),
		QStringLiteral("iptables -A %1 -o lo -j ACCEPT").arg( kChainName ),
		QStringLiteral("iptables -A %1 -m state --state ESTABLISHED,RELATED -j ACCEPT").arg( kChainName ),
	};
	for( const auto& destination : allowedDestinations )
	{
		commands += QStringLiteral("iptables -A %1 -d %2 -j ACCEPT").arg( kChainName, destination );
	}
	commands += QStringLiteral("iptables -A %1 -j REJECT").arg( kChainName );
	commands += QStringLiteral("iptables -C OUTPUT -j %1 2>/dev/null || iptables -I OUTPUT -j %1").arg( kChainName );
	return commands;
}


QStringList buildUnblockCommands()
{
	return QStringList{
		QStringLiteral("iptables -D OUTPUT -j %1 2>/dev/null").arg( kChainName ),
		QStringLiteral("iptables -F %1 2>/dev/null").arg( kChainName ),
		QStringLiteral("iptables -X %1 2>/dev/null").arg( kChainName ),
		QStringLiteral("true"), // ensure a clean exit code even if nothing existed
	};
}

#endif

}


bool FirewallController::blockInternet()
{
	return runAll( buildBlockCommands() );
}


bool FirewallController::unblockInternet()
{
	return runAll( buildUnblockCommands() );
}
