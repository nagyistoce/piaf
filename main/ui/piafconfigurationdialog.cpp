/****************************************************************************
** Form implementation generated from reading ui file 'ui/piafconfigurationdialog.ui'
**
** Created: Wed Feb 24 17:22:30 2010
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "piafconfigurationdialog.h"

#include <qvariant.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>

#include "stdlib.h"
#include "piafconfigurationdialog.ui.h"
/*
 *  Constructs a PiafConfigurationDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
PiafConfigurationDialog::PiafConfigurationDialog( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "PiafConfigurationDialog" );
    setSizeGripEnabled( FALSE );

    textLabel1 = new QLabel( this, "textLabel1" );
    textLabel1->setGeometry( QRect( 10, 1, 211, 20 ) );

    textLabel1_2 = new QLabel( this, "textLabel1_2" );
    textLabel1_2->setGeometry( QRect( 10, 70, 211, 20 ) );

    imagePushButton = new QPushButton( this, "imagePushButton" );
    imagePushButton->setGeometry( QRect( 510, 90, 50, 31 ) );

    textLabel1_3 = new QLabel( this, "textLabel1_3" );
    textLabel1_3->setGeometry( QRect( 10, 130, 211, 20 ) );

    QWidget* privateLayoutWidget = new QWidget( this, "Layout1" );
    privateLayoutWidget->setGeometry( QRect( 10, 220, 550, 33 ) );
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

    moviePushButton = new QPushButton( this, "moviePushButton" );
    moviePushButton->setGeometry( QRect( 510, 150, 50, 31 ) );

    measurePushButton = new QPushButton( this, "measurePushButton" );
    measurePushButton->setGeometry( QRect( 510, 30, 50, 31 ) );

    defaultMovieLineEdit = new QLineEdit( this, "defaultMovieLineEdit" );
    defaultMovieLineEdit->setGeometry( QRect( 10, 150, 490, 31 ) );

    defaultImageLineEdit = new QLineEdit( this, "defaultImageLineEdit" );
    defaultImageLineEdit->setGeometry( QRect( 10, 90, 490, 31 ) );

    saveAtExitCheckBox = new QCheckBox( this, "saveAtExitCheckBox" );
    saveAtExitCheckBox->setGeometry( QRect( 10, 190, 280, 30 ) );
    saveAtExitCheckBox->setChecked( TRUE );

    defaultMeasureLineEdit = new QLineEdit( this, "defaultMeasureLineEdit" );
    defaultMeasureLineEdit->setGeometry( QRect( 10, 30, 490, 31 ) );
    languageChange();
    resize( QSize(571, 264).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( defaultImageLineEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( slotImageChanged(const QString&) ) );
    connect( defaultMeasureLineEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( slotMeasureChanged(const QString&) ) );
    connect( defaultMovieLineEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( slotMovieChanged(const QString&) ) );
    connect( imagePushButton, SIGNAL( clicked() ), this, SLOT( slotImagePushed() ) );
    connect( measurePushButton, SIGNAL( clicked() ), this, SLOT( slotMeasurePushed() ) );
    connect( moviePushButton, SIGNAL( clicked() ), this, SLOT( slotMoviePushed() ) );
    connect( saveAtExitCheckBox, SIGNAL( toggled(bool) ), this, SLOT( slotSaveAtExit(bool) ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
PiafConfigurationDialog::~PiafConfigurationDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void PiafConfigurationDialog::languageChange()
{
    setCaption( tr( "Piaf Configuration Dialog" ) );
    textLabel1->setText( tr( "Default measure directory :" ) );
    textLabel1_2->setText( tr( "Default image directory :" ) );
    imagePushButton->setText( tr( "..." ) );
    textLabel1_3->setText( tr( "Default movie directory :" ) );
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAccel( QKeySequence( QString::null ) );
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAccel( QKeySequence( QString::null ) );
    moviePushButton->setText( tr( "..." ) );
    measurePushButton->setText( tr( "..." ) );
    saveAtExitCheckBox->setText( tr( "Save configuration at exit" ) );
}

