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

    init(sd, od);

    return EXIT_SUCCESS;
}

void
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

void
pel(char *str, int i)
{
    int l = 0, li = 0;
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
    struct item *it = (struct item*)malloc(sizeof(struct item)); 

    char *sp = strstr(str, "[[[");
    if (sp == NULL)
        return NULL;
    int s = sp - str;

    char *ep = strstr(str, "]]]");
    if (ep == NULL) {
        fprintf(stderr, "Syntax error missing closing brackets for \"[[[\" at: ");
        pel(str, s);
        exit(EXIT_FAILURE);
    }
    int e = ep - str;  

    it->s_i = -1;
    it->e_i = -1;
    for (int i = s+3; i < e; i++) {
        if (it->s_i == -1 && str[s+i] != ' ')
            it->s_i = s+i;
        if (it->s_i != -1 && str[s+i] == ' ') {
            it->e_i = s+i-1; 
            break;
        }
    }

    it->fname = strndup(str+it->s_i, it->e_i - it->s_i);

    it->type = 0;
    if (strstr(it->fname, ".md") != NULL)
        it->type = 1;

    return it;
}

void
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

void
init(char *sd, char *od)
{
    char **files = NULL;
    int fc = lsdirf(&files, sd, 1);

    char **ffiles = NULL;
    char *ex[] = { ".html", ".htm", ".md" };
    int ffc = fext(&ffiles, files, fc, ex, 3);

    for (int i = 0; i < ffc; i++) {
        struct item *it = nparse(fcont(ffiles[i]));
        if (it == NULL)
            continue;
        if (it->type == 1)
            printf("Markdown file: %s\n", it->fname);
        else
            printf("Other file: %s\n", it->fname);
    }

    free(files);
    free(ffiles);
}

