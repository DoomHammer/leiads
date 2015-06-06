; leiads.nsi
; Copyright (C) 2010-2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)
;
; This file is part of the Leia application.
;
; Leia is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; Leia is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with Leia.  If not, see <http://www.gnu.org/licenses/>.

!define VERSION '0.6-2'

;--------------------------------
;Include Modern UI

!include "MUI2.nsh"

;--------------------------------
;General

;Name and file
Name "Leia"
OutFile "leiads_${VERSION}_win32.exe"

;Default installation folder
InstallDir "$PROGRAMFILES\Leia"
  
;Get installation folder from registry if available
InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "InstallLocation"

;Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING

;--------------------------------
;Pages

!insertmacro MUI_PAGE_LICENSE "LICENSE"
;!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Swedish"

;--------------------------------
;Installer Section

Section ""

  SetOutPath $INSTDIR
  SetShellVarContext all  ; default=current

  File COPYING
  File .\win32\Lastfm_Release\leiads.exe
  File .\share\*.*
  
  CreateShortCut "$SMPROGRAMS\Leia.lnk" "$INSTDIR\leiads.exe"
  CreateShortCut "$DESKTOP\Leia.lnk" "$INSTDIR\leiads.exe"

  ; Write the uninstall keys for Windows
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "Comments" "An UPnP Audio Control Point Application"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "DisplayIcon" "$INSTDIR\leiads.exe,0"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "DisplayName" "Leia UPnP Audio Control Point (${VERSION})"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "DisplayVersion" "${VERSION}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "NoRepair" 1
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "Publisher" "Axel Sommerfeldt"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "URLInfoAbout" "http://leia.sommerfeldt.f-m.fm"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads" "URLUpdateInfo" "http://leia.sommerfeldt.f-m.fm"

  ;Create uninstaller
  WriteUninstaller "Uninstall.exe"

SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\*.*"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir  "$INSTDIR"

  SetShellVarContext current
  Delete "$SMPROGRAMS\Leia.lnk"
  Delete "$DESKTOP\Leia.lnk"

  SetShellVarContext all
  Delete "$SMPROGRAMS\Leia.lnk"
  Delete "$DESKTOP\Leia.lnk"

  DeleteRegKey HKLM "Software\leiads"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\leiads"

SectionEnd
