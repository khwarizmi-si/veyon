/*
 * ChatMasterDialog.h - teacher-side chat window with one conversation per computer
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

#include <QDialog>
#include <QHash>

#include "ComputerControlInterface.h"

class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;

class ChatMasterDialog : public QDialog
{
	Q_OBJECT
public:
	explicit ChatMasterDialog( QWidget* parent );
	~ChatMasterDialog() override = default;

	// add computers to the conversation list (no-op for already present ones)
	void addComputers( const ComputerControlInterfaceList& computerControlInterfaces );

	// append an incoming student message to the matching conversation
	void appendIncoming( const ComputerControlInterface::Pointer& computerControlInterface, const QString& text );

Q_SIGNALS:
	void messageSubmitted( ComputerControlInterface::Pointer computerControlInterface, const QString& text );

private:
	void onComputerSelected( int row );
	void onSend();
	void appendLine( const ComputerControlInterface::Pointer& computerControlInterface,
					 const QString& sender, const QString& text );
	void renderHistory( const ComputerControlInterface::Pointer& computerControlInterface );
	ComputerControlInterface::Pointer currentComputer() const;

	QListWidget* m_computerList = nullptr;
	QTextEdit* m_conversation = nullptr;
	QLineEdit* m_input = nullptr;
	QPushButton* m_sendButton = nullptr;

	ComputerControlInterfaceList m_computers;
	QHash<ComputerControlInterface*, QString> m_history;

};
