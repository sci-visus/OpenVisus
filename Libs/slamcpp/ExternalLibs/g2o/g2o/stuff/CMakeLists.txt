AddExternalLibrary(g2o_stuff 
  filesys_tools.h    string_tools.h
  color_macros.h     macros.h         timeutil.cpp
  command_args.cpp   misc.h           sparse_helper.cpp  timeutil.h
  command_args.h     os_specific.c    sparse_helper.h
  filesys_tools.cpp  os_specific.h    string_tools.cpp
  property.cpp       property.h       
  sampler.cpp        sampler.h        unscented.h
  tictoc.cpp tictoc.h
  g2o_stuff_api.h)
  

set_target_properties(g2o_stuff PROPERTIES FOLDER "ExternalLibs/g2o")