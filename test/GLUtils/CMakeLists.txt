file(GLOB GLUTILS_SRC CONFIGURE_DEPENDS "*.cpp" "*.hpp" "*.inl")
zancle_add_test(test-zancle-glutils SOURCES ${GLUTILS_SRC} DEPENDS Zancle::GLUtils)
