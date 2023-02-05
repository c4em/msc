#pragma once

#define MD4C_USE_UTF8

int main(int argc, char **argv);
static void failure(const char *reason, int fatal);
int lsdirf(char ***files, char *path, int recurse);
int fext(char ***ffiles, char **files, int fc, char **ex, int exc);
char *fcont(char *path);

static struct option long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "src", required_argument, NULL, 's' },
    { "dst", required_argument, NULL, 'd' },
    { 0, 0, 0, 0 },
};
