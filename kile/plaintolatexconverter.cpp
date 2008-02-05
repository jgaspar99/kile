/*
 * Copyright (c) 2004 Simon MARTIN <simartin@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "plaintolatexconverter.h"

PlainToLaTeXConverter::PlainToLaTeXConverter(void)
{
	// Fill the replacement map
	//TODO Do it only once!
	m_replaceMap.insert('$', "\\$");
	m_replaceMap.insert('%', "\\%");
	m_replaceMap.insert('^', "\\^");
	m_replaceMap.insert('&', "\\&");
	m_replaceMap.insert('_', "\\_");
	m_replaceMap.insert('#', "\\#");
	m_replaceMap.insert('{', "\\{");
	m_replaceMap.insert('}', "\\}");
	m_replaceMap.insert('~', "$\\sim$");
}

PlainToLaTeXConverter::~PlainToLaTeXConverter(void) {}

/**
 * Converts plain text to LaTeX.
 * @param toConv The string to convert
 * @return The conversion's result
 */
QString PlainToLaTeXConverter::ConvertToLaTeX(const QString& toConv) const
{
	QString result(toConv);

	// Replacing what must be...
	uint sSize = result.length();
	QMapConstIterator<QChar, QString> mapEnd = m_replaceMap.end();
	for(uint i = 0 ; i < sSize ; ++i) 
	{
		QMapConstIterator<QChar, QString> it = m_replaceMap.find(result.at(i));

		if(it != mapEnd) { // The character must be replaced
			result.replace(i, 1, *it);
			uint len = (*it).length();
			if(1 < len) {
				i += len - 1;
				sSize += len - 1;
			}
		}
	}

	return result;
}