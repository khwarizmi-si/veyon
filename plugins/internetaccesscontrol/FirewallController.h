/*
 * FirewallController.h - OS-specific internet block/unblock helper
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

// Blocks/unblocks general internet access while keeping loopback, the local
// network and established connections (incl. the Veyon link) reachable.
// Implemented with iptables on Linux and the Windows Firewall (netsh) on Windows.
// Must run with elevated privileges (Veyon Service / SYSTEM / root).
namespace FirewallController
{

bool blockInternet();
bool unblockInternet();

}
