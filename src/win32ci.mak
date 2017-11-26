# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "WIN32CI.MAK" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : .\netci.exe $(OUTDIR)/WIN32CI.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_WINDOWS" /FR /c
CPP_PROJ=/nologo /W3 /GX /YX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "USE_WINDOWS" /FR$(INTDIR)/ /Fp$(OUTDIR)/"WIN32CI.pch" /Fo$(INTDIR)/ /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"NETCI.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"WIN32CI.bsc" 
BSC32_SBRS= \
	$(INTDIR)/INTRFACE.SBR \
	$(INTDIR)/SYS7.SBR \
	$(INTDIR)/COMPILE2.SBR \
	$(INTDIR)/EDIT.SBR \
	$(INTDIR)/TOKEN.SBR \
	$(INTDIR)/SYS3.SBR \
	$(INTDIR)/PREPROC.SBR \
	$(INTDIR)/CACHE1.SBR \
	$(INTDIR)/SYS8.SBR \
	$(INTDIR)/TABLE.SBR \
	$(INTDIR)/SYS4.SBR \
	$(INTDIR)/CACHE2.SBR \
	$(INTDIR)/OPER1.SBR \
	$(INTDIR)/CLEARQ.SBR \
	$(INTDIR)/SYS5.SBR \
	$(INTDIR)/DBHANDLE.SBR \
	$(INTDIR)/SYS6A.SBR \
	$(INTDIR)/OPER2.SBR \
	$(INTDIR)/SYS1.SBR \
	$(INTDIR)/MAIN.SBR \
	$(INTDIR)/GLOBALS.SBR \
	$(INTDIR)/INTERP.SBR \
	$(INTDIR)/FILE.SBR \
	$(INTDIR)/COMPILE1.SBR \
	$(INTDIR)/CONSTRCT.SBR \
	$(INTDIR)/SYS6B.SBR \
	$(INTDIR)/SYS2.SBR \
	$(INTDIR)/WINMAIN.SBR

$(OUTDIR)/WIN32CI.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /NOLOGO /SUBSYSTEM:windows /MACHINE:I386 /OUT:"netci.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /NOLOGO /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"WIN32CI.pdb" /MACHINE:I386 /OUT:"netci.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/INTRFACE.OBJ \
	$(INTDIR)/SYS7.OBJ \
	$(INTDIR)/COMPILE2.OBJ \
	$(INTDIR)/EDIT.OBJ \
	$(INTDIR)/TOKEN.OBJ \
	$(INTDIR)/SYS3.OBJ \
	$(INTDIR)/PREPROC.OBJ \
	$(INTDIR)/CACHE1.OBJ \
	$(INTDIR)/SYS8.OBJ \
	$(INTDIR)/TABLE.OBJ \
	$(INTDIR)/SYS4.OBJ \
	$(INTDIR)/CACHE2.OBJ \
	$(INTDIR)/OPER1.OBJ \
	$(INTDIR)/CLEARQ.OBJ \
	$(INTDIR)/SYS5.OBJ \
	$(INTDIR)/DBHANDLE.OBJ \
	$(INTDIR)/SYS6A.OBJ \
	$(INTDIR)/OPER2.OBJ \
	$(INTDIR)/SYS1.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/GLOBALS.OBJ \
	$(INTDIR)/INTERP.OBJ \
	$(INTDIR)/FILE.OBJ \
	$(INTDIR)/COMPILE1.OBJ \
	$(INTDIR)/CONSTRCT.OBJ \
	$(INTDIR)/SYS6B.OBJ \
	$(INTDIR)/SYS2.OBJ \
	$(INTDIR)/WINMAIN.OBJ \
	$(INTDIR)/NETCI.res

.\netci.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : .\netci.exe $(OUTDIR)/WIN32CI.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /W3 /GX /Zi /YX /O2 /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_WINDOWS" /D "DEBUG" /FR /c
CPP_PROJ=/nologo /W3 /GX /Zi /YX /O2 /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D\
 "USE_WINDOWS" /D "DEBUG" /FR$(INTDIR)/ /Fp$(OUTDIR)/"WIN32CI.pch" /Fo$(INTDIR)/\
 /Fd$(OUTDIR)/"WIN32CI.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"NETCI.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"WIN32CI.bsc" 
BSC32_SBRS= \
	$(INTDIR)/INTRFACE.SBR \
	$(INTDIR)/SYS7.SBR \
	$(INTDIR)/COMPILE2.SBR \
	$(INTDIR)/EDIT.SBR \
	$(INTDIR)/TOKEN.SBR \
	$(INTDIR)/SYS3.SBR \
	$(INTDIR)/PREPROC.SBR \
	$(INTDIR)/CACHE1.SBR \
	$(INTDIR)/SYS8.SBR \
	$(INTDIR)/TABLE.SBR \
	$(INTDIR)/SYS4.SBR \
	$(INTDIR)/CACHE2.SBR \
	$(INTDIR)/OPER1.SBR \
	$(INTDIR)/CLEARQ.SBR \
	$(INTDIR)/SYS5.SBR \
	$(INTDIR)/DBHANDLE.SBR \
	$(INTDIR)/SYS6A.SBR \
	$(INTDIR)/OPER2.SBR \
	$(INTDIR)/SYS1.SBR \
	$(INTDIR)/MAIN.SBR \
	$(INTDIR)/GLOBALS.SBR \
	$(INTDIR)/INTERP.SBR \
	$(INTDIR)/FILE.SBR \
	$(INTDIR)/COMPILE1.SBR \
	$(INTDIR)/CONSTRCT.SBR \
	$(INTDIR)/SYS6B.SBR \
	$(INTDIR)/SYS2.SBR \
	$(INTDIR)/WINMAIN.SBR

