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
#include "ChunkUploader.h"

// Qt
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkReply>
#include <QDebug>

// o2
#include "o2/o1.h"

// qt-json
#include "qt-json/json.h"

ChunkUploader::ChunkUploader( QWidget *parent ) :
    QWidget( parent ),
    m_data(),
    m_fileName(),
    m_uploadId(),
    m_chunkSize( 4*1024*1024 ),
    m_totalBytes( 0 ),
    m_totalUploaded( 0 ),
    m_index( 0 ),
    m_uploadedLast( 0 )
{
}

ChunkUploader::~ChunkUploader()
{
}

void ChunkUploader::initialise( QString file )
{
    m_selectedFile.setFileName( file );
    if (  m_selectedFile.bytesAvailable() < m_chunkSize ) {
        /** @brief The file is less than 4MB so we need to calculate the appropriate
         * chunk size for this upload. For ease of use, we'll keep chunk size
         * half the size of the file as during upload, first we need to obtain an
         * upload ID and then upload the other chunks. In this case, first we obtain the
         * upload ID and then upload the next half of the file. */
        m_chunkSize = m_selectedFile.bytesAvailable() / 2;
    }

    if ( m_selectedFile.open( QIODevice::ReadOnly ) ) {
        m_fileName = QFileInfo( m_selectedFile ).fileName();
        if ( m_fileName.toAscii().toPercentEncoding().contains( "%20" ) ) {
            // The filename contains a space which we need to remove
            QStringList list = m_fileName.split( " " );
            m_fileName = QString();
            for ( int i = 0; i < list.size(); ++i ) {
                m_fileName += i == list.size()-1 ? list.at(i) : list.at(i) + "_";
            }
        }
        m_totalBytes = m_selectedFile.bytesAvailable();
        checkForDuplicates();
    } else {
        QMessageBox::critical( parentWidget(), QString(), "Sorry but this file type is not supported"
                               ". Please select an appropriate file to start uploading." );
    }
}

void ChunkUploader::setClients( O1Dropbox *dropboxClient, QNetworkAccessManager *managerClient )
{
    m_dropbox = dropboxClient;
    m_manager = managerClient;
}

void ChunkUploader::setAppType( AppType type )
{
    m_appType = type;
}

void ChunkUploader::chunkUploadProgress( qint64 uploaded, qint64 )
{
    m_uploadedLast = uploaded;
    m_totalUploaded = m_index + m_uploadedLast;
    handleProgressBar();
}

void ChunkUploader::checkForDuplicates()
{
    QString url;
    if ( m_appType == FullDropbox ) {
        url = "https://api.dropbox.com/1/search/dropbox/Public";
    } else if ( m_appType == AppFolder ) {
        url = "https://api.dropbox.com/1/search/sandbox";
    }

    QUrl searchUrl( url );
    QList<O1RequestParameter> param;
    param.append( O1RequestParameter( "oauth_signature_method", "HMAC-SHA1" ) );
    param.append( O1RequestParameter( "oauth_consumer_key", m_dropbox->clientId().toAscii() ) );
    param.append( O1RequestParameter( "oauth_version", "1.0" ) );
    param.append( O1RequestParameter( "oauth_timestamp", QString::number( QDateTime::currentDateTimeUtc().toTime_t() ).toAscii() ) );
    param.append( O1RequestParameter( "oauth_nonce", m_dropbox->nonce() ) );
    param.append( O1RequestParameter( "oauth_token", m_dropbox->token().toAscii() ) );
    param.append( O1RequestParameter( "query", m_fileName.toAscii() ) );
    QByteArray signature = O1::sign( param, QList<O1RequestParameter>(), searchUrl,
                                     QNetworkAccessManager::GetOperation,
                                     m_dropbox->clientSecret(), m_dropbox->tokenSecret() );
    param.append( O1RequestParameter( "oauth_signature", signature ) );

    for ( int i = 0; i < param.size() - 2; ++i ) {
        searchUrl.addQueryItem( param.at(i).name, param.at(i).value );
    }
    searchUrl.addQueryItem( "oauth_signature", signature.toPercentEncoding() );
    searchUrl.addQueryItem( "query", m_fileName );

    QNetworkRequest req( searchUrl );
    req.setRawHeader( "Authorization", O1::buildAuthorizationHeader( param ) );

    QNetworkReply *reply = m_manager->get( req );
    connect( reply, SIGNAL(finished()), SLOT(checkedForDuplicates()) );
}

