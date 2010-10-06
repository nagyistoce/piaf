/****************************************************************************
** Form implementation generated from reading ui file 'ui/pluginlistdialog.ui'
**
** Created: Wed Feb 24 17:22:31 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "pluginlistdialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qheader.h>
#include <qlistview.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

#include "pluginlistdialog.ui.h"
/*
 *  Constructs a PluginListDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PluginListDialog::PluginListDialog( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "PluginListDialog" );
    setSizeGripEnabled( FALSE );

    delPushButton = new QPushButton( this, "delPushButton" );
    delPushButton->setGeometry( QRect( 410, 50, 91, 31 ) );

    iconPushButton = new QPushButton( this, "iconPushButton" );
    iconPushButton->setGeometry( QRect( 410, 90, 91, 31 ) );

    QWidget* privateLayoutWidget = new QWidget( this, "Layout1" );
    privateLayoutWidget->setGeometry( QRect( 20, 240, 480, 33 ) );
    Layout1 = new QHBoxLayout( privateLayoutWidget, 0, 6, "Layout1"); 
    Horizontal_Spacing2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( Horizontal_Spacing2 );

    buttonOk = new QPushButton( privateLayoutWidget, "buttonOk" );
    buttonOk->setAutoDefault( TRUE );
    buttonOk->setDefault( TRUE );
    Layout1->addWidget( buttonOk );

    buttonCancel = new QPushButton( privateLayoutWidget, "buttonCancel" );
    buttonCancel->setAutoDefault( TRUE );
    Layout1->addWidget( buttonCancel );

    addPushButton = new QPushButton( this, "addPushButton" );
    addPushButton->setGeometry( QRect( 410, 10, 91, 31 ) );

    pluginListView = new QListView( this, "pluginListView" );
    pluginListView->addColumn( tr( "Plugin binary" ) );
    pluginListView->addColumn( tr( "Icon" ) );
    pluginListView->setGeometry( QRect( 10, 10, 390, 211 ) );
    pluginListView->setAllColumnsShowFocus( TRUE );
    pluginListView->setRootIsDecorated( TRUE );
    languageChange();
    resize( QSize(504, 278).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( pluginListView, SIGNAL( currentChanged(QListViewItem*) ), this, SLOT( slotSelectPlugin(QListViewItem*) ) );
    connect( addPushButton, SIGNAL( clicked() ), this, SLOT( slotAddPlugin() ) );
    connect( delPushButton, SIGNAL( clicked() ), this, SLOT( slotDelPlugin() ) );
    connect( iconPushButton, SIGNAL( clicked() ), this, SLOT( slotIconPlugin() ) );
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PluginListDialog::~PluginListDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void PluginListDialog::languageChange()
{
    setCaption( tr( "Plugin List Dialog" ) );
    delPushButton->setText( tr( "Delete" ) );
    iconPushButton->setText( tr( "Icon" ) );
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAccel( QKeySequence( QString::null ) );
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAccel( QKeySequence( QString::null ) );
    addPushButton->setText( tr( "Add" ) );
    pluginListView->header()->setLabel( 0, tr( "Plugin binary" ) );
    pluginListView->header()->setLabel( 1, tr( "Icon" ) );
}

