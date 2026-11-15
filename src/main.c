#include <error.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "config.h"

// variables
char* os;
char* kernelVersion;
char* shell;

//method for executing a command and get the output
char* exec(const char* cmd) {
    char buffer[16] = {0};
    size_t resultLen = 0;
    size_t resultSize = 16;
    char* result = malloc(resultSize);
    memset(result, 0, 16);

    FILE* pipe = popen(cmd, "r");

    while(!feof(pipe))
        if (fgets(buffer, 16, pipe) != NULL) {
            resultLen += strlen(buffer);
            if (resultLen > resultSize) {
                resultSize *= 2;
                result = realloc(result, resultSize);
            }
            strcat(result, buffer);
        }

    return result;
}

unsigned int countOccur(FILE* haystack, const char* needle){
    char buffer[1024];
    unsigned int occurences = 0;
    while(!feof(haystack))
        if (fgets(buffer, 1024, haystack) != NULL) {
            unsigned int done = 0;
            char* new;
            while((new = strstr(buffer + done, needle))) {
                done = (new - buffer) + 1;
                occurences++;
            }
        }
    return occurences;
}

char* get_cpu(FILE* cpuid){
    //error(1, 0, "displayCpu is not impemented");
    char* line = NULL;
    size_t num = 0;
    for (;;) {
        getline(&line, &num, cpuid);
        if (line == NULL) perror("getline");
        if (*line == 'm' && strstr(line, "model name")) break;
        free(line);
    }

    char* name = strchr(line, ':');
    name++;

    name[strlen(name) - 1] = 0;

    char* name_dup = strdup(name);

    free(line);

    rewind(cpuid);

    unsigned int cpus = countOccur(cpuid, "model name");

    // '(' cpus ')' '\0'
    char cpus_str[5];
    sprintf(cpus_str, "(%u)", cpus);

    // model_name ' ' '(' cpus ')' \0
    char* ret = malloc(strlen(name_dup) + 1 + strlen(cpus_str) + 1 + 1);
    strcpy(ret, name_dup);
    strcat(ret, " ");
    strcat(ret, cpus_str);
    return ret;
}

char* get_value(char* line){
    char* after = strchr(line, '=');

    after[strlen(after) - 1] = 0;

    if (*(++after) == '"') {
        after++;
        after[strlen(after) - 1] = 0;
    }

    return after;
}

char* UNKNOWN = "Unknown";

void get_distro(FILE* osrel){
    char* line = NULL ;
    char
        * id = NULL,
        * name = NULL,
        * pname = NULL;

    size_t num = 0    ;
    while (getline(&line, &num, osrel) > 0) {
        if (line == NULL) perror("getline");
        if (*line == 'I' && strstr(line, "ID="))
            id = strdup(get_value(line));
        if (*line == 'N' && strstr(line, "NAME="))
            name = strdup(get_value(line));
        if (*line == 'P' && strstr(line, "PRETTY_NAME="))
            pname = strdup(get_value(line));
        line = NULL;
    }

    if (pname){
        os = strdup(pname);
        free(name);
        free(id);
    } else if (name) {
        os = strdup(name);
        free(id);
    } else if (id) {
        os = strdup(id);
    } else {
        os = UNKNOWN;
    }
}

void checkOS(void) {
    struct utsname kernInfo = {};

    // uname shouldnt fail, but handle the fail anyways
    if (uname(&kernInfo) < 0)
        perror("Uname");

    kernelVersion = strdup(kernInfo.release);

    // Handle All Non-linux Unix Releases
    if (strcmp("Linux", kernInfo.sysname) != 0) {
        os = strdup(kernInfo.sysname);
        return;
    }

    FILE* osrel = fopen("/etc/os-release", "r");

    if (osrel == NULL)
        perror("fopen(\"/etc/os-release\", \"r\")");

    get_distro(osrel);
}

