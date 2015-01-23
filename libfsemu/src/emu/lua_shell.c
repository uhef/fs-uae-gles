#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WITH_LUA

#include "lua_shell.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <lualib.h>
#ifdef __cplusplus
}
#endif

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include <fs/net.h>
#include <fs/log.h>
#include <fs/thread.h>
#include <fs/conf.h>
#include <fs/emu_lua.h>

static const char *hello_msg = "FS-UAE " PACKAGE_VERSION " " LUA_VERSION "\n";
static const char *bye_msg = "bye.\n";
static const char *fail_msg = "FAILED!\n";
static const char *default_addr = "127.0.0.1";
static const char *default_port = "6800";
static fs_thread *g_listen_thread = NULL;
static int g_listen_fd;
static int g_keep_listening;
static int g_client_fd;

#define MAX_CMD_LEN     256
static char buf[MAX_CMD_LEN];

static int myprintf(int fd, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(buf, MAX_CMD_LEN, fmt, ap);
    va_end(ap);
    if(n>0) {
        return write(fd, buf, n);
    } else {
        return 0;
    }
}

static int handle_command(int fd, lua_State *L, const char *cmd_line)
{
    // parse and execute command
    if(luaL_loadbuffer(L, cmd_line, strlen(cmd_line), "=shell")
        || lua_pcall(L, 0, 0, 0)) {
        // error
        const char *err_msg = lua_tostring(L, -1);
        myprintf(fd, "ERROR: %s\n", err_msg);
        lua_pop(L,1);
    }
    return 0;
}

static int read_line(int fd, const char *prompt)
{
    int result;

    // send prompt
    result = write(fd, prompt, strlen(prompt));
    if(result < 0) {
        return result;
    }

    // read line
    result = read(fd, buf, MAX_CMD_LEN-1);
    if(result <= 0) {
        return result;
    }
    if(buf[result-1] == '\n') {
        buf[result-1] = '\0';
        result--;
    }
    if(result>0) {
        if(buf[result-1] == '\r') {
            buf[result-1] = '\0';
            result--;
        }
    }
    return result;
}

static void main_loop(int fd, lua_State *L)
{
    int result = 0;
    while(1) {
        // read a command
        result = read_line(fd, "> ");
        if(result < 0) {
            break;
        }
        // exit shell? -> CTRL+D
        if(buf[0] == '\x04') {
            break;
        }
        if(!strcmp(buf,"endshell")) {
            break;
        }
        // execute command
        result = handle_command(fd, L, buf);
        if(result < 0) {
            break;
        }
    }
    // show final result
    if(result < 0) {
        write(fd, fail_msg, strlen(fail_msg));
    } else {
        write(fd, bye_msg, strlen(bye_msg));
    }
}

static void handle_client(int fd)
{
    fs_log("lua-shell: client connect\n");

    // welcome
    write(fd, hello_msg, strlen(hello_msg));

    // create lua context
    lua_State *L = fs_emu_lua_create_state();
    if(L == NULL) {
        // error
    }
    else {
        // enter main loop
        main_loop(fd, L);

        // free context
        fs_emu_lua_destroy_state(L);
    }

    // close connection
    close(fd);
    fs_log("lua-shell: client disconnect\n");
}

static void *lua_shell_listener(void *data)
{
    struct sockaddr_in client_addr;
    
    fs_log("lua-shell: +listener\n");
    while(g_keep_listening) {
        // get next client
        socklen_t cli_size = sizeof(client_addr);
        int client_fd = accept(g_listen_fd, (struct sockaddr *)&client_addr, &cli_size);
        if(client_fd < 0) {
            fs_log("lua-shell: failed accept: %s\n", strerror(errno));
            break;
        }
        g_client_fd = client_fd;
        handle_client(client_fd);
        g_client_fd = -1;
    }
    fs_log("lua-shell: -listener\n");
    return NULL;
}

void fs_emu_lua_shell_init(void)
{
    // is shell enabled?
    if(!fs_config_get_boolean("lua_shell")) {
        fs_log("lua-shell: disabled\n");
        return;
    }

    // get config values
    const char *addr = fs_config_get_string("lua_shell_addr");
    if(addr == NULL) {
        addr = default_addr;
    }
    const char *port = fs_config_get_string("lua_shell_port");
    if(port == NULL) {
        port = default_port;
    }
    fs_log("lua-shell: addr=%s, port=%s\n", addr, port);

    // find address
    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    int s = getaddrinfo(addr, port, &hints, &result);
    if (s != 0) {
        fs_log("lua-shell: getaddrinfo failed: %s\n", gai_strerror(s));
        return;
    }
    if(s > 1) {
        fs_log("lua-shell: found multiple address matches... taking first\n");
    } 

    // try to open socket
    g_listen_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(g_listen_fd < 0) {
        freeaddrinfo(result);
        fs_log("lua-shell: can't create socket: %s\n", strerror(errno));
        return;
    }
    
    // bind socket
    if(bind(g_listen_fd, result->ai_addr, result->ai_addrlen) < 0) {
        freeaddrinfo(result);
        close(g_listen_fd);
        fs_log("lua-shell: can't bind socket: %s\n", strerror(errno));
        return;
    }

    // cleanup addrinfo
    freeaddrinfo(result);

    // start listening
    if(listen(g_listen_fd, 5) < 0) {
        close(g_listen_fd);
        fs_log("lua-shell: can't listen on socket: %s\n", strerror(errno));
        return;
    }

    // launch listener thread
    g_keep_listening = 1;
    g_listen_thread = fs_thread_create("lua_shell_listener", 
                                       lua_shell_listener, NULL);
}

void fs_emu_lua_shell_free(void)
{
    fs_log("lua-shell: stopping...\n");

    // close client socket
    if(g_client_fd >= 0) {
        close(g_client_fd);
    }

    // close listener socket
    if(g_listen_fd >= 0) {
        close(g_listen_fd);
    }
    
    // end listener thread
    if(g_listen_thread != NULL) {
        fs_thread_wait(g_listen_thread);
        fs_thread_free(g_listen_thread);
        g_listen_thread = NULL;
    }
}

#endif // WITH_LUA
