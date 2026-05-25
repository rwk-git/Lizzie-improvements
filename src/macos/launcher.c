/*
 * Architecture-agnostic native launcher for Lizzie.app.
 *
 * Replaces the x86_64-only JavaAppLauncher binary shipped by appbundle-maven-plugin.
 * Compiled as a universal binary (arm64 + x86_64) so it runs natively on both
 * Intel and Apple Silicon without Rosetta.
 *
 * Build (performed automatically by maven-antrun-plugin during mvn package):
 *   clang -arch arm64 -arch x86_64 -o JavaAppLauncher launcher.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <mach-o/dyld.h>

#define MAX_PATH  4096
#define MAX_CP    131072
#define MAX_ARGS  64

static void alert(const char *msg) {
    char script[2048];
    snprintf(script, sizeof(script),
        "osascript -e 'tell app \"System Events\" to display dialog \"%s\" "
        "buttons {\"OK\"} default button 1 with icon stop'", msg);
    system(script);
}

int main(int argc, char *argv[]) {
    /* Use dyld to get the real executable path (more reliable than argv[0]) */
    char self[MAX_PATH];
    uint32_t size = (uint32_t)sizeof(self);
    if (_NSGetExecutablePath(self, &size) != 0) {
        alert("Could not resolve the application path.");
        return 1;
    }
    char resolved[MAX_PATH];
    if (realpath(self, resolved) == NULL) {
        alert("Could not resolve the real application path.");
        return 1;
    }

    /*
     * Directory layout:
     *   <AppName>.app/Contents/MacOS/<this binary>
     *   <AppName>.app/Contents/Java/<all dependency jars>
     */
    char tmp[MAX_PATH];
    strncpy(tmp, resolved, sizeof(tmp) - 1);
    char *macos_dir   = dirname(tmp);          /* .../Contents/MacOS */
    char contents[MAX_PATH];
    snprintf(contents, sizeof(contents), "%s/..", macos_dir); /* .../Contents */

    /* Set working directory to the app bundle root */
    char app_root[MAX_PATH];
    snprintf(app_root, sizeof(app_root), "%s/../..", macos_dir);
    chdir(app_root);

    /* ── Locate a JVM ─────────────────────────────────────────────────────── */
    char java_home[MAX_PATH] = "";
    FILE *fp = popen("/usr/libexec/java_home -v 1.8+ 2>/dev/null", "r");
    if (fp) {
        if (fgets(java_home, (int)sizeof(java_home) - 1, fp) == NULL)
            java_home[0] = '\0';
        pclose(fp);
        java_home[strcspn(java_home, "\n")] = '\0';
    }
    if (!java_home[0]) {
        /* Fallback: whatever java_home considers the default */
        fp = popen("/usr/libexec/java_home 2>/dev/null", "r");
        if (fp) {
            if (fgets(java_home, (int)sizeof(java_home) - 1, fp) == NULL)
                java_home[0] = '\0';
            pclose(fp);
            java_home[strcspn(java_home, "\n")] = '\0';
        }
    }
    if (!java_home[0]) {
        alert("Java 8 or later is required to run Lizzie.\n"
              "Please install a JDK from https://adoptium.net.");
        return 1;
    }

    char java_exe[MAX_PATH];
    snprintf(java_exe, sizeof(java_exe), "%s/bin/java", java_home);

    /* ── Build classpath from all JARs in Contents/Java ──────────────────── */
    char java_dir[MAX_PATH];
    snprintf(java_dir, sizeof(java_dir), "%s/Java", contents);

    char find_cmd[MAX_PATH + 64];
    snprintf(find_cmd, sizeof(find_cmd),
             "find '%s' -name '*.jar' 2>/dev/null | tr '\\n' ':'", java_dir);

    static char classpath[MAX_CP];
    fp = popen(find_cmd, "r");
    if (fp) {
        size_t n = fread(classpath, 1, sizeof(classpath) - 1, fp);
        classpath[n] = '\0';
        pclose(fp);
    }
    if (!classpath[0]) {
        alert("Application JARs not found in the bundle.\n"
              "Please rebuild the application with: mvn package");
        return 1;
    }

    /* ── Assemble the java argument list ─────────────────────────────────── */
    char *args[MAX_ARGS];
    int   ai = 0;
    args[ai++] = java_exe;
    args[ai++] = "-Dapple.laf.useScreenMenuBar=true";
    args[ai++] = "-Xdock:name=Lizzie";
    args[ai++] = "-cp";
    args[ai++] = classpath;
    args[ai++] = "featurecat.lizzie.Lizzie";
    for (int i = 1; i < argc && ai < MAX_ARGS - 1; i++)
        args[ai++] = argv[i];
    args[ai] = NULL;

    execv(java_exe, args);

    /* execv only returns on error */
    char errmsg[512];
    snprintf(errmsg, sizeof(errmsg), "Failed to start Java: %s", strerror(errno));
    alert(errmsg);
    return 1;
}
