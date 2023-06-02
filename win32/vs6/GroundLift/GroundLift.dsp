# Microsoft Developer Studio Project File - Name="GroundLift" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=GroundLift - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GroundLift.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GroundLift.mak" CFG="GroundLift - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GroundLift - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "GroundLift - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "GroundLift - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\common\win32" /I "..\..\..\common" /D "NDEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"groundlift.lib"

!ELSEIF  "$(CFG)" == "GroundLift - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\common\win32" /I "..\..\..\common" /D "_DEBUG" /D "WIN32_LEAN_AND_MEAN" /D "WIN32" /D "UNICODE" /D "_UNICODE" /D "_LIB" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"groundlift.lib"

!ENDIF 

# Begin Target

# Name "GroundLift - Win32 Release"
# Name "GroundLift - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\src\groundlift\client.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\conf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\error.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\obex.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\server.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\sockets.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\src\groundlift\client.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\conf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\error.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\obex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\server.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\groundlift\sockets.h
# End Source File
# End Group
# Begin Group "Configuration"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\groundlift\defaults.h
# End Source File
# End Group
# End Target
# End Project
