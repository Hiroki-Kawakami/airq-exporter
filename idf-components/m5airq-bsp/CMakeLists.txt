file(GLOB_RECURSE BSP_SRCS "*.c")
idf_component_register(
    SRCS ${BSP_SRCS}
    INCLUDE_DIRS "."
    PRIV_REQUIRES driver)
