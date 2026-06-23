/*
 * ChatMasterDialog.cpp - implementation of ChatMasterDialog class
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

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextCursor>
#include <QTextEdit>
#include <QTime>
#include <QVBoxLayout>

#include "ChatMasterDialog.h"


ChatMasterDialog::ChatMasterDialog( QWidget* parent ) :
	QDialog( parent )
{
	setWindowTitle( tr( "Chat" ) );
	resize( 640, 420 );

	m_computerList = new QListWidget( this );
	m_computerList->setMaximumWidth( 200 );

	m_conversation = new QTextEdit( this );
	m_conversation->setReadOnly( true );

	m_input = new QLineEdit( this );
	m_input->setPlaceholderText( tr( "Type a message and press Enter" ) );
	m_input->setEnabled( false );

	m_sendButton = new QPushButton( tr( "Send" ), this );
	m_sendButton->setEnabled( false );

	auto inputRow = new QHBoxLayout;
	inputRow->addWidget( m_input );
	inputRow->addWidget( m_sendButton );

	auto rightColumn = new QWidget( this );
	auto rightLayout = new QVBoxLayout( rightColumn );
	rightLayout->setContentsMargins( 0, 0, 0, 0 );
	rightLayout->addWidget( m_conversation );
	rightLayout->addLayout( inputRow );

	auto splitter = new QSplitter( this );
	splitter->addWidget( m_computerList );
	splitter->addWidget( rightColumn );
	splitter->setStretchFactor( 1, 1 );

	auto mainLayout = new QVBoxLayout( this );
	mainLayout->addWidget( splitter );

	connect( m_computerList, &QListWidget::currentRowChanged, this, &ChatMasterDialog::onComputerSelected );
	connect( m_sendButton, &QPushButton::clicked, this, &ChatMasterDialog::onSend );
	connect( m_input, &QLineEdit::returnPressed, this, &ChatMasterDialog::onSend );
}



void ChatMasterDialog::addComputers( const ComputerControlInterfaceList& computerControlInterfaces )
{
	for( const auto& computerControlInterface : computerControlInterfaces )
	{
		if( m_computers.contains( computerControlInterface ) )
		{
			continue;
		}

		m_computers.append( computerControlInterface );
		m_computerList->addItem( computerControlInterface->computerName() );
	}

	if( m_computerList->currentRow() < 0 && m_computerList->count() > 0 )
	{
		m_computerList->setCurrentRow( 0 );
	}
}



void ChatMasterDialog::appendIncoming( const ComputerControlInterface::Pointer& computerControlInterface, const QString& text )
{
	appendLine( computerControlInterface, computerControlInterface->computerName(), text );
}



void ChatMasterDialog::onComputerSelected( int row )
{
	const bool hasSelection = row >= 0 && row < m_computers.count();
	m_input->setEnabled( hasSelection );
	m_sendButton->setEnabled( hasSelection );

	if( hasSelection )
	{
		renderHistory( m_computers.at( row ) );
	}
	else
	{
		m_conversation->clear();
	}
}



void ChatMasterDialog::onSend()
{
	const auto computerControlInterface = currentComputer();
	const auto text = m_input->text().trimmed();
	if( computerControlInterface.isNull() || text.isEmpty() )
	{
		return;
	}

	appendLine( computerControlInterface, tr( "Teacher" ), text );
	m_input->clear();

	Q_EMIT messageSubmitted( computerControlInterface, text );
}



void ChatMasterDialog::appendLine( const ComputerControlInterface::Pointer& computerControlInterface,
								   const QString& sender, const QString& text )
{
	const auto line = QStringLiteral( "<b>[%1] %2:</b> %3<br>" )
						  .arg( QTime::currentTime().toString( QStringLiteral( "HH:mm" ) ),
								sender.toHtmlEscaped(), text.toHtmlEscaped() );

	m_history[computerControlInterface.data()] += line;

	if( computerControlInterface == currentComputer() )
	{
		renderHistory( computerControlInterface );
	}
}



void ChatMasterDialog::renderHistory( const ComputerControlInterface::Pointer& computerControlInterface )
{
	m_conversation->setHtml( m_history.value( computerControlInterface.data() ) );
	m_conversation->moveCursor( QTextCursor::End );
}



ComputerControlInterface::Pointer ChatMasterDialog::currentComputer() const
{
	const int row = m_computerList->currentRow();
	if( row < 0 || row >= m_computers.count() )
	{
		return {};
	}
	return m_computers.at( row );
}
