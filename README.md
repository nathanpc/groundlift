# GroundLift

The answer to the question: "What if AirDrop was made for UNIX?"

This project aims to provide a universal way of sharing files and information
quickly between two devices, no matter how old or weird these devices actually
are.

## Compiling

Compiling this project is fairly easy since it's designed to be self-contained,
requiring no external dependencies apart from a plain C89 build environment.

### UNIX

If you're compiling the project under a UNIX variant, and you have access to the
[GNU Make](https://www.gnu.org/software/make/) build system. All you have to do
is run the following commands to compile, and optionally install, the
applications:

```bash
make
sudo make install # Optional (set the PREFIX variable to decide where to install)
```

### Windows

Although this is UNIX software, a great deal of care has been taken to ensure
that it's also fully compatible with Windows, using the official Microsoft tools
for development and compiling, thus ensuring full native compatibility.

#### Compiling with Visual C++ 6

The main goal of this project is to provide a universal way of sharing files
between devices, so in order for us to accomplish that we are required to
establish a common denominator on all platforms. On Windows this means that we
must be capable of compiling under [Visual C++ 6.0](https://winworldpc.com/product/microsoft-visual-stu/60).

You'll find a Visual C++ 6 workspace file that you can open and compile the
entire project in the `win32/vs6` directory.

In order to make Visual C++ 6 more palatable and play ball with our "modern-ish"
codebase, specially when dealing with the network stack, you'll be required to
install Microsoft's [Windows Server 2003 Platform SDK (February 2003)](https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/e1147034-9b0b-4494-a5bc-6dfebb6b7eb1/download-and-install-microsoft-platform-sdk-febuary-2003-last-version-with-vc6-support?forum=windowssdk).

## License

This project is licensed under the
[Mozilla Public License Version 2.0](https://www.mozilla.org/en-US/MPL/2.0/).
