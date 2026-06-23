/*
 * ChatClientWindow.cpp - implementation of ChatClientWindow class
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

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QTextEdit>
#include <QTime>
#include <QVBoxLayout>

#include "ChatClientWindow.h"


ChatClientWindow::ChatClientWindow( QWidget* parent ) :
	QWidget( parent, Qt::Window )
{
	setWindowTitle( tr( "Chat with teacher" ) );
	resize( 420, 360 );

	m_conversation = new QTextEdit( this );
	m_conversation->setReadOnly( true );

	m_input = new QLineEdit( this );
	m_input->setPlaceholderText( tr( "Type a message and press Enter" ) );

	m_sendButton = new QPushButton( tr( "Send" ), this );

	auto inputRow = new QHBoxLayout;
	inputRow->addWidget( m_input );
	inputRow->addWidget( m_sendButton );

	auto mainLayout = new QVBoxLayout( this );
	mainLayout->addWidget( m_conversation );
	mainLayout->addLayout( inputRow );

	connect( m_sendButton, &QPushButton::clicked, this, &ChatClientWindow::onSend );
	connect( m_input, &QLineEdit::returnPressed, this, &ChatClientWindow::onSend );
}



void ChatClientWindow::appendFromTeacher( const QString& text )
{
	appendLine( tr( "Teacher" ), text );

	show();
	raise();
	activateWindow();
}



void ChatClientWindow::closeEvent( QCloseEvent* event )
{
	// the chat must remain available in the background, so hide instead of close
	hide();
	event->ignore();
}



void ChatClientWindow::onSend()
{
	const auto text = m_input->text().trimmed();
	if( text.isEmpty() )
	{
		return;
	}

	appendLine( tr( "You" ), text );
	m_input->clear();

	Q_EMIT messageSubmitted( text );
}



void ChatClientWindow::appendLine( const QString& sender, const QString& text )
{
	const auto line = QStringLiteral( "<b>[%1] %2:</b> %3<br>" )
						  .arg( QTime::currentTime().toString( QStringLiteral( "HH:mm" ) ),
								sender.toHtmlEscaped(), text.toHtmlEscaped() );

	m_conversation->moveCursor( QTextCursor::End );
	m_conversation->insertHtml( line );
	m_conversation->moveCursor( QTextCursor::End );
}
