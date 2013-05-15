#nub

nub uses the Dropbox Core API to upload files to your Dropbox account. Refer to https://www.dropbox.com/developers/core/docs#chunked-upload.

##Getting Started
All you have to do is set your Dropbox app key and app secret to get started. Compile the code and enjoy!

##How nub works?
Once you've logged in, the upload button appears. Click on it, select a file and click Open. After this, just sit back and see the progress bar upload your file in chunks of 4MB to the Dropbox server with HTTP PUT requests. If, by any chance, there occurs an error, nub will notify you via an error dialog.

##Credits:
o2 - OAuth library for Qt - https://github.com/pipacs/o2

qt-json - Simple JSON parser for Qt - https://github.com/ereilin/qt-json