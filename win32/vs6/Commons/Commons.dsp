# Microsoft Developer Studio Project File - Name="Commons" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Commons - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Commons.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Commons.mak" CFG="Commons - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Commons - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Commons - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Commons - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\common\std" /I "..\..\..\common\win32" /D "NDEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"glcommon.lib"

!ELSEIF  "$(CFG)" == "Commons - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\common\win32" /D "_DEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"glcommon.lib"

!ENDIF 

# Begin Target

# Name "Commons - Win32 Release"
# Name "Commons - Win32 Debug"
# Begin Group "Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\utils\bits.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\filesystem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\filesystem.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\threads.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\threads.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\utf16.c
# End Source File
# Begin Source File

SOURCE=..\..\..\common\utils\utf16.h
# End Source File
# End Group
# Begin Group "Standard Libraries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\common\win32\inttypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\win32\ssize_t.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\win32\stdbool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\win32\stdint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\common\win32\stdshim.h
# End Source File
# End Group
# End Target
# End Project
