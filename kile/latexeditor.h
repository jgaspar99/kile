/***************************************************************************
                          latexeditor.h  -  description
                             -------------------
    begin                : Sat Dec 29 2001
    copyright            : (C) 2001 by Pascal Brachet
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LATEXEDITOR_H
#define LATEXEDITOR_H

#include <qwidget.h>
#include <qtextedit.h>
#include <qstring.h>
#include <qcolor.h>
#include "parenmatcher.h"


/**
  *@author Pascal Brachet
  */
typedef  QColor ListColors[8];
class LatexEditor : public QTextEdit  {
   Q_OBJECT
public:
  enum Selection
  {
    Error = 3,
    Step = 4
  };

	LatexEditor(QWidget *parent, const char *name, QFont & efont,bool parmatch, ListColors col);
	~LatexEditor();
   void gotoLine( int line );
   bool search( const QString &expr, bool cs, bool wo, bool forward, bool startAtCursor );
   void replace( const QString &r);

  QTextDocument *document() const { return QTextEdit::document(); }
  void placeCursor( const QPoint &p, QTextCursor *c ) { QTextEdit::placeCursor( p, c ); }
  void setDocument( QTextDocument *doc ) { QTextEdit::setDocument( doc ); }
  QTextCursor *textCursor() const { return QTextEdit::textCursor(); }
  void repaintChanged() { QTextEdit::repaintChanged(); }

  virtual void configChanged();

  void setErrorSelection( int line );
  void setStepSelection( int line );
  void clearStepSelection();
  void clearSelections();

  virtual bool supportsErrors() const { return TRUE; }
  virtual bool supportsBreakPoints() const { return TRUE; }
  virtual void makeFunctionVisible( QTextParag * ) {}

  void drawCursor( bool b ) { QTextEdit::drawCursor( b ); }
  void commentSelection();
  void uncommentSelection();
  void indentSelection();
  void changeSettings(QFont & new_font,bool new_parmatch,ListColors new_col);
  QString getEncoding();
  void setEncoding(QString enc);
  
signals:
    void clearErrorMarker();
    void intervalChanged();

private slots:
    void cursorPosChanged( QTextCursor *c );
    void doChangeInterval();

protected:
  ParenMatcher *parenMatcher;
  bool hasError;

private:
  QString encoding;    

};

#endif
