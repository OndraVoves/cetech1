
#include <stdio.h>

#include <cetech/core/log.h>

int exec(const char *argv) {
#if defined(CETECH_LINUX)
    char output[4096];

    log_api_v0.debug("os", "exec %s", argv);

    FILE *fp = popen(argv, "r");
    if (fp == NULL)
        return 0;

    while (fgets(output, CETECH_ARRAY_LEN(output), fp) != NULL) {
        printf("%s", output);
    }

    int status = fclose(fp);

    if (status != 0) {
        log_api_v0.error("os", "output %s", output);
    }

    return status;
#endif
}