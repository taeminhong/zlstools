# What Is This Project?

zlstools is a collection of command-line tools and shell extensions for viewing or extract data from `.zls` files.

Included Tools:

- zlsthumb: Extracts draping item images from `.zls` files.
- ZlsThumbnailprovider: A Windows Explorer extension that enables `.zls` file previews.

# How To Build

You'll need the following programs to build the installer:

- [Visual Studio 2015 or higher](https://visualstudio.microsoft.com/vs/express/).
- [Inno Setup 6.2 or higher](https://jrsoftware.org/isinfo.php).

Build steps:

1. Open zlstools.sln in Visual Studio.
2. From the mune, select "Build" > "Batch Build...".
3. Click "Select All" and "Build".
4. Open setup.iss in Inno Setup compiler.
5. From the menu, select "Build" > "Complie".

The installer(zlstools-<version>.exe) will be generated in the project root directory.

# Miniz

This project depends on Miniz, a compression library. A minor modification has been made to suit this project's needs.
Miniz is distributed under the MIT License. For more details, please visit the [Miniz project page](https://github.com/richgel999/miniz).