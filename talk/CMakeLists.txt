
include_directories(BEFORE "../include/")

add_executable(
    netkit-ntalk
    ctl.c
    ctl_transact.c
    display.c
    get_addrs.c
    get_names.c
    init_disp.c
    invite.c
    io.c
    look_up.c
    msgs.c
    talk.c
)
target_link_libraries(
    netkit-ntalk
    ncurses
)
install(
    TARGETS netkit-ntalk
    DESTINATION ${BIN_DIR}
)

install(
    FILES talk.1
    DESTINATION ${MAN_DIR}/man1/
    RENAME netkit-ntalk.1
)
