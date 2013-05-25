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
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY EXPRESS OR
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
#include <QFile>
#include <QNetworkReply>
#include <QMessageBox>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QDebug>

// o2
#include "o2/o1dropbox.h"
#include "o2/o1requestor.h"

// qt-json
#include "qt-json/json.h"

const int chunkSize = 4194304; // 4MB chunks

nub::nub( QWidget *parent ) :
    QWidget( parent ),
    ui( new Ui::MainWindow ),
    m_uploadId( "" ),
    m_totalBytes( 0 ),
    m_totalUploaded( 0 ),
    m_index( 0 )
{
    ui->setupUi( this );
    // Place the widget in an appropriate place on the desktop
    setGeometry( qApp->desktop()->availableGeometry().width() / 2 - 250,
                 qApp->desktop()->availableGeometry().y() + 100, 500, 158 );

    setWindowTitle( "nub - Dropbox Chunk Uploader" );
    ui->progressBar->hide();
    ui->successLabel->hide();

    dropbox = new O1Dropbox( this );
    dropbox->setClientId( "1pjliqi839c24fy" ); // APP KEY
    dropbox->setClientSecret( "nrzyrdfx4qic5fg" ); // APP SECRET
    if ( dropbox->linked() ) {
        ui->deauthorizeButton->setEnabled( true );
        ui->authorizeButton->setEnabled( false );
    } else {
        ui->uploadButton->hide();
        ui->deauthorizeButton->setEnabled( false );
        ui->authorizeButton->setEnabled( true );
    }

    QFileDialog *fileDialog = new QFileDialog( this, Qt::Sheet );
    manager = new QNetworkAccessManager( this );

    connect( dropbox, SIGNAL(linkingFailed()), SLOT(onLinkingFailed()) );
    connect( dropbox, SIGNAL(linkingSucceeded()), SLOT(onLinkingSucceeded()) );
    connect( dropbox, SIGNAL(openBrowser(QUrl)), SLOT(onOpenBrowser(QUrl)) );
    connect( dropbox, SIGNAL(closeBrowser()), SLOT(onCloseBrowser()) );
    connect( ui->deauthorizeButton, SIGNAL(clicked()), SLOT(deauthorize()) );
    connect( ui->authorizeButton, SIGNAL(clicked()), SLOT(authorize()) );
    connect( ui->uploadButton, SIGNAL(clicked()), fileDialog, SLOT(exec()) );
    connect( fileDialog, SIGNAL(fileSelected(QString)), SLOT(uploadFile(QString)) );
    connect( this, SIGNAL(uploadProgressChanged(int)), ui->progressBar, SLOT(setValue(int)) );
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
    QFile selectedFile( file );
    if (  selectedFile.bytesAvailable() < chunkSize ) {
        QMessageBox::information( this, QString(), "The selected file seems to"
                               " be quite small for a chunk upload to the server."
                               " Please select a larger file." );
        return;
    }

    if ( selectedFile.open( QIODevice::ReadOnly ) ) {
        ui->uploadButton->hide();
        ui->progressBar->show();
        fileName = QFileInfo( selectedFile ).fileName();
        data = selectedFile.readAll();
        m_totalBytes = data.length();
        selectedFile.close();
        uploadChunks();
    } else {
        QMessageBox::critical( this, QString(), "Sorry but this file type is not supported"
                               ". Please select an appropriate file to start uploading." );
    }
}

void nub::chunkUploadProgress( qint64 uploaded, qint64 )
{
    m_uploadedLast = uploaded;
    m_totalUploaded = m_index + m_uploadedLast;
    handleProgressBar();
}

void nub::uploadChunks()
{
    QUrl uploadUrl( "https://api-content.dropbox.com/1/chunked_upload" );
    QList<O1RequestParameter> param;
    param.append( O1RequestParameter( "oauth_signature_method", "HMAC-SHA1" ) );
    param.append( O1RequestParameter( "oauth_consumer_key", dropbox->clientId().toAscii() ) );
    param.append( O1RequestParameter( "oauth_version", "1.0" ) );
    param.append( O1RequestParameter( "oauth_timestamp", QString::number( QDateTime::currentDateTimeUtc().toTime_t() ).toAscii() ) );
    param.append( O1RequestParameter( "oauth_nonce", dropbox->nonce() ) );
    param.append( O1RequestParameter( "oauth_token", dropbox->token().toAscii() ) );
    if ( uploadId() != "" ) {
        // First chunk has been uploaded and we have an upload ID
        param.append( O1RequestParameter( "upload_id", uploadId().toAscii() ) );
        param.append( O1RequestParameter( "offset", QString::number( m_totalUploaded ).toAscii() ) );
    }
    QByteArray signature = O1::sign( param, QList<O1RequestParameter>(), uploadUrl,
                                     QNetworkAccessManager::PutOperation,
                                     dropbox->clientSecret(), dropbox->tokenSecret() );
    param.append( O1RequestParameter( "oauth_signature", signature ) );

    for ( int i = 0; i < param.size() - 1; ++i ) {
        uploadUrl.addQueryItem( param.at(i).name, param.at(i).value );
    }
    uploadUrl.addQueryItem( "oauth_signature", signature.toPercentEncoding() );

    QNetworkRequest req( uploadUrl );
    req.setRawHeader( "Authorization", O1::buildAuthorizationHeader( param ) );

    qint64 toUpload = m_totalBytes - m_totalUploaded < chunkSize ? m_totalBytes - m_totalUploaded : chunkSize;
    QNetworkReply *reply = manager->put( req, data.mid( m_index, m_totalUploaded == 0 ? chunkSize : toUpload ) );
    connect( reply, SIGNAL(finished()), SLOT(chunkUploaded()) );
    connect( reply, SIGNAL(uploadProgress(qint64,qint64)), SLOT(chunkUploadProgress(qint64,qint64)) );
}