// gets all packages for all supported operating systems
char* get_packages(void) {
    size_t packLen = 0;
    size_t packSize = 32;
    char* packages = malloc(packSize);

#define GET(bin, cmd, name)\
    if (access("/usr/" bin, 0) == 0) {\
        char* tmp = exec("echo -n $(" cmd ")");\
        packLen += strlen(tmp) + strlen(" (" name ") ");\
        if (packLen > packSize) {\
            while(packLen > packSize) packSize *= 2;\
            packages =  realloc(packages, packSize);\
        }\
        strcat(packages, tmp);\
        strcat(packages, " (" name ") ");\
    }

    GET("bin/emerge"       , "ls -d /var/db/pkg/*/* | wc -l"  , "emerge" );
    GET("bin/pacman"       , "pacman -Qq | wc -l"             , "pacman" );
    GET("bin/xbps-install" , "xbps-query -l | wc -l"          , "xbps"   );
    GET("sbin/pkg_info"    , "pkg_info | wc -l"               , "pkg"    );
    GET("bin/dpkg-query"   , "dpkg-query -f '.\n' -W | wc -l" , "dpkg"   );

    // fallback for if no package manager could be found, lists all programs
    // installed under /usr/bin
    if (*packages == 0)
        GET("bin", "ls /usr/bin | wc -l", "/bin");

    return packages;
}

// method to handle the printing of the information
void print(const char* str1, const char* str2) {
    if (str1 && str1[0])
        printf("\x1b[%dm %s ~ ", primaryColor, str1);

    printf("\x1b[%dm%s\x1b[%dm\n", secondaryColor, str2, resetColor);
}

char* get_user(void) {
    struct passwd* pass = getpwuid(getuid());

    if (pass == NULL)
        // getuid never fails
        perror("getpwuid");

    if (!shell)
        shell = pass->pw_shell;

    // NOTE: statically allocated, DO NOT `free(3)`
    return pass->pw_name;
}

char* get_host(void) {
    char* hostname = malloc(1024);
    memset(hostname, 0, 1024);

    // 1023 to account for the lack of a terminating NULL byte
    if (gethostname(hostname, 1023) < 0)
        perror("gethostname");

    return hostname;
}

char* get_shell(void) {
    if (shell)
        return shell;

    struct passwd* pass = getpwuid(getuid());
    if (pass == NULL)
        // getuid never fails
        perror("getpwuid");

    shell = pass->pw_shell;
    return shell;
}

char* get_w_server(void){
    char* DISPLAY  = getenv("DISPLAY");
    char* WDISPLAY = getenv("WAYLAND_DISLAY");
    bool displayIsWayland = (DISPLAY && (strcmp(DISPLAY, "wayland") == 0));
    bool X11 = ( DISPLAY != NULL && !displayIsWayland);
    bool way = (WDISPLAY != NULL ||  displayIsWayland);
    char all = (X11 + (way << 1));

    char* ret = "Unknown";
    switch (all) {
        case 1: ret = "X11"           ; break ;
        case 2: ret = "Wayland"       ; break ;
        case 3: ret = "X11 | Wayland" ; break ;
    }
    return strdup(ret);
}

// Adapted from https://gist.github.com/dgoguerra/7194777
char* human_size(uint64_t bytes){
	char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
	char length = sizeof(suffix) / sizeof(suffix[0]);

	int i = 0;
	double dblBytes = bytes;

	if (bytes >= 1024) {
		for (i = 0; (bytes / 1024) > 0 && i<length-1; i++, bytes /= 1024)
			dblBytes = bytes / 1024.0;
	}

      char* fmt = "%.02lf%s";
      char* output = malloc(snprintf(NULL, 0, fmt, dblBytes, suffix[i]));

	sprintf(output, "%.02lf%s", dblBytes, suffix[i]);
	return output;
}

uint64_t total_memory(void){
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

char* get_mem(void){
    uint64_t mem = total_memory();
    char* mem_str = human_size(mem);

    const char* fmt = "Total: %s";
    char* buf = malloc(snprintf(NULL, 0, fmt, mem_str));
    sprintf(buf, fmt, mem_str);
    return buf;
}

int main() {
    checkOS();
    if (os == NULL) return 1;
    if (displayHostname) {
        char* user = get_user();
        char* host = get_host();

        // user @ hostname \0
        char* host_data = malloc(strlen(user) + 1 + strlen(host) + 1);
        strcpy(host_data, user);
        strcat(host_data, "@");
        strcat(host_data, host);

        print("", host_data);
        free(host_data);
        free(host);
    }

    if (displayOperatingSystem)
        print("os", os);

    if (displayShell)
        print("sh", get_shell());

    if (displayPackages)
        print("pkgs", get_packages());

    if (displayKernelVersion)
        print("kernel", kernelVersion);

    if (displayCpu) {
        FILE* cpuin = fopen("/proc/cpuinfo", "r");
        print("cpu", get_cpu(cpuin));
        fclose(cpuin);
    }

    if (displayMemory)
        print("memory", get_mem());

    if (displayServer)
        print("display", get_w_server());

    return 0;
}
