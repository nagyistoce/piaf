#ifndef QLISTBOXMARKER_H
#define QLISTBOXMARKER_H

#include <q3listbox.h>

class QListBoxMarker : public Q3ListBoxText
{
public:
  QListBoxMarker( Q3ListBox* parent, const QString & text, long index = -1,
                  int mode = 0, int style = 0, int symbol = 0 )
   : Q3ListBoxText( parent, text )
  {
    mrk_key = index;
    mrk_mode = mode;
    mrk_style = style;
    symb_style = symbol;
  }
 
	~QListBoxMarker() {}

	long 	key() { return mrk_key; }
	void 	setKey( const long index )      { mrk_key = index; }

  int mode() { return mrk_mode; }
  void setMode( int mode ) { mrk_mode = mode; }
    
  int markerStyle() { return mrk_style; }
  void setMarkerStyle( int style ) { mrk_style = style; }

  int symbolStyle() { return symb_style; }
  void setSymbolStyle( int symbol ) { symb_style = symbol; }
  
  void  setText (const QString & text) { Q3ListBoxItem::setText(text); }
  
protected:
	long mrk_key;
  int mrk_mode;
  int mrk_style;
  int symb_style;
};

#endif //QLISTBOXMARKER_H