void ChunkUploader::checkedForDuplicates()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox::critical( parentWidget(), QString(), reply->errorString() );
        emit errorOccurred();
        return;
    }

    QByteArray jsonOutput = reply->readAll();
    bool ok;
    QVariantMap result = QtJson::parse( QString::fromAscii( jsonOutput ), ok ).toMap();

    if( !ok ) {
        qFatal( "An error occurred during parsing the JSON ouput" );
        return;
    }

    if ( !jsonOutput.contains( "path" ) ) {
        uploadChunks();
        qDebug() << "I'm here";
    } else {
        qDebug() << "I'm here 1";
        QMessageBox *msgBox = new QMessageBox( parentWidget() );
        QString folder = m_appType == FullDropbox ? "Dropbox Folder" : "App Folder";
        msgBox->setText( QString( "This file already exists in your " + folder + ". Do you want to overwrite it?" ) );
        msgBox->setStandardButtons( QMessageBox::Yes | QMessageBox::No );
        msgBox->setDefaultButton( QMessageBox::No );
        int ret = msgBox->exec();
        switch ( ret ) {
        case QMessageBox::Yes:
            uploadChunks();
            break;
        case QMessageBox::No:
            emit errorOccurred();
            break;
        }
    }
    reply->deleteLater();
}

void ChunkUploader::uploadChunks()
{
    qint64 offset = m_totalBytes - m_totalUploaded < m_chunkSize ? m_totalBytes - m_totalUploaded : m_chunkSize;
    qint64 toUpload = m_totalUploaded == 0 ? m_chunkSize : offset;
    m_data = m_selectedFile.read( toUpload );

    QUrl uploadUrl( "https://api-content.dropbox.com/1/chunked_upload" );
    QList<O1RequestParameter> param;
    param.append( O1RequestParameter( "oauth_signature_method", "HMAC-SHA1" ) );
    param.append( O1RequestParameter( "oauth_consumer_key", m_dropbox->clientId().toAscii() ) );
    param.append( O1RequestParameter( "oauth_version", "1.0" ) );
    param.append( O1RequestParameter( "oauth_timestamp", QString::number( QDateTime::currentDateTimeUtc().toTime_t() ).toAscii() ) );
    param.append( O1RequestParameter( "oauth_nonce", m_dropbox->nonce() ) );
    param.append( O1RequestParameter( "oauth_token", m_dropbox->token().toAscii() ) );
    if ( !m_uploadId.isNull() ) {
        // First chunk has been uploaded and we have an upload ID
        param.append( O1RequestParameter( "upload_id", m_uploadId.toAscii() ) );
        param.append( O1RequestParameter( "offset", QString::number( m_totalUploaded ).toAscii() ) );
    }
    QByteArray signature = O1::sign( param, QList<O1RequestParameter>(), uploadUrl,
                                     QNetworkAccessManager::PutOperation,
                                     m_dropbox->clientSecret(), m_dropbox->tokenSecret() );
    param.append( O1RequestParameter( "oauth_signature", signature ) );

    for ( int i = 0; i < param.size() - 1; ++i ) {
        uploadUrl.addQueryItem( param.at(i).name, param.at(i).value );
    }
    uploadUrl.addQueryItem( "oauth_signature", signature.toPercentEncoding() );

    QNetworkRequest req( uploadUrl );
    req.setRawHeader( "Authorization", O1::buildAuthorizationHeader( param ) );


    QNetworkReply *reply = m_manager->put( req, m_data );
    connect( reply, SIGNAL(finished()), SLOT(chunkUploaded()) );
    connect( reply, SIGNAL(uploadProgress(qint64,qint64)), SLOT(chunkUploadProgress(qint64,qint64)) );
}