$(OUTDIR)/WIN32CI.bsc : $(OUTDIR)  $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386 /OUT:"netci.exe"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /NOLOGO /SUBSYSTEM:windows /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"WIN32CI.pdb" /DEBUG /MACHINE:I386 /OUT:"netci.exe" 
DEF_FILE=
LINK32_OBJS= \
	$(INTDIR)/INTRFACE.OBJ \
	$(INTDIR)/SYS7.OBJ \
	$(INTDIR)/COMPILE2.OBJ \
	$(INTDIR)/EDIT.OBJ \
	$(INTDIR)/TOKEN.OBJ \
	$(INTDIR)/SYS3.OBJ \
	$(INTDIR)/PREPROC.OBJ \
	$(INTDIR)/CACHE1.OBJ \
	$(INTDIR)/SYS8.OBJ \
	$(INTDIR)/TABLE.OBJ \
	$(INTDIR)/SYS4.OBJ \
	$(INTDIR)/CACHE2.OBJ \
	$(INTDIR)/OPER1.OBJ \
	$(INTDIR)/CLEARQ.OBJ \
	$(INTDIR)/SYS5.OBJ \
	$(INTDIR)/DBHANDLE.OBJ \
	$(INTDIR)/SYS6A.OBJ \
	$(INTDIR)/OPER2.OBJ \
	$(INTDIR)/SYS1.OBJ \
	$(INTDIR)/MAIN.OBJ \
	$(INTDIR)/GLOBALS.OBJ \
	$(INTDIR)/INTERP.OBJ \
	$(INTDIR)/FILE.OBJ \
	$(INTDIR)/COMPILE1.OBJ \
	$(INTDIR)/CONSTRCT.OBJ \
	$(INTDIR)/SYS6B.OBJ \
	$(INTDIR)/SYS2.OBJ \
	$(INTDIR)/WINMAIN.OBJ \
	$(INTDIR)/NETCI.res

.\netci.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\INTRFACE.C
DEP_INTRF=\
	.\CONFIG.H\
	.\OBJECT.H\
	.\GLOBALS.H\
	.\INTERP.H\
	.\DBHANDLE.H\
	.\CONSTRCT.H\
	.\INTRFACE.H\
	.\CLEARQ.H\
	.\FILE.H\
	.\CACHE.H\
	.\EDIT.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/INTRFACE.OBJ :  $(SOURCE)  $(DEP_INTRF) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS7.C
DEP_SYS7_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS7.OBJ :  $(SOURCE)  $(DEP_SYS7_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\COMPILE2.C

$(INTDIR)/COMPILE2.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\EDIT.C

$(INTDIR)/EDIT.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TOKEN.C

$(INTDIR)/TOKEN.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS3.C
DEP_SYS3_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS3.OBJ :  $(SOURCE)  $(DEP_SYS3_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\PREPROC.C

$(INTDIR)/PREPROC.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CACHE1.C

$(INTDIR)/CACHE1.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS8.C

$(INTDIR)/SYS8.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\TABLE.C

$(INTDIR)/TABLE.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS4.C
DEP_SYS4_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS4.OBJ :  $(SOURCE)  $(DEP_SYS4_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CACHE2.C

$(INTDIR)/CACHE2.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\OPER1.C
DEP_OPER1=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/OPER1.OBJ :  $(SOURCE)  $(DEP_OPER1) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CLEARQ.C

$(INTDIR)/CLEARQ.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS5.C
DEP_SYS5_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS5.OBJ :  $(SOURCE)  $(DEP_SYS5_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\DBHANDLE.C

$(INTDIR)/DBHANDLE.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS6A.C

$(INTDIR)/SYS6A.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\OPER2.C
DEP_OPER2=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/OPER2.OBJ :  $(SOURCE)  $(DEP_OPER2) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS1.C
DEP_SYS1_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS1.OBJ :  $(SOURCE)  $(DEP_SYS1_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\MAIN.C

$(INTDIR)/MAIN.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\GLOBALS.C

$(INTDIR)/GLOBALS.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\INTERP.C

$(INTDIR)/INTERP.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\FILE.C

$(INTDIR)/FILE.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\COMPILE1.C

$(INTDIR)/COMPILE1.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\CONSTRCT.C

$(INTDIR)/CONSTRCT.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS6B.C

$(INTDIR)/SYS6B.OBJ :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\SYS2.C
DEP_SYS2_=\
	.\CONFIG.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/SYS2.OBJ :  $(SOURCE)  $(DEP_SYS2_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINMAIN.C
DEP_WINMA=\
	.\CONFIG.H\
	.\OBJECT.H\
	.\INTERP.H\
	.\INTRFACE.H\
	.\CACHE.H\
	.\CONSTRCT.H\
	.\CLEARQ.H\
	.\GLOBALS.H\
	.\DBHANDLE.H\
	.\FILE.H\
	.\INSTR.H\
	.\WINMAIN.H\
	.\TUNE.H\
	.\CI.H\
	.\STDINC.H

$(INTDIR)/WINMAIN.OBJ :  $(SOURCE)  $(DEP_WINMA) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\NETCI.RC
DEP_NETCI=\
	.\NETCI.ICO

$(INTDIR)/NETCI.res :  $(SOURCE)  $(DEP_NETCI) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

# End Source File
# End Group
# End Project
################################################################################
