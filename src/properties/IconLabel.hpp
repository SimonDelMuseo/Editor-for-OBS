/******************************************************************************
    Copyright (C) 2024 by Sebastian Beckmann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <QIcon>
#include <QLabel>

class IconLabel : public QLabel {
	Q_OBJECT
	Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
	Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize)

public:
	inline IconLabel(QWidget *parent = nullptr) : QLabel(parent), m_icon(), m_iconSize(16) {}

	inline QIcon icon() const { return m_icon; }
	void setIcon(const QIcon &icon);

	inline int iconSize() const { return m_iconSize; }
	void setIconSize(int newSize);

private:
	QIcon m_icon;
	int m_iconSize;
};