void ChunkUploader::chunkUploaded()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox::critical( parentWidget(), QString(), reply->errorString() );
        emit errorOccurred();
        return;
    }

    if ( m_uploadId.isNull() ) {
        // This means that the first chunk has been uploaded
        // and we need to obtain the upload ID now
        QByteArray jsonOutput = reply->readAll();
        bool ok;
        QVariantMap result = QtJson::parse( QString::fromAscii( jsonOutput ), ok ).toMap();

        if( !ok ) {
            qFatal( "An error occurred during parsing the JSON ouput" );
            return;
        }

        if ( result.contains( "upload_id" ) && result.value( "offset" ).toInt() == m_chunkSize ) {
            // We hereby confirm that the first chunk has been uploaded
            // and we need to set the upload ID now
            m_uploadId = result.value( "upload_id" ).toString();
        }
    }

    if ( m_totalUploaded == m_totalBytes ) {
        commitChunkUpload();
    } else {
        m_index = m_totalUploaded;
        m_selectedFile.seek( m_index );
        uploadChunks();
    }
    reply->deleteLater();
}

void ChunkUploader::handleProgressBar()
{
    int change = ( 100.0 / m_totalBytes ) * m_totalUploaded;
    emit uploadProgressChanged( change );
}

void ChunkUploader::commitChunkUpload()
{
    QString url;
    if ( m_appType == FullDropbox ) {
        url = "https://api-content.dropbox.com/1/commit_chunked_upload/dropbox/Public/" + QUrl::toPercentEncoding( m_fileName );
    } else if ( m_appType == AppFolder ) {
        url = "https://api-content.dropbox.com/1/commit_chunked_upload/sandbox/" + QUrl::toPercentEncoding( m_fileName );
    }

    QUrl commitUrl( url );

    QList<O1RequestParameter> param;
    param.append( O1RequestParameter( "oauth_signature_method", "HMAC-SHA1" ) );
    param.append( O1RequestParameter( "oauth_consumer_key", m_dropbox->clientId().toAscii() ) );
    param.append( O1RequestParameter( "oauth_version", "1.0" ) );
    param.append( O1RequestParameter( "oauth_timestamp", QString::number( QDateTime::currentDateTimeUtc().toTime_t() ).toAscii() ) );
    param.append( O1RequestParameter( "oauth_nonce", m_dropbox->nonce() ) );
    param.append( O1RequestParameter( "oauth_token", m_dropbox->token().toAscii() ) );
    param.append( O1RequestParameter( "upload_id", m_uploadId.toAscii() ) );
    QByteArray signature = O1::sign( param, QList<O1RequestParameter>(), commitUrl,
                                     QNetworkAccessManager::PostOperation,
                                     m_dropbox->clientSecret(), m_dropbox->tokenSecret() );
    param.append( O1RequestParameter( "oauth_signature", signature ) );

    QByteArray paramArray;
    paramArray.append( QString( "upload_id=%1&" ).arg( m_uploadId ) );
    paramArray.append( QString( "oauth_consumer_key=%1&" ).arg( m_dropbox->clientId() ) );
    paramArray.append( QString( "oauth_token=%1&" ).arg( m_dropbox->token() ) );
    paramArray.append( "oauth_signature_method=HMAC-SHA1&" );
    paramArray.append( "oauth_version=1.0&" );
    paramArray.append( QString( "oauth_timestamp=%1&" ).arg( QString::number( QDateTime::currentDateTimeUtc().toTime_t() ) ) );
    paramArray.append( QString( "oauth_nonce=%1&" ).arg( QString::fromAscii( m_dropbox->nonce() ) ) );
    paramArray.append( QString( "oauth_signature=%1" ).arg( QString::fromAscii( signature ) ) );

    QNetworkRequest req( commitUrl );
    req.setRawHeader( "Authorization", O1::buildAuthorizationHeader( param ) );
    req.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
    QNetworkReply *reply = m_manager->post( req, paramArray );
    connect( reply, SIGNAL(finished()), SLOT(chunkUploadCommitted()) );
}

void ChunkUploader::chunkUploadCommitted()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
    if ( reply->error() != QNetworkReply::NoError ) {
        QMessageBox::critical( parentWidget(), QString(), reply->errorString() );
        qDebug() << reply->error();
        emit errorOccurred();
        return;
    }

    m_selectedFile.close();
    emit uploadSuccessful();
    reply->deleteLater();
}
