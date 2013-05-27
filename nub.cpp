/* Copyright 2013 Mohammed Nafees. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MOHAMMED NAFEES ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL EELI REILIN OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Mohammed Nafees.
 */

// Self
#include "nub.h"
#include "ui_nub.h"

// Qt
#include <QFileDialog>
#include <QNetworkReply>
#include <QMessageBox>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDebug>

// nub
#include "ChunkUploader.h"

// o2
#include "o2/o1dropbox.h"

// Your Dropbox app's details
const char* APP_KEY = "1pjliqi839c24fy";
const char* APP_SECRET = "nrzyrdfx4qic5fg";
const AppType APP_TYPE = FullDropbox;

nub::nub( QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::MainWindow )
{
    ui->setupUi( this );
    // Place the widget in an appropriate place on the desktop
    setGeometry( qApp->desktop()->availableGeometry().width() / 2 - 250,
                 qApp->desktop()->availableGeometry().y() + 100, 500, 158 );

    setWindowTitle( "nub - Dropbox Chunk Uploader" );
    ui->progressBar->hide();
    ui->successLabel->hide();

    dropbox = new O1Dropbox( this );
    dropbox->setClientId( APP_KEY );
    dropbox->setClientSecret( APP_SECRET );
    if ( dropbox->linked() ) {
        ui->deauthorizeButton->setEnabled( true );
        ui->authorizeButton->setEnabled( false );
    } else {
        ui->uploadButton->hide();
        ui->deauthorizeButton->setEnabled( false );
        ui->authorizeButton->setEnabled( true );
    }

    QFileDialog *fileDialog = new QFileDialog( this, Qt::Sheet );
    connect( fileDialog, SIGNAL(fileSelected(QString)), SLOT(uploadFile(QString)) );

    manager = new QNetworkAccessManager( this );

    // O1Dropbox
    connect( dropbox, SIGNAL(linkingFailed()), SLOT(onLinkingFailed()) );
    connect( dropbox, SIGNAL(linkingSucceeded()), SLOT(onLinkingSucceeded()) );
    connect( dropbox, SIGNAL(openBrowser(QUrl)), SLOT(onOpenBrowser(QUrl)) );
    connect( dropbox, SIGNAL(closeBrowser()), SLOT(onCloseBrowser()) );

    // Ui::MainWindow
    connect( ui->deauthorizeButton, SIGNAL(clicked()), SLOT(deauthorize()) );
    connect( ui->authorizeButton, SIGNAL(clicked()), SLOT(authorize()) );
    connect( ui->uploadButton, SIGNAL(clicked()), fileDialog, SLOT(exec()) );
}

nub::~nub()
{
    delete ui;
}

void nub::onLinkingFailed()
{
    QMessageBox::critical( this, QString(), "There seems to be a problem with the authorization process."
                           " Please try again." );
    deauthorize();
}

void nub::onLinkingSucceeded()
{
    ui->uploadButton->show();
}

void nub::onOpenBrowser( QUrl url )
{
    QDesktopServices::openUrl( url );
}

void nub::onCloseBrowser()
{
    /** @todo Find how to handle this */
}

void nub::deauthorize()
{
    ui->deauthorizeButton->setEnabled( false );
    ui->authorizeButton->setEnabled( true );
    dropbox->unlink();
    ui->uploadButton->hide();
    ui->successLabel->hide();
}

void nub::authorize()
{
    ui->deauthorizeButton->setEnabled( true );
    ui->authorizeButton->setEnabled( false );
    dropbox->link();
}

void nub::uploadFile( QString file )
{
    ui->deauthorizeButton->setEnabled( false );
    ui->uploadButton->hide();
    ui->progressBar->show();
    if ( ui->successLabel->isVisible() )
        ui->successLabel->hide();

    ChunkUploader *uploader = new ChunkUploader( this );
    uploader->setAppType( APP_TYPE );
    uploader->setClients( dropbox, manager );
    uploader->initialise( file );
    connect( uploader, SIGNAL(uploadProgressChanged(int)), ui->progressBar, SLOT(setValue(int)) );
    connect( uploader, SIGNAL(errorOccurred()), ui->progressBar, SLOT(hide()) );
    connect( uploader, SIGNAL(errorOccurred()), ui->progressBar, SLOT(reset()) );
    connect( uploader, SIGNAL(errorOccurred()), ui->uploadButton, SLOT(show()) );
    connect( uploader, SIGNAL(uploadSuccessful()), ui->progressBar, SLOT(hide()) );
    connect( uploader, SIGNAL(uploadSuccessful()), ui->progressBar, SLOT(reset()) );
    connect( uploader, SIGNAL(uploadSuccessful()), ui->successLabel, SLOT(show()) );
    connect( uploader, SIGNAL(uploadSuccessful()), ui->uploadButton, SLOT(show()) );
    //connect( uploader, SIGNAL(uploadSuccessful()), ui->deauthorizeButton, SLOT(setDisabled(bool)) );
}
