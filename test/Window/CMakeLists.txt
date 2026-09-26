file(GLOB WINDOW_SRC CONFIGURE_DEPENDS "*.cpp" "*.hpp" "*.inl")
zancle_add_test(test-zancle-window SOURCES ${WINDOW_SRC} DEPENDS Zancle::Window Zancle::GLUtils) # `TestContext` uses `priv::GlContext`
