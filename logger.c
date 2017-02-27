//
// Created by Abhishikth Nandam on 2/26/17.
//


#include <time.h>
#include <stdio.h>
#include <stdarg.h>

FILE *logFile;

void log_init(char * LOG_DIR) {
    logFile = fopen(LOG_DIR, "w");
    if (logFile == NULL) {
        fprintf(stdout, "Error opening logfile. \n");
    }
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    fprintf (logFile, "Current local time and date: %s\n", asctime (timeinfo));
}


void close_log() {
    fprintf(logFile, "server being shutdown");
    fclose(logFile);
}
