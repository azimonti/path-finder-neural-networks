#!/bin/bash

displayusage() {
    echo " =================================================================================== "
    echo "|    Usage:                                                                         |"
    echo "| cbuild.sh OPTS                                                                    |"
    echo "|    available options [OPTS]:                                                      |"
    echo "| -b) --build)            automatically updates the build when necessary            |"
    echo "| -c) --clean)            removes build dirs                                        |"
    echo "| -d) --dry-run)          creates the make file without building                    |"
    echo "| -f) --force)            forces an update of the build                             |"
    echo "| -h) --help)             print this help                                           |"
    echo "| -m) --make)             performs make                                             |"
	echo "| -n) --nproc)            sets the number of parallel processing (default nproc -1) |"
    echo "| -o) --build-one)        build a single test. Equivalent to \"-w -DBUILDSINGLE=NAME\"|"
    echo "| -r) --recompile)        continuously build when a file is saved                   |"
    echo "| -s) --build-suite)      build test suite. Equivalent to \"-w -DBUILDSUITE=ON\"    |"
    echo "| -t) --build-type)       specifies a different cmake build type (e.g. \"-t Debug\")  |"
    echo "| -u) --no-unity-build)   do not use unity build. Equivalent to \"-w -NOUNITYBUILD=ON\"  |"
    echo "| -w) --cmake-params)     specifies cmake options in quoted (e.g. \"-DVAR=value\")    |"
    echo "| -z) --analyze)          run scan-build                                            |"
    echo "| [no arguments]          automatically updates the build when necessary            |"
    echo " =================================================================================== "
}

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     MACHINE=linux;;
    Darwin*)    MACHINE=macos;;
    CYGWIN*)    MACHINE=win;;
    MINGW*)     MACHINE=win;;
    *)          echo "unsupported architecture"; exit 1
esac

# Set all option to FALSE
ANALYZE="FALSE"
BUILD="FALSE"
CLEANBUILD="FALSE"
DRY_RUN="FALSE"
UPDATEMAKEFILES="FALSE"
EMAKE="FALSE"
CONTINUOUSCOMPILE="FALSE"

BASHSCRIPTDIR="$(cd "$(dirname "$0")" || exit; pwd)"
CURRENTDIR="$(pwd)"
SOURCEDIR="${CURRENTDIR}"

if [[ ! -f "${SOURCEDIR}/CMakeLists.txt" ]] ; then
	echo "CMakeLists.txt not found. Exiting..."
	exit 1
fi

update_makefiles(){
    mkdir -p "${BUILDDIR}"
    CURDIR="$(pwd)"
    cd "${BUILDDIR}" || exit

	if [ "${MACHINE}" == "macos" ]; then
        cmake "${SOURCEDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    elif [ "${MACHINE}" == "linux" ]; then
        cmake "${SOURCEDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    elif [ "${MACHINE}" == "win" ]; then
        WINARCH="x64"
        if [[ $PROCESSOR_IDENTIFIER == *"ARM"* ]]; then WINARCH="ARM64"; fi
        cmake -A "${WINARCH}" "${SOURCEDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    fi
	CMAKE_RET=$?
	if [ $CMAKE_RET -ne 0 ] ; then exit ${CMAKE_RET}; fi
    local BT="${BUILDTYPE:-Release}"
    echo "$(tr '[:lower:]' '[:upper:]' <<< "${BT:0:1}")$(tr '[:upper:]' '[:lower:]' <<< "${BT:1}")" > "${BUILDTYPEFILE}"
    cd "${CURDIR}" || exit
    MAKEFILEUPDATED="TRUE"
}

build(){
    if [[ ! -f "${NBFILES}" ]] ; then mkdir -p "${BUILDDIR}";  echo 0 > "${NBFILES}" ; fi
    PREVFILES=$(<"${NBFILES}")
    CURRFILES=$(find "${SOURCEDIR}" |  wc -l)

    if [[ "${MAKEFILEUPDATED}" == "TRUE" ]] || [[ "${CURRFILES}" -ne "${PREVFILES}" ]] ; then update_makefiles ; fi

	if [ "${MACHINE}" == "win" ]; then
		CMAKE_BUILD_PARALLEL_LEVEL=3 cmake --build "${BUILDDIR}" --config "${BUILDTYPE:-Release}"
	else
		cmake --build "${BUILDDIR}" --config "${BUILDTYPE:-Release}" -j "${NPROC}"
	fi
	CMAKE_RET=$?
	echo "${CURRFILES}" > "${NBFILES}"
	if [ $CMAKE_RET -ne 0 ] ; then exit ${CMAKE_RET}; fi
}

