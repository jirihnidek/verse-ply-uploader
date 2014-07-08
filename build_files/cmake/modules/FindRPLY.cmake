# This module tries to find RPLY library and include files
#
# RPLY_INCLUDE_DIR, where to find verse.h
# RPLY_LIBRARY_DIR, where to find libverse.so
# RPLY_LIBRARIES, the library to link against
# RPLY_FOUND, IF false, do not try to use Verse
#

FIND_PATH ( RPLY_INCLUDE_DIR rply.h
    /usr/include
    /usr/local/include
    /opt/local/include
    /sw/include
)

FIND_LIBRARY ( RPLY_LIBRARIES rply
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
)

GET_FILENAME_COMPONENT( RPLY_LIBRARY_DIR ${RPLY_LIBRARIES} PATH )

SET ( RPLY_FOUND "NO" )
IF ( RPLY_INCLUDE_DIR )
    IF ( RPLY_LIBRARIES )
        SET ( RPLY_FOUND "YES" )
    ENDIF ( RPLY_LIBRARIES )
ENDIF ( RPLY_INCLUDE_DIR )

MARK_AS_ADVANCED (
    RPLY_LIBRARY_DIR
    RPLY_INCLUDE_DIR
    RPLY_LIBRARIES
)
