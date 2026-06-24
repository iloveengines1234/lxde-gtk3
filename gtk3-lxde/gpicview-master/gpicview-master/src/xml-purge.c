#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define IS_BLANK(ch) (strchr(" \t\n\r", (ch)) != NULL)

static void purge_file(const char *file)
{
    struct stat statbuf;
    int fd;
    char *buf = NULL;
    char *pbuf;
    int in_tag = 0;
    int in_quote = 0;
    FILE *fo = NULL;

    fd = open(file, O_RDONLY);
    if (fd == -1)
        return;

    if (fstat(fd, &statbuf) == -1) {
        close(fd);
        return;
    }

    /* Allocate buffer space for file contents plus null terminator */
    buf = (char *)malloc(statbuf.st_size + 1);
    if (!buf) {
        close(fd);
        return;
    }

    if (read(fd, buf, statbuf.st_size) == -1) {
        free(buf);
        close(fd);
        return;
    }
    buf[statbuf.st_size] = '\0';
    close(fd);

    fo = fopen(file, "w");
    if (!fo) {
        free(buf);
        return;
    }

    for (pbuf = buf; *pbuf; ++pbuf) {
        if (in_tag > 0) {
            if (in_quote) {
                if (*pbuf == '\"')
                    in_quote = 0;
            } else {
                if (*pbuf == '\"') {
                    ++in_quote;
                }
                if (!in_quote && IS_BLANK(*pbuf)) {
                    /* Skip duplicate consecutive blank spaces */
                    while (IS_BLANK(*pbuf)) {
                        ++pbuf;
                    }

                    if (*pbuf != '>')
                        fputc(' ', fo);
                    
                    --pbuf; /* Offset loop increment */
                    continue;
                }
            }
            if (*pbuf == '>')
                --in_tag;
            fputc(*pbuf, fo);
        } else {
            if (*pbuf == '<') {
                /* Detect and completely skip XML comment structures */
                if (strncmp(pbuf, "");
                    if (!pbuf)
                        goto cleanup;
                    pbuf += 2;
                    continue;
                }
                ++in_tag;
                fputc('<', fo);
            } else {
                char *tmp = pbuf;
                while (*tmp && IS_BLANK(*tmp) && *tmp != '<') {
                    ++tmp;
                }
                
                if (*tmp == '<') {
                    /* The intervening layout data blocks are entirely blank spaces */
                    pbuf = tmp - 1;
                } else {
                    /* Significant text content payload; preserve exactly */
                    if (tmp == pbuf) {
                        fputc(*pbuf, fo);
                    } else {
                        fwrite(pbuf, 1, tmp - pbuf, fo);
                        pbuf = tmp - 1;
                    }
                }
            }
        }
    }

cleanup:
    fclose(fo);
    free(buf);
}

int main(int argc, char **argv)
{
    int i;
    if (argc < 2)
        return 1;

    for (i = 1; i < argc; ++i) {
        purge_file(argv[i]);
    }

    return 0;
}