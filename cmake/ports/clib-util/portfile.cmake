# header-only library
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CLibUtil
    REF cbd51eb82670429bb4ca1375b8418e998c0ba189
    SHA512 d93d9c27f19d3a7d7c6f42e15ecf87bc61019e12c3c5b181d2518b9cdeed1bd124f9723a20aa03bc2104ab5c0bd9c2293ce21933cb1aaf04d4214bb202f433c2
    HEAD_REF master
)

# Install codes
set(CLIBUTIL_SOURCE	${SOURCE_PATH}/include/ClibUtil)
file(INSTALL ${CLIBUTIL_SOURCE} DESTINATION ${CURRENT_PACKAGES_DIR}/include)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
