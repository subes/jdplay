#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc.exe
CCC=g++.exe
CXX=g++.exe
FC=

# Macros
PLATFORM=MinGW-Windows

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Debug/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/JDPlay.o \
	${OBJECTDIR}/DirectPlay.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-LC\:/Programme/Microsoft\ DirectX\ SDK\ \(April\ 2007\)/Lib/x86

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/${PLATFORM}/jdplay.exe

dist/Debug/${PLATFORM}/jdplay.exe: dplayx.lib

dist/Debug/${PLATFORM}/jdplay.exe: dxguid.lib

dist/Debug/${PLATFORM}/jdplay.exe: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/${PLATFORM}
	${LINK.cc} -o dist/Debug/${PLATFORM}/jdplay ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/JDPlay.o: JDPlay.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/atlmfc/include -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/include -IC\:/Programme/Microsoft\ DirectX\ SDK\ \(April\ 2007\)/Include -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/atlmfc/include -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/include -IC\:/Programme/Microsoft\ DirectX\ SDK\ \(April\ 2007\)/Include -MMD -MP -MF $@.d -o ${OBJECTDIR}/JDPlay.o JDPlay.cpp

${OBJECTDIR}/DirectPlay.o: DirectPlay.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/atlmfc/include -IC\:/Programme/Microsoft\ Visual\ Studio\ 9.0/VC/include -IC\:/Programme/Microsoft\ DirectX\ SDK\ \(April\ 2007\)/Include -MMD -MP -MF $@.d -o ${OBJECTDIR}/DirectPlay.o DirectPlay.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/${PLATFORM}/jdplay.exe

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
