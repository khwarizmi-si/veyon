/*
 * ChatClientWindow.h - student-side persistent chat window
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

#include <QWidget>

class QTextEdit;
class QLineEdit;
class QPushButton;

class ChatClientWindow : public QWidget
{
	Q_OBJECT
public:
	explicit ChatClientWindow( QWidget* parent = nullptr );
	~ChatClientWindow() override = default;

	// show an incoming teacher message and raise the window
	void appendFromTeacher( const QString& text );

Q_SIGNALS:
	void messageSubmitted( const QString& text );

protected:
	// keep the window alive in the background instead of destroying it
	void closeEvent( QCloseEvent* event ) override;

private:
	void onSend();
	void appendLine( const QString& sender, const QString& text );

	QTextEdit* m_conversation = nullptr;
	QLineEdit* m_input = nullptr;
	QPushButton* m_sendButton = nullptr;

};
