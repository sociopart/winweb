# ![WinWeb](/docs/resources/ww-logo.png)

WinWeb is a powerful and easy-to-use C/C++ library designed to
simplify file downloads and interaction with the Windows API. 

It provides a set of functions, constants, and data structures that enable
developers to effortlessly download files from URLs and handle various aspects
of the download process.

Goals of this project:
- Be fast and backward-compatible. Using C99 and WinInet, you can achieve two
  goals at once: WinInet is present (or might be installed) since 3.11, and C99
  is supported probably everywhere.
- Be easy. You can download a file with a few lines of code!
- Be portable. Since nothing extremely new is not implemented, you can easily
  get a 20KB executable that downloads stuff
  (see [examples](/docs/0-docs.md)).
- Be flexible. But you still do plenty of things, such as displaying raw HTTP
  headers, showing log, customising the progressbar look... Thousands of them!

Feel free to use and contribute! 

## Downloading the source code

To download the latest source from the Git server, do this:
```
git clone https://github.com/sociopart/winweb.git
```

## Documentation

You can find all answers to your frequent questions in
[the Documentation](/docs/0-docs.md). 

