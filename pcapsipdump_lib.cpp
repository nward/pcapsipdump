#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "pcapsipdump_lib.h"

// equivalent of "mkdir -p"
int mkdir_p(const char *path, mode_t mode) {
    char s[256];
    char *p;
    struct stat sb;

    if (stat(path, &sb) == 0) {
        return 0;
    } else {
        strlcpy(s, path, sizeof(s));
        p = strrchr(s, '/');
        if (p != NULL) {
            *p = '\0';
            if (mkdir_p(s, mode) != 0) {
                return -1;
            }
        }
        return mkdir(path, mode);
    }
    return -1;
}


size_t expand_dir_template(char *s, size_t max, const char *format,
                           const char *from,
                           const char *to,
                           const char *callid,
                           const time_t t) {
    struct tm *tm = localtime(&t);
    size_t fl = strlen(format);
    size_t s1l = fl + 256;
    char *s1 = (char *)malloc(s1l);
    char *s1p = s1;
    char asciisan[128] = {
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '_', '_', '_', '#', '_', '_', '&', '_', '(', ')', '_', '+', ',', '-', '.', '_',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', ';', '_', '=', '_', '_',
        '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '_', ']', '^', '_',
        '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '_', '}', '~', '_'
    };
    for (size_t i = 0; i <= fl; i++) {
        char c0 = format[i];
        if (c0 == '%' && i < fl - 1) {
            char c1 = format[i+1];
            if (c1 == 'f' && (s1l - (s1p - s1)) > strlen(from) ){
                strcpy(s1p, from);
                for(;*s1p;s1p++){
                    *s1p = asciisan[*s1p & 0x7f];
                }
                i++;
            } else if (c1 == 't' && (s1l - (s1p - s1)) > strlen(to) ){
                strcpy(s1p, to);
                for(;*s1p;s1p++){
                    *s1p = asciisan[*s1p & 0x7f];
                }
                i++;
            } else if (c1 == 'i' && (s1l - (s1p - s1)) > strlen(callid) ){
                strcpy(s1p, callid);
                for(;*s1p;s1p++){
                    *s1p = asciisan[*s1p & 0x7f];
                }
                i++;
            } else {
                *(s1p++) = c0;
            }
        } else {
            *(s1p++) = c0;
        }
    }
    {
        size_t r = strftime(s, max, s1, tm);
        free(s1);
        return r;
    }
}


int opts_sanity_check_d(char **opt_fntemplate)
{
    char s[2048];
    char *orig_opt_fntemplate = *opt_fntemplate;
    struct stat sb;
    FILE *f;

    expand_dir_template(s, sizeof(s), *opt_fntemplate, "", "", "", (int)time(NULL));
    if (stat(s, &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "Bad option '-d %s': File exists (expecting directory name or filename template)", orig_opt_fntemplate);
            return(2);
        }
        // Looks like user has specified bare directory in '-d' option.
        // First, make sure we can create files in that directory
        strcat(s, "/tmp-siJNlSdiugQ3iyjaPNW");
        if ((f = fopen(s, "a")) == NULL){
            fprintf(stderr, "Can't create file for '-d %s': ", orig_opt_fntemplate);
            perror (s);
            return(2);
        }
        fclose(f);
        unlink(s);
        // Then append some default filename template to be backwards-compatible
        *opt_fntemplate = (char *)malloc(strlen(s) + 128);
        strcpy(*opt_fntemplate, orig_opt_fntemplate);
        strcat(*opt_fntemplate, "/%Y%m%d/%H/%Y%m%d-%H%M%S-%f-%t-%i.pcap");
        expand_dir_template(s, sizeof(s), orig_opt_fntemplate, "", "", "", 0);
    } else {
        if (!strchr(*opt_fntemplate, '%') || !strchr(s, '/')) {
            fprintf(stderr, "Bad option '-d %s': Neither directory nor filename template", orig_opt_fntemplate);
            return(2);
        // (try to) create directory hierarchy
        } else if (mkdir_p(dirname(s), 0777)) {
            fprintf(stderr, "Can't create directory for '-d %s': ", orig_opt_fntemplate);
            perror (s);
            return(2);
        }
    }
    return(0);
}
