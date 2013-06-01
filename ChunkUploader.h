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

#ifndef CHUNKUPLOADER_H
#define CHUNKUPLOADER_H

// Qt
#include <QWidget>
#include <QFile>
#include <QNetworkAccessManager>

// o2
#include "o2/o1dropbox.h"

class QByteArray;

enum AppType {
    FullDropbox,
    AppFolder
};

class ChunkUploader : public QWidget
{
    Q_OBJECT
public:
    explicit ChunkUploader( QWidget *parent );
    ~ChunkUploader();

    void initialise( QString file );
    void setClients( O1Dropbox *dropboxClient, QNetworkAccessManager *managerClient );
    void setAppType( AppType type );

signals:
    void uploadingChunks();
    void uploadProgressChanged( int change );
    void errorOccurred();
    void uploadSuccessful();

private slots:
    void chunkUploadProgress( qint64 uploaded, qint64 );
    void checkedForDuplicates();
    void chunkUploaded();
    void chunkUploadCommitted();

private:
    O1Dropbox *m_dropbox;
    QNetworkAccessManager *m_manager;

    QFile m_selectedFile;
    QByteArray m_data;
    QString m_fileName;
    QString m_uploadId;
    qint64 m_chunkSize;
    qint64 m_totalBytes;
    qint64 m_totalUploaded;
    qint64 m_index;
    qint64 m_uploadedLast;
    AppType m_appType;

    void checkForDuplicates();
    void uploadChunks();
    void handleProgressBar();
    void commitChunkUpload();
};

#endif