void nub::chunkUploaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox::critical( this, QString(), reply->errorString() );
        ui->progressBar->hide();
        ui->uploadButton->show();
        return;
    }

    if ( uploadId() == "" ) {
        // This means that the first chunk has been uploaded
        // and we need to obtain the upload ID now
        QByteArray jsonOutput = reply->readAll();
        bool ok;
        QVariantMap result = QtJson::parse( QString::fromAscii( jsonOutput ), ok ).toMap();

        if( !ok ) {
            qFatal( "An error occurred during parsing the JSON ouput" );
            return;
        }

        if ( result.contains( "upload_id" ) && result.value( "offset" ).toInt() == chunkSize ) {
            // We hereby confirm that the first chunk has been uploaded
            // and we need to set the upload ID now
            setUploadId( result.value( "upload_id" ).toString() );
        }
        reply->deleteLater();
    }

    if ( m_totalUploaded == m_totalBytes ) {
        commitChunkUpload();
    } else {
        m_index = m_totalUploaded;
        uploadChunks();
    }
}

void nub::setUploadId( QString id )
{
    m_uploadId = id;
}

QString nub::uploadId()
{
    return m_uploadId;
}

void nub::handleProgressBar()
{
    int change = ( 100.0 / m_totalBytes ) * m_totalUploaded;
    emit uploadProgressChanged( change );
}

void nub::commitChunkUpload()
{
    QUrl commitUrl( "https://api-content.dropbox.com/1/commit_chunked_upload/dropbox/Public/" + QUrl::toPercentEncoding( fileName ) );

    QList<O1RequestParameter> param;
    param.append( O1RequestParameter( "oauth_signature_method", "HMAC-SHA1" ) );
    param.append( O1RequestParameter( "oauth_consumer_key", dropbox->clientId().toAscii() ) );
    param.append( O1RequestParameter( "oauth_version", "1.0" ) );
    param.append( O1RequestParameter( "oauth_timestamp", QString::number( QDateTime::currentDateTimeUtc().toTime_t() ).toAscii() ) );
    param.append( O1RequestParameter( "oauth_nonce", dropbox->nonce() ) );
    param.append( O1RequestParameter( "oauth_token", dropbox->token().toAscii() ) );
    param.append( O1RequestParameter( "upload_id", uploadId().toAscii() ) );
    QByteArray signature = O1::sign( param, QList<O1RequestParameter>(), commitUrl,
                                     QNetworkAccessManager::PostOperation,
                                     dropbox->clientSecret(), dropbox->tokenSecret() );

    QByteArray paramArray;
    paramArray.append( QString( "upload_id=%1&" ).arg( uploadId() ) );
    paramArray.append( QString( "oauth_consumer_key=%1&" ).arg( dropbox->clientId() ) );
    paramArray.append( QString( "oauth_token=%1&" ).arg( dropbox->token() ) );
    paramArray.append( "oauth_signature_method=HMAC-SHA1&" );
    paramArray.append( "oauth_version=1.0&" );
    paramArray.append( QString( "oauth_timestamp=%1&" ).arg( QString::number( QDateTime::currentDateTimeUtc().toTime_t() ) ) );
    paramArray.append( QString( "oauth_nonce=%1&" ).arg( QString::fromAscii( dropbox->nonce() ) ) );
    paramArray.append( QString( "oauth_signature=%1" ).arg( QString::fromAscii( signature ) ) );

    QNetworkRequest req( commitUrl );
    req.setRawHeader( "Authorization", O1::buildAuthorizationHeader( param ) );
    req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
    QNetworkReply *reply = manager->post( req, paramArray );
    connect( reply, SIGNAL(finished()), SLOT(chunkUploadCommitted()) );
}

void nub::chunkUploadCommitted()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox::critical( this, QString(), reply->errorString() );
        ui->progressBar->hide();
        ui->uploadButton->show();
        return;
    }

    ui->progressBar->hide();
    ui->successLabel->show();
    ui->uploadButton->show();
    reply->deleteLater();
}
