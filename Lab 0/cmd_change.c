/* Check # arguments supplied for --command */
while(!isOption(&argv[(optind-1)+argcc]) && ((optind+argcc++) < argc))
  {
    //if((optind+argcc++) >= argc)                                                                                   
    //  break;                                                                                                       
    argvc[argcc-1] = &argv[(optind-1)+argcc-1];
  }
