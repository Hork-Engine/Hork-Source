#INCLUDE_DIRECTORIES(
# ${BULLET_PHYSICS_SOURCE_DIR}/Extras/HACD
#)

SET(HACD_SRCS
	hacdGraph.cpp
	hacdHACD.cpp
	hacdICHull.cpp
	hacdManifoldMesh.cpp
)

SET(HACD_HDRS
	hacdCircularList.h
	hacdGraph.h
	hacdHACD.h
	hacdICHull.h
	hacdManifoldMesh.h
	hacdVector.h
	hacdVersion.h
	hacdCircularList.inl
	hacdVector.inl
)

add_library(HACD ${HACD_SRCS} ${HACD_HDRS})
