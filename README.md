#nub v0.2

<iframe style="border: 0; margin: 0; padding: 0;" src="https://www.gittip.com/binaryking/widget.html" width="48pt" height="22pt"></iframe>

nub uses the Dropbox Core API to upload files to your Dropbox account. Refer to https://www.dropbox.com/developers/core/docs#chunked-upload.

##Getting Started
To get started, follow these steps:

Set your App key and secret that you can obtain from the [Dropbox Apps Console](https://www.dropbox.com/developers/apps).
<br>Next, in `nub.cpp` change `APP_KEY` and `APP_SECRET` to your app's key and secret and `APP_TYPE` to `FullDropbox` if your app's permission is to access the user's Full Dropbox folder or to `AppFolder` if your app's permission is to access the App folder only.<br>Compile! and enjoy.

##How nub works?
Once you've logged in, the upload button appears. Click on it, select a file and click Open. After this, just sit back and see the progress bar upload your file in chunks of 4MB (if the file size is greater than 4MB) to the Dropbox server with HTTP PUT requests. If, by any chance, there occurs an error, nub will notify you via an error dialog.

##Changelog:
`v0.1 - Casual release<br>
v0.2 - Bug fixes`

##Credits:
o2 - OAuth library for Qt - https://github.com/pipacs/o2<br>
qt-json - Simple JSON parser for Qt - https://github.com/ereilin/qt-json