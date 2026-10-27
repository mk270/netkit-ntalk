
include_directories(BEFORE "../include/")

add_executable(
    in.ntalkd
    talkd.c
    announce.c
    process.c
    table.c
    print.c
    repairs.c
)
target_link_libraries(
    in.ntalkd
    wrap
)
install(
    TARGETS in.ntalkd
    DESTINATION ${SBIN_DIR}
)
install(
    CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        in.ntalkd \$ENV{DESTDIR}${SBIN_DIR}/in.talkd \
    )"
)

install(
    FILES talkd.8
    DESTINATION ${MAN_DIR}/man8/
    RENAME in.ntalkd.8
)
install(
    CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        in.ntalkd.8 \$ENV{DESTDIR}${MAN_DIR}/man8/in.talkd.8 \
    )"
)
install(
    CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        in.ntalkd.8 \$ENV{DESTDIR}${MAN_DIR}/man8/ntalkd.8 \
    )"
)
install(
    CODE "execute_process( \
        COMMAND ${CMAKE_COMMAND} -E create_symlink \
        in.ntalkd.8 \$ENV{DESTDIR}${MAN_DIR}/man8/talkd.8 \
    )"
)
