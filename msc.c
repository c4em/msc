/*
 * msc: Minimal Site Compiler
 * (https://git.dirae.org/msc)
 * 
 * Copyright (C) 2023 caem
 *  
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

/*
 * TODO:
 * - Go through the code and add checks for edge cases, this is currently a house of cards
 * - (?fixed) Fix the issue of the dist file being incorrect on the second run if the previous dist dir wasn't removed
 * - Fix memory leaks
 * - Clean up code
 * - Fix the heap buffer overflow which will eventually become a problem
 */

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
                if (access(optarg, R_OK) == -1)
                    failure("Failed to open specified source directory", 1);
                if (sd == NULL) {
                    sd = strdup(optarg);
                    if (sd == NULL)
                        failure("Failed to allocate memory for source directory", 1);
                }
                if (sd[strlen(sd)] != '/') {
                    sd = realloc(sd, strlen(sd)+2);
                    strcat(sd, "/");
                }
                break;
            case 'd':
                if (access(optarg, F_OK) == -1) 
                    if (mkdir(optarg, S_IRWXU) == -1)
                        failure("Failed to open/create destination directory", 1); 
                if (od == NULL) {
                    od = strdup(optarg);
                    if (od == NULL)
                        failure("Failed to allocate memory for destination directory", 1);
                }
                if (od[strlen(od)] != '/') {
                    od = realloc(od, strlen(od)+2);
                    strcat(od, "/");
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

        sd = realloc(sd, strlen(sd)+2);
        sd = strcat(sd, "/");
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
        strcat(fname, dir->d_name);

        if (dir->d_type == DT_DIR && recurse == 1) {
            if (dir->d_name[0] != '_') {
                if (strlen(path) > 0 && path[strlen(path)] != '/')
                    strcat(fname, "/");
                int r = lsdirf(files, fname, recurse);
                if (r == -1) {
                    free(fname);
                    return -1;
                }
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

    fseek(f, 0L, SEEK_END);
    size_t len = ftell(f);
    rewind(f);

    char *cont = calloc(1, len+1);
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
*nparse(char *str, char *sd)
{
    struct item *it = (struct item*)malloc(sizeof(struct item)); 

    char *sp = strstr(str, "[[[");
    if (sp == NULL)
        return NULL;
    it->s_i = sp - str;

    char *ep = strstr(str, "]]]");
    if (ep == NULL) {
        fprintf(stderr, "Syntax error: missing closing brackets for \"[[[\" at: ");
        pel(str, it->s_i);
        exit(EXIT_FAILURE);
    }
    it->e_i = (ep - str)+3;  

    int s = -1, e = -1;
    for (int i = 3; i < it->e_i; i++) {
        if (s == -1 && str[it->s_i+i] != ' ')
            s = it->s_i+i;
        if (s != -1 && str[it->s_i+i] == ' ') {
            e = it->s_i+i; 
            break;
        }
    }
    if (sd != NULL) {
        it->fname = strdup(sd); 
        char *temp = strndup(str+s, e - s);
        it->fname = realloc(it->fname, strlen(sd)+strlen(temp)+1);
        strcat(it->fname, temp);
        free(temp);
    } else {
        it->fname = strndup(str+s, e - s);
    }

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

    return bb.d;
}

void
cpy(char *src, char *dest)
{
    int s = -1;
    for (int i = strlen(dest); i > 0; i--) {
        if (dest[i] == '/') {
            s = i;
            break;
        }
    }

    if (s != -1) {
        char *ppath = strndup(dest, s);
        mdr(ppath);
        free(ppath);
    }

    char *cont = fcont(src);
    if (cont == NULL) {
        fprintf(stderr, "Failed to read data from file %s for copying", src);
        failure("", 1);
    }

    FILE *dfp = fopen(dest, "w");
    if (dfp == NULL) {
        fprintf(stderr, "Could not write to output file %s", dest);
        failure("", 1);
    }

    fputs(cont, dfp);

    fclose(dfp);
}

void
cpyf(char **files, int fc, char *sd, char *od)
{
    for (int i = 0; i < fc; i++) {
        if (strstr(files[i], ".md") != NULL)
            continue;

        char *ofp = strdup(od);
        ofp = realloc(ofp, strlen(ofp)+strlen(files[i]+strlen(sd))+1);
        strcat(ofp, files[i]+strlen(sd));

        cpy(files[i], ofp);
        free(ofp);
    }
}

char
*rmdf(char *fp)
{
    fp = strndup(fp, strlen(fp)-2);
    fp = realloc(fp, strlen(fp)+5);
    strcat(fp, "html");

    return fp;
}

void
wrd(char *dfp, char *dfc)
{
    FILE *dfpp = fopen(dfp, "w");
    if (dfpp == NULL) {
        fprintf(stderr, "Could not write to output file %s", dfp);
        failure("", 1);
    }

    fputs(dfc, dfpp);
    fclose(dfpp);
}

char
*ged(char *fname, int type)
{
    if (type == 1) 
        return md2html(fcont(fname));
    else {
        char *cont = fcont(fname);
        return cont;
    }
}

char
*gmed(char **files, int fc)
{
    char *co = NULL;
    for (int i = 0; i < fc; i++) {
        int type = 0;
        if (strstr(files[i], ".md") != NULL)
            type = 1;
        char *pc = ged(files[i], type);
        if (co == NULL) {
            co = strdup(pc);
            free(pc);
            continue;
        }
        co = realloc(co, strlen(co)+strlen(pc)+1);
        co = strcat(co, pc);
        free(pc);
    }
    return co;
}

void
mdr(char *path)
{
    int s = -1, e = -1;
    int plen = strlen(path);
    for (int i = 0; i < plen; i++) {
        if (s == -1 && path[i] == '/') {
            s = i;
            continue;
        }
        if (s != -1 && path[i] == '/') {
            e = i;
        }
        if (i+1 == plen) {
            e = i+1;
        }

        if (s != -1 && e != -1) {
            char *fpath = strndup(path, e);
            if (access(fpath, F_OK) != 0) {
                int st = mkdir(fpath, S_IRWXU);
                free(fpath);
                if (st != 0 && st != EACCES) {
                    char *edir = strndup(path, e);
                    fprintf(stderr, "Could not create directory: %s", edir);
                    free(edir);
                    failure("", 1);
                }

                s = e;
                e = -1;
            }
        }
    }
}

void
pff(char **ffiles, int ffc, char *sd, char *od)
{
    for (int i = 0; i < ffc; i++) {
        char *ofp = strdup(od);
        char *orgfp = NULL;
        if (strstr(ffiles[i], ".md") != NULL) {
            orgfp = strdup(ffiles[i]);
            ffiles[i] = rmdf(ffiles[i]);
        }
        ofp = realloc(ofp, strlen(ofp)+strlen(ffiles[i]+strlen(sd))+1);
        strcat(ofp, ffiles[i]+strlen(sd));

        char *ffcont = NULL;
        if (orgfp != NULL) {
            ffcont = md2html(fcont(orgfp));
        } else {
            ffcont = fcont(ffiles[i]);
        }

        if (ffcont == NULL) {
            fprintf(stderr, "Failed to read from filtered file %s", ffiles[i]);
            failure("", 1);
        }

        struct item *it = nparse(ffcont, sd);
        while (it != NULL) {
            char *ofc = strndup(ffcont, it->s_i);
            char *m = NULL;
            if (strstr(it->fname, "/*") != NULL) {
                char *path = strndup(it->fname, strlen(it->fname)-1);
                char **ef = NULL;
                int ec = lsdirf(&ef, path, 1);
                m = gmed(ef, ec);
            } else {
                m = ged(it->fname, it->type);
            }

            ofc = realloc(ofc, strlen(ofc)+strlen(m)+strlen(ffcont+it->e_i)+1);
            ofc = strcat(ofc, m);
            ofc = strcat(ofc, ffcont+it->e_i);

            wrd(ofp, ofc); 
            
            free(m);

            free(it);
            it = nparse(ofc, sd);
            free(ffcont);
            ffcont = strdup(ofc);
            free(ofc);
        }
        free(ofp);
    }
}

void
init(char *sd, char *od)
{
    char **files = NULL;
    int fc = lsdirf(&files, sd, 1);

    char **ffiles = NULL;
    char *ex[] = { ".html", ".htm", ".md" };
    int ffc = fext(&ffiles, files, fc, ex, 3);

    cpyf(files, fc, sd, od); 

    pff(ffiles, ffc, sd, od);

    free(files);
    free(ffiles);
}

