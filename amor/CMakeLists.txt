qt1_wrap_moc(MOC_FILES
    amorbubble.h
    amordialog.h
    amor.h
    amorwidget.h
)

add_executable(amor
    amor.cpp amorwidget.cpp main.cpp
    amoranim.cpp amorpm.cpp amordialog.cpp
    amorbubble.cpp amorconfig.cpp amortips.cpp

    ${MOC_FILES}
)
target_link_libraries(amor
    ${KDE1_KDECORE}
    ${KDE1_KDEUI}
    Qt::Qt1
    X11::X11
)

install(TARGETS amor RUNTIME DESTINATION ${KDE1_BINDIR})

install_icon(amor.xpm)
install(FILES amor.kdelnk DESTINATION ${KDE1_APPSDIR}/Games/)

add_subdirectory(data)
