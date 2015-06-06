# Microsoft Developer Studio Project File - Name="leiads" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=leiads - Win32 Lastfm Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "leiads.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "leiads.mak" CFG="leiads - Win32 Lastfm Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "leiads - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "leiads - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "leiads - Win32 Lastfm Release" (based on "Win32 (x86) Application")
!MESSAGE "leiads - Win32 Lastfm Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "leiads - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "RADIOTIME_LINN_DS" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "leiads - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "RADIOTIME_LINN_DS" /D "NOLOGFILE" /D LOGLEVEL=2 /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /verbose

!ELSEIF  "$(CFG)" == "leiads - Win32 Lastfm Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "leiads___Win32_Lastfm_Release"
# PROP BASE Intermediate_Dir "leiads___Win32_Lastfm_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Lastfm_Release"
# PROP Intermediate_Dir "Lastfm_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT BASE CPP /Fr /YX /Yc /Yu
# ADD CPP /nologo /W3 /GX /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "LASTFM" /D "RADIOTIME" /D "RADIOTIME_LINN_DS" /Fr /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "leiads - Win32 Lastfm Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "leiads___Win32_Lastfm_Debug"
# PROP BASE Intermediate_Dir "leiads___Win32_Lastfm_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Lastfm_Debug"
# PROP Intermediate_Dir "Lastfm_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D DEBUG=2 /FR /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "LASTFM" /D "RADIOTIME" /D "RADIOTIME_LINN_DS" /D "NOLOGFILE" /D LOGLEVEL=2 /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /verbose
# ADD LINK32 gtk-win32-2.0.lib gdk-win32-2.0.lib atk-1.0.lib gdk_pixbuf-2.0.lib pangowin32-1.0.lib pangocairo-1.0.lib pango-1.0.lib cairo.lib gthread-2.0.lib gobject-2.0.lib gmodule-2.0.lib glib-2.0.lib shell32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /verbose

!ENDIF 

# Begin Target

# Name "leiads - Win32 Release"
# Name "leiads - Win32 Debug"
# Name "leiads - Win32 Lastfm Release"
# Name "leiads - Win32 Lastfm Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\about.c
# End Source File
# Begin Source File

SOURCE=..\src\browser.c
# End Source File
# Begin Source File

SOURCE=..\src\debug.c
# End Source File
# Begin Source File

SOURCE=..\src\info.c
# End Source File
# Begin Source File

SOURCE=..\src\intl.c
# End Source File
# Begin Source File

SOURCE=..\src\keybd.c
# End Source File
# Begin Source File

SOURCE=..\src\lastfm.c
# End Source File
# Begin Source File

SOURCE="..\src\leia-ds.c"
# End Source File
# Begin Source File

SOURCE=.\leiads.rc
# End Source File
# Begin Source File

SOURCE=..\src\princess\linn_preamp.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\linn_source.c
# End Source File
# Begin Source File

SOURCE=..\src\md5.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\media_renderer.c
# End Source File
# Begin Source File

SOURCE=..\src\metadata.c
# End Source File
# Begin Source File

SOURCE=..\src\miscellaneous.c
# End Source File
# Begin Source File

SOURCE=..\src\now_playing.c
# End Source File
# Begin Source File

SOURCE=..\src\playlist.c
# End Source File
# Begin Source File

SOURCE=..\src\playlist_file.c
# End Source File
# Begin Source File

SOURCE=..\src\radiotime.c
# End Source File
# Begin Source File

SOURCE=..\src\select_renderer.c
# End Source File
# Begin Source File

SOURCE=..\src\settings.c
# End Source File
# Begin Source File

SOURCE=..\src\transport.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_list.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_media_renderer.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_media_server.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_private.c
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_xml.c
# End Source File
# Begin Source File

SOURCE=..\src\volume.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\src\leia-ds.h"
# End Source File
# Begin Source File

SOURCE=..\src\princess\leia.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\linn_preamp.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\linn_source.h
# End Source File
# Begin Source File

SOURCE=..\src\md5.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\media_renderer.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_media_renderer.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_media_server.h
# End Source File
# Begin Source File

SOURCE=..\src\princess\upnp_private.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\leiads.ico
# End Source File
# End Group
# Begin Group "Language Files"

# PROP Default_Filter "txt"
# Begin Source File

SOURCE=..\share\leiads_de.txt
# End Source File
# Begin Source File

SOURCE=..\share\leiads_en.txt
# End Source File
# Begin Source File

SOURCE=..\share\leiads_es.txt
# End Source File
# Begin Source File

SOURCE=..\share\leiads_fr.txt
# End Source File
# Begin Source File

SOURCE=..\share\leiads_sv.txt
# End Source File
# End Group
# Begin Source File

SOURCE=..\debian\changelog
# End Source File
# Begin Source File

SOURCE=..\Makefile
# End Source File
# End Target
# End Project
