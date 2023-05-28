# Compiling with Visual C++ 6

The main goal of this project is to provide a universal way of sharing files
between devices, so in order for us to accomplish that we are required to
establish a common denominator on all platforms. On Windows this means that we
must be capable of compiling under [Visual C++ 6.0](https://winworldpc.com/product/microsoft-visual-stu/60).

You'll find a Visual C++ 6 workspace file that you can open and compile the
entire project in the `win32/vs6` directory.

All of the steps necessary to get the code base to compile under such an old
compiler have been taken into consideration, but there are some issues that are
completely out of our hands and must be fixed manually on each development
system.

## Windows Server 2003 Platform SDK (February 2003)

In order to make Visual C++ 6 more palatable and play ball with our "modern-ish"
code base, specially when dealing with the network stack, you'll be required to
install Microsoft's [Windows Server 2003 Platform SDK (February 2003)](https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/e1147034-9b0b-4494-a5bc-6dfebb6b7eb1/download-and-install-microsoft-platform-sdk-febuary-2003-last-version-with-vc6-support?forum=windowssdk).

## Fixing C/C++ Overloaded Functions

Since our library is written in C89, for maximum compatility, but our Windows
application is written in C++, in order to make Win32 development easier, if you
try to compile the project for the first time you'll encounter the following
compilation errors:

```
c:\program files\microsoft visual studio\vc98\include\wchar.h(700) : error C2733: second C linkage of overloaded function 'wmemchr' not allowed
        c:\program files\microsoft visual studio\vc98\include\wchar.h(699) : see declaration of 'wmemchr'
c:\program files\microsoft visual studio\vc98\include\wchar.h(702) : error C2733: second C linkage of overloaded function 'wcschr' not allowed
        c:\program files\microsoft visual studio\vc98\include\wchar.h(701) : see declaration of 'wcschr'
c:\program files\microsoft visual studio\vc98\include\wchar.h(704) : error C2733: second C linkage of overloaded function 'wcspbrk' not allowed
        c:\program files\microsoft visual studio\vc98\include\wchar.h(703) : see declaration of 'wcspbrk'
c:\program files\microsoft visual studio\vc98\include\wchar.h(706) : error C2733: second C linkage of overloaded function 'wcsrchr' not allowed
        c:\program files\microsoft visual studio\vc98\include\wchar.h(705) : see declaration of 'wcsrchr'
c:\program files\microsoft visual studio\vc98\include\wchar.h(708) : error C2733: second C linkage of overloaded function 'wcsstr' not allowed
        c:\program files\microsoft visual studio\vc98\include\wchar.h(707) : see declaration of 'wcsstr'
```

This issues are manifesting themselves due to the fact that Microsoft has
overloaded some C standard functions in the `wchar.h` header specially for C++
(this was Microsoft at it's finest in this era of their development tools),
which becomes an issue since it's trying to compile those using the C compiler
when building the Windows application.

In order to fix this issue we are required to edit the header in which these
overloaded functions are defined. If you double-click on the first error Visual
Studio should take you to the offending section:

```c
inline wchar_t *wmemchr(wchar_t *_S, wchar_t _C, size_t _N)
        {return ((wchar_t *)wmemchr((const wchar_t *)_S, _C, _N)); }
inline wchar_t *wcschr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcschr((const wchar_t *)_S, _C)); }
inline wchar_t *wcspbrk(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_S, _P)); }
inline wchar_t *wcsrchr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_S, _C)); }
inline wchar_t *wcsstr(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcsstr((const wchar_t *)_S, _P)); }
```

As you can see these C standard functions got some C++ overloads for no good
reason, so theoretically commenting them out of the header should make no
practical difference.

You can disable the offending section by doing something similar to this:

```c
/* Disabled due to conlicts with projects that mix C with C++. */
#if 0
inline wchar_t *wmemchr(wchar_t *_S, wchar_t _C, size_t _N)
        {return ((wchar_t *)wmemchr((const wchar_t *)_S, _C, _N)); }
inline wchar_t *wcschr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcschr((const wchar_t *)_S, _C)); }
inline wchar_t *wcspbrk(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcspbrk((const wchar_t *)_S, _P)); }
inline wchar_t *wcsrchr(wchar_t *_S, wchar_t _C)
        {return ((wchar_t *)wcsrchr((const wchar_t *)_S, _C)); }
inline wchar_t *wcsstr(wchar_t *_S, const wchar_t *_P)
        {return ((wchar_t *)wcsstr((const wchar_t *)_S, _P)); }
#endif /* Disabled */
```

Now you should be able to build the project.