analyze(){
    mkdir -p "${BUILDDIR}"
    cd "${BUILDDIR}" || exit

	if [ "${MACHINE}" == "macos" ]; then
        scan-build cmake "${SOURCEDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    else
        echo "unsupported architecture"; exit 1
    fi
	CMAKE_RET=$?
	if [ $CMAKE_RET -ne 0 ] ; then exit ${CMAKE_RET}; fi
    local BT="${BUILDTYPE:-Release}"
    echo "$(tr '[:lower:]' '[:upper:]' <<< "${BT:0:1}")$(tr '[:upper:]' '[:lower:]' <<< "${BT:1}")" > "${BUILDTYPEFILE}"
	cd - || exit

    scan-build cmake --build "${BUILDDIR}" --config "${BUILDTYPE:-Release}" -j "${NPROC}"
    CMAKE_RET=$?
	if [ $CMAKE_RET -ne 0 ] ; then exit ${CMAKE_RET}; fi
}

for arg in "$@"; do
	shift
	case "$arg" in
		"--build")           set -- "$@" "-b" ;;
		"--clean")           set -- "$@" "-c" ;;
		"--dry-run")         set -- "$@" "-d" ;;
		"--force")           set -- "$@" "-f" ;;
		"--help")            set -- "$@" "-h" ;;
		"--make")            set -- "$@" "-m" ;;
		"--nproc")           set -- "$@" "-n" ;;
		"--build-one")       set -- "$@" "-o" ;;
		"--recompile")       set -- "$@" "-r" ;;
		"--build-suite")     set -- "$@" "-s" ;;
		"--build-type")      set -- "$@" "-t" ;;
        "--no-unity-build")  set -- "$@" "-u" ;;
		"--cmake-params")    set -- "$@" "-w" ;;
		"--analyze")         set -- "$@" "-z" ;;
		*)                   set -- "$@" "$arg";;
	esac
done

# Parse short options
OPTIND=1
while getopts "bcdfhmn:o:rst:uw:z?" opt
do
	case "$opt" in
        "b") BUILD="TRUE";;
		"c") CLEANBUILD="TRUE";;
		"d") UPDATEMAKEFILES="TRUE"; DRY_RUN="TRUE" ;;
		"f") UPDATEMAKEFILES="TRUE";;
		"h") displayusage; exit 0;;
        "m") EMAKE="TRUE";;
		"n") NPROC="${OPTARG}";;
		"o") CMAKEOPTS+=" -DBUILDSINGLE=${OPTARG} "; UPDATEMAKEFILES="TRUE";;
		"r") CONTINUOUSCOMPILE="TRUE";;
		"s") CMAKEOPTS+=" -DBUILDSUITE=ON "; UPDATEMAKEFILES="TRUE";;
		"t") BUILDTYPE=${OPTARG}; UPDATEMAKEFILES="TRUE";;
		"u") CMAKEOPTS+=" -DNOUNITYBUILD=ON "; UPDATEMAKEFILES="TRUE";;
		"w") CMAKEOPTS+="${OPTARG} "; UPDATEMAKEFILES="TRUE";;
		"z") ANALYZE="TRUE" ;;
        "?") displayusage; exit 0;;
    esac
done

shift "$((OPTIND-1))"

CFG=${BUILDTYPE:-Release}
CFG=$(echo "${CFG}" | tr '[:upper:]' '[:lower:]' )
BUILDDIR="${CURRENTDIR}/build/master/${CFG}"
NBFILES="${BUILDDIR}/.nbfiles"
BUILDTYPEFILE="${BUILDDIR}/.buildtype"

if [[ -z "${NPROC}" ]]; then (( NPROC = $(nproc) - 1 )); fi

if [[ "${CLEANBUILD}" == "TRUE" ]] || [[ "${ANALYZE}" == "TRUE" ]] ; then
    read -r -p "Are you sure? [y/N] " CLEANCONFIRM
    case "${CLEANCONFIRM}" in
        [yY][eE][sS]|[yY])
            rm -rf "${ROOTDIR}"/_bin/test "${BUILDDIR}";;
        *)
            exit 0;;
    esac
fi

if [[ "${BUILD}" == "TRUE" ]] ; then build; exit 0; fi

if [[ "${EMAKE}" == "TRUE" ]] ; then emake; exit 0; fi

if [[ "${ANALYZE}" == "TRUE" ]] ; then analyze; exit 0; fi

if [[ "${UPDATEMAKEFILES}" == "TRUE" ]] ; then update_makefiles; fi

if [[ "${DRY_RUN}" == "TRUE" ]] ; then  exit 0; fi

if [[ "${CONTINUOUSCOMPILE}" == "TRUE" ]] ; then
    find "${SOURCEDIR}" -not \( -path "${BUILDDIR}" -prune \) -type f | entr -s "${BASHSCRIPTDIR}/cbuild.sh"
else
    build
fi
