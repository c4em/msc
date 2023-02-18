#pragma once

#define MD4C_USE_UTF8

static struct option long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "src", required_argument, NULL, 's' },
    { "dst", required_argument, NULL, 'd' },
    { 0, 0, 0, 0 },
};

struct item {
    char *fname;
    int s_i;
    int e_i;
    int type; // 0 = other; 1 = md;
};

struct bbuf {
    char *d;
    size_t as;
    size_t s;
};

int main(int argc, char **argv);
static void failure(const char *reason, int fatal);
int lsdirf(char ***files, char *path, int recurse);
int fext(char ***ffiles, char **files, int fc, char **ex, int exc);
char *fcont(char *path);
static void pel(char *str, int i);
struct item *nparse(char *str, char *sd);
static void po(const MD_CHAR *dat, MD_SIZE dat_s, void *ud);
char *md2html(char *str);
void cpy(char *src, char *dest);
void cpyf(char **filess, int fc, char *sd, char *od);
char *rmdf(char *fp);
void wrd(char *dfp, char *dfc);
char *ged(char *fname, int type);
char *gmed(char **files, int fc);
void pff(char **ffiles, int ffc, char *sd, char *od);
void init(char *sd, char *od);

