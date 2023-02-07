#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <md4c-html.h>

#include "msc.h"

int
main(int argc, char **argv)
{
    int c;
    char *sd = NULL, *od = NULL;
    while ((c = getopt_long(argc, argv, "hs:d:", long_options, NULL)) != -1) {
        switch(c) {
            case 'h':
                break;
            case 's':
                printf("s optarg: %s\n", optarg);
                if (access(optarg, R_OK) == -1)
                    failure("Failed to open specified source directory", 1);
                if (sd == NULL) {
                    sd = strdup(optarg);
                    if (sd == NULL)
                        failure("Failed to allocate memory for source directory", 1);
                }
                break;
            case 'd':
                printf("d optarg: %s\n", optarg);
                if (access(optarg, F_OK) == -1) 
                    if (mkdir(optarg, S_IRWXU) == -1)
                        failure("Failed to open/create destination directory", 1); 
                if (od == NULL) {
                    od = strdup(optarg);
                    if (od == NULL)
                        failure("Failed to allocate memory for destination directory", 1);
                }
                break;
        }
    }

    if (sd == NULL) {
        sd = getcwd(sd, 256);
        if (sd == NULL)
            failure("Failed to get current working directory", 1); 
        if (access(sd, R_OK) == -1)
            failure("Failed to get current working directory", 1); 
    }

    if (od == NULL) {
        int newlen = strlen(sd) + 7;
        od = malloc(newlen);
        if (!od)
            failure("Failed to allocate memory for the destination directory", 1);
        strcpy(od, sd);
        if (sd[strlen(sd)] == '/' || sd[strlen(sd) - 1] == '/')
            strcat(od, "dist/");
        else
            strcat(od, "/dist/");

        if (access(od, F_OK) == -1) 
            if (mkdir(od, S_IRWXU) == -1)
                failure("Failed to open/create destination directory", 1); 
    }

    char **files = NULL;
    int fc = lsdirf(&files, sd, 1);
    if (fc != -1) {
        for (int i = 0; i < fc; i++) {
            // printf("%s\n", files[i]);
        }
    }

    char **ffiles = NULL;
    char **ex = malloc(2);
    ex[0] = strdup(".c");
    ex[1] = strdup(".h");
    int ffc = fext(&ffiles, files, fc, ex, 2);
    if (ffc != -1) {
        printf("Filtered:\n");
        for (int i = 0; i < ffc; i++) {
            // printf("%s", fcont(ffiles[i]));
        }
    }

    return EXIT_SUCCESS;
}

static void
failure(const char *reason, int fatal)
{
    if (!fatal)
        fatal = 0;

    fprintf(stderr, "%s: %s (error %d)\n", reason, strerror(errno), errno);
    if (fatal == 1)
        exit(errno);
}

int
lsdirf(char ***files, char *path, int recurse)
{
    DIR *d = opendir(path);
    if (d == NULL) {
        fprintf(stderr, "Failed to open directory \"%s\"", path);
        failure("", 0);
        return -1;
    }

    int c = 0;
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        char *fname = malloc(strlen(path)+strlen(dir->d_name)+2);
        if (fname == NULL)
            failure("Memory allocation failed in lsdirf()", 1);
        strcpy(fname, path);
        if (strlen(path) > 0 && path[strlen(path)] != '/')
            strcat(fname, "/");
        strcat(fname, dir->d_name);

        if (dir->d_type == DT_DIR && recurse == 1) {
            int r = lsdirf(files, fname, recurse);
            if (r == -1) {
                free(fname);
                return -1;
            }
            continue;
        }

        *files = realloc(*files, sizeof(**files) * (c + 1));
        if (*files == NULL) {
            free(fname);
            failure("Memory reallocation failed in lsdirf()", 1);
        }

        (*files)[c] = strdup(fname);
        if (!(*files)[c]) {
            free(fname);
            failure("strdup() failed in lsdirf()", 1);
        }

        c++;

        free(fname);
    }
    closedir(d);

    return c;
}

int
fext(char ***ffiles, char **files, int fc, char **ex, int exc)
{
    int c = 0;
    for (int i = 0; i < fc; i++) {
        for (int j = 0; j < exc; j++) {
            if (strlen(files[i]) > strlen(ex[j])) {
                if (strcmp(files[i] + strlen(files[i])-strlen(ex[j]), ex[j]) == 0) {
                    *ffiles = realloc(*ffiles, sizeof(**ffiles) * (c + 1));
                    if (*ffiles == NULL) 
                        failure("Memory reallocation failed in fext()", 1);
                    (*ffiles)[c] = strdup(files[i]);
                    if (!(*files)[c]) 
                        failure("strdup() failed in fext()", 1);
                    c++;
                    break;
                }
            }
        }
    }
    return c;
}

char
*fcont(char *path)
{
    FILE *f = fopen(path, "rb"); 
    if (f == NULL) {
        fprintf(stderr, "Failed to open file \"%s\"", path);
        failure("", 0);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *cont = malloc(len);
    if (cont == NULL) {
        fclose(f);
        failure("Failed to allocate memory in fcont()", 0);
        return NULL;
    }
    
    size_t rlen = fread(cont, 1, len, f);
    if (rlen != len) {
        fprintf(stderr, "Warning: Could not read all characters from file: %s\n", path);
    }

    fclose(f);

    return cont;
}

static void
pel(char *str, int i)
{
    int l, li = 0;
    for (int j = 0; j < i; j++) {
        if (str[j] == '\n') {
            l++;
            li = j + 1;
        }
    }
    fprintf(stderr, "%d:%d", li, i-l-li);
}

struct item
*nparse(char *str)
{
    struct item *i; 

    char *sp = strstr(str, "[[[");
    if (sp == NULL)
        return NULL;
    int s = sp - str;
    i->s_i = s;

    char *ep = strstr(str, "]]]");
    if (ep == NULL) {
        fprintf(stderr, "Syntax error missing closing brackets for \"[[[\" at: ");
        pel(str, i->s_i);
        exit(EXIT_FAILURE);
    }
    int e = ep - str;  
    i->e_i = e;

    for (int j = s+3; j < e; j++) {
        if (s != i->s_i && str[j] != ' ') {
            s = j;
            continue;
        }
        if (s == i->s_i && i->e_i != e && str[j] != ' ') {
            e = j;
            break;
        }
    }

    i->fname = malloc(ep-sp+1);
    memcpy(i->fname, sp, e - s);
    i->fname[ep-sp] = '\0';

    free(ep);
    free(sp);

    return i;
}

static void
po(const MD_CHAR *dat, MD_SIZE dat_s, void *ud)
{
    struct bbuf *buf = ud;
    if (buf->as < buf->s + dat_s) {
        size_t ns = buf->s + buf->s / 2 + dat_s;
        buf->d = realloc(buf->d, ns);
        buf->as = ns;
    }
    memcpy(buf->d + buf->s, dat, dat_s);
    buf->s += dat_s;
}

char
*md2html(char *str)
{
    int flags = MD_FLAG_UNDERLINE | MD_FLAG_LATEXMATHSPANS | 
        MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS |
        MD_FLAG_PERMISSIVEATXHEADERS | MD_FLAG_PERMISSIVEEMAILAUTOLINKS; 

    struct bbuf bb = { 0 };
    bb.s = 0;
    bb.as = (strlen(str)*2)/8+64;
    bb.d = malloc(bb.as);

    int r = md_html(str, strlen(str), po, &bb, flags, 0);
    if (r == -1) {
        return NULL;
    }
    printf("%s", bb.d);

    return bb.d;
}

