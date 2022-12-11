#include <stdio.h>
#include <syslog.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    FILE *f;

    openlog(NULL, 0, LOG_USER);
    syslog(LOG_DEBUG, "Writing %s to %s\n", argv[2], argv[1]);
    if (argc != 3)
    {
        syslog(LOG_ERR, "ERROR: Invalid number of arguments provided!\n");
        return 1;
    }
    f = fopen(argv[1], "w");
    if (f==NULL)
    {
        syslog(LOG_ERR, "Error opening the file %s: %d\n", argv[1], errno);
        return 1;
    }

    fprintf(f, argv[2]);
    return 0;
}