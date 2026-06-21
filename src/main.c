#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 256

static int read_first_line(const char *path, char *buf, size_t size)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        snprintf(buf, size, "N/A");
        return -1;
    }

    if (fgets(buf, size, fp) == NULL) {
        snprintf(buf, size, "N/A");
        fclose(fp);
        return -1;
    }

    buf[strcspn(buf, "\r\n")] = '\0';

    fclose(fp);
    return 0;
}

static void print_hostname(void)
{
    char buf[BUF_SIZE];

    read_first_line("/etc/hostname", buf, sizeof(buf));

    printf("hostname : %s\n", buf);
}

static void print_cpu_temp(void)
{
    char buf[BUF_SIZE];

    if (read_first_line("/sys/class/thermal/thermal_zone0/temp", buf, sizeof(buf)) != 0) {
        printf("cpu_temp : N/A\n");
        return;
    }

    int temp_milli = atoi(buf);

    printf("cpu_temp : %.1f C\n", temp_milli / 1000.0);
}

int main(void)
{
    printf("rkmon v0.1\n");
    printf("====================\n");

    print_hostname();
    print_cpu_temp();

    return 0;
}