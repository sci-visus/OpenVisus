
add_subdirectory(Kernel)
add_subdirectory(Dataflow)
add_subdirectory(Db)
add_subdirectory(Idx)
add_subdirectory(Nodes)

if (VISUS_GUI)
  add_subdirectory(Gui)
  add_subdirectory(GuiNodes)
  add_subdirectory(AppKit)
endif()

add_subdirectory(xidx)
