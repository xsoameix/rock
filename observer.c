#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>

typedef struct sockaddr_in6     addr_in6_t;
typedef struct sockaddr_in      addr_in_t;
typedef struct sockaddr         addr_t;
typedef struct sockaddr_storage addr_store_t;
typedef struct addrinfo         addr_info_t;

int
valid_addr(const char * value) {
  addr_info_t hint = {0}, * res;
  int ret, valid = 1;
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;
  ret = getaddrinfo(value, NULL, &hint, &res);
  if (ret) return valid;
  if (res->ai_family == AF_INET || res->ai_family == AF_INET6) valid = 0;
  freeaddrinfo(res);
  return valid;
}

int
valid_port(const char * str) {
  in_port_t port = 0;
  size_t i, len = strlen(str);
  if (len > 5) return 1;
  for (i = 0; i < len; i++) if (!isdigit(str[i])) return 1;
  return 0;
}

void
get_addr_port(char ** addr, char ** port) {
  char * value;
  value = getenv("OBSERVER_ADDR");
  if (value != NULL && !valid_addr(value)) * addr = value;
  value = getenv("OBSERVER_PORT");
  if (value != NULL && !valid_port(value)) * port = value;
}

addr_info_t *
build_addr(char ** addr, char ** port) {
  addr_info_t hint = {0}, * res;
  get_addr_port(addr, port);
  getaddrinfo(* addr, * port, &hint, &res);
  return res;
}

#define TITLE "[observer]"
#define INFO(message, ...) \
  do { \
    printf(TITLE " " message, ##__VA_ARGS__), \
    fflush(stdout); \
  } while(0)
#define ERR(message) \
  do { \
    size_t size = strlen(TITLE) + strlen(message) + 1; \
    char * buf = malloc(size); \
    snprintf(buf, size, "%s %s", TITLE, message); \
    perror(message); \
    free(buf); \
  } while(0)

void
close_server(char * message, int sock, addr_info_t * addr) {
  if (errno) {
    ERR(message);
    INFO("Exit\n");
    if (sock != -1) close(sock);
    if (addr) freeaddrinfo(addr);
    exit(0);
  } else {
    INFO("Exit\n");
    if (sock != -1) close(sock);
    freeaddrinfo(addr);
  }
}

int
create_socket(addr_info_t * addr) {
  int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (sock == -1) close_server("socket failed", -1, addr);
  return sock;
}

void
set_socket_opt(int sock, addr_info_t * addr) {
  int on = 1, ret;
  ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (ret == -1) close_server("setsocketopt 'SO_REUSEADDR' failed", sock, addr);
  if (addr->ai_addr->sa_family == AF_INET6) {
    ret = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
    if (ret == -1) close_server("setsockopt 'IPV6_V6ONLY' failed", sock, addr);
  }
}

void
bind_socket(int sock, addr_info_t * addr) {
  int ret = bind(sock, addr->ai_addr, addr->ai_addrlen);
  if (ret == -1) close_server("bind failed", sock, addr);
}

void
listen_socket(int sock, addr_info_t * info, char * addr, char * port) {
  int ret = listen(sock, 20);
  if (ret == -1) close_server("listen failed", sock, info);
  INFO("Listening on %s:%s\n", addr, port);
}

static jmp_buf catch = {0};

void
interrupt(int signo) {
  longjmp(catch, 1);
}

void
set_interrupt(int sock, addr_info_t * addr) {
  void (* sig)(int);
  sig = signal(SIGINT, interrupt);
  if (sig == SIG_ERR) close_server("cannot catch SIGINT", sock, addr);
  sig = signal(SIGTERM, interrupt);
  if (sig == SIG_ERR) close_server("cannot catch SIGTERM", sock, addr);
}

void
close_sock(char * message, int sock, jmp_buf * fail) {
  ERR(message);
  close(sock), longjmp(* fail, 1);
}

void
recv_sock(int sock, void * buf, size_t len, jmp_buf * fail) {
  ssize_t ret;
  size_t total = 0;
  while (total < len) {
    ret = recv(sock, buf + total, len - total, 0);
    if (ret == 0)  close_sock("disconnected", sock, fail);
    if (ret == -1) close_sock("recv failed", sock, fail);
    total += (size_t) ret;
  }
}

void
send_sock(int sock, void * buf, size_t len, jmp_buf * fail) {
  ssize_t ret;
  size_t total = 0;
  if (!fail) return;
  while (total < len) {
    ret = send(sock, buf + total, len - total, 0);
    if (ret == -1) close_sock("send failed", sock, fail);
    total += (size_t) ret;
  }
}

int
read_sender(int out, int * start) {
  size_t line;
  char buf[5], * pos = buf, * next;
  int len = read(out, buf, sizeof(buf) - 1);
  if (len < 0) {
    ERR("read failed");
    return 2;
  }
  if (len == 0) return 1;
  buf[len] = '\0';
  while ((next = strchr(pos, '\n')) != NULL) {
    if (!* start) printf("[sender] ");
    printf("%.*s\n", next - pos, pos);
    pos = next + 1;
    * start = 0;
  }
  if (!* start && * pos)
    printf("[sender] %s", pos), * start = 1;
  else
    printf("%s", pos);
  fflush(stdout);
  return 0;
}

#define KILL_OK  "Kill the sender ...\n"
#define KILL_ERR "Cannot kill the sender\n"
#define RUN_OK   "Run the sender ...\n"
#define RUN_ERR  "Cannot run the sender\n"

void
kill_sender(int sock, pid_t sender, int out, int * start, jmp_buf * fail) {
  int ret = kill(sender, SIGINT), status;
  if (ret != 0) {
    send_sock(sock, KILL_ERR, strlen(KILL_ERR), fail);
    ERR("kill failed");
    return;
  }
  while (!read_sender(out, start));
  if (waitpid(sender, &status, 0) != sender) {
    send_sock(sock, KILL_ERR, strlen(KILL_ERR), fail);
    ERR("waitpid failed");
    return;
  }
  send_sock(sock, KILL_OK, strlen(KILL_OK), fail);
}

typedef int    env_chk_t(char * key, char ** val);
typedef size_t env_set_t(char * buf, char * key, char * val, int set);

int
sender_argv(char * key, char ** val) {
  return key == NULL;
}

int
sender_envp(char * key, char ** val) {
  return (* val = getenv(key)) == NULL;
}

int
sender_bind(char * key, char ** val) {
  return (* val = getenv(key)) == NULL || (* val = getenv(* val)) == NULL;
}

size_t
key_only(char * buf, char * key, char * val, int set) {
  size_t size = strlen(key) + 1;
  if (!set) snprintf(buf, size, "%s", key);
  return size;
}

size_t
key_and_val(char * buf, char * key, char * val, int set) {
  size_t size = strlen(key) + 1 + strlen(val) + 1;
  if (!set) snprintf(buf, size, "%s=%s", key, val);
  return size;
}

void
sender_env(char * env, char * del, env_chk_t * check, env_set_t * setup,
    char ** strs, char *** envp, int set) {
  char * str, * key, * val;
  size_t size, i;
  if (env == NULL) return;
  str = strdup(env);
  for (key = strtok(str, del); key != NULL; key = strtok(NULL, del)) {
    if (check(key, &val)) continue;
    size = setup(* strs, key, val, set);
    if (set)    * (size_t *) envp += 1, * strs += size;
    else ** envp = * strs, * envp += 1, * strs += size;
  }
  free(str);
}

void
sender_env_init(char ** strs, char *** envp) {
  void * mem;
  size_t envp_size = * (size_t *) envp * sizeof(** envp);
  size_t strs_size = * (size_t *) strs;
  * envp = mem = malloc(envp_size + strs_size), * strs = mem + envp_size;
}

char **
get_sender_argv(char * exe) {
  char * argv = getenv("OBSERVER_ARGV");
  char * strs = (void *) strlen(exe) + 1;
  char ** ret = (void *) 2, ** ori; /* {exe, ..., NULL} */
  sender_env(argv, " ", sender_argv, key_only, &strs, &ret, 1);
  sender_env_init(&strs, &ret), ori = ret;
  * ret++ = exe;
  sender_env(argv, " ", sender_argv, key_only, &strs, &ret, 0);
  * ret = NULL;
  return ori;
}

char **
get_sender_envp(void) {
  char * envp = getenv("OBSERVER_ENVP");
  char * bind = getenv("OBSERVER_BIND");
  char * strs = 0, ** ret = (void *) 1, ** ori;
  sender_env(envp, ":", sender_envp, key_and_val, &strs, &ret, 1);
  sender_env(bind, ":", sender_bind, key_and_val, &strs, &ret, 1);
  sender_env_init(&strs, &ret), ori = ret;
  sender_env(envp, ":", sender_envp, key_and_val, &strs, &ret, 0);
  sender_env(bind, ":", sender_bind, key_and_val, &strs, &ret, 0);
  * ret = NULL;
  return ori;
}

void
do_run_sender(void) {
  char *  cwd  = getenv("OBSERVER_CWD");
  char *  exe  = getenv("OBSERVER_EXEC");
  char ** argv = get_sender_argv(exe);
  char ** envp = get_sender_envp();
  if (cwd != NULL) chdir(cwd);
  if (exe == NULL) {
    perror("environment variable OBSERVER_EXEC is not set");
    return;
  }
  execve(exe, argv, envp);
  free(envp);
  free(argv);
}

void
run_sender(int sock, pid_t * sender, int * out, jmp_buf * fail) {
  int pfd[2], save[2], ret;
  if (pipe(pfd) == -1) {
    send_sock(sock, RUN_ERR, strlen(RUN_ERR), fail);
    ERR("pipe failed");
    return;
  }
  * sender = fork();
  if (* sender > 0) {
    close(pfd[1]);
    * out = pfd[0];
    return;
  }
  if (* sender < 0) {
    send_sock(sock, RUN_ERR, strlen(RUN_ERR), fail);
    ERR("fork failed");
    return;
  }
  send_sock(sock, RUN_OK, strlen(RUN_OK), fail);
  close(pfd[0]);
  save[0] = dup(1);
  save[1] = dup(2);
  dup2(pfd[1], 1);
  dup2(pfd[1], 2);
  close(pfd[1]);
  do_run_sender();
  dup2(save[1], 2);
  dup2(save[0], 1);
  if (!fail) perror("execve failed"); /* !fail == first time */
  for (;;) pause();
}

#define EXEC_KILL 0
#define EXEC_RUN  1
#define EXEC_EXIT 2

void
exec_command(int sock, pid_t * sender, int * out, int * start) {
  jmp_buf fail = {0};
  int leave;
  uint32_t cmd;
  for (leave = setjmp(fail); !leave; ) {
    recv_sock(sock, &cmd, sizeof(cmd), &fail), cmd = ntohl(cmd);
    if (cmd == EXEC_KILL) kill_sender(sock, * sender, * out, start, &fail);
    if (cmd == EXEC_RUN)  run_sender(sock, sender, out, &fail);
    if (cmd == EXEC_EXIT) leave = 1;
  }
}

void
do_accept_socket(pid_t * sender, int * out, int * start, int server_sock) {
  addr_store_t addr = {0};
  socklen_t addrlen = sizeof(addr);
  int sock = accept(server_sock, (addr_t *) &addr, &addrlen);
  if (sock == -1) {
    ERR("accept failed");
    return;
  }
  exec_command(sock, sender, out, start);
  close(sock);
}

void
do_select(pid_t * sender, int * out, int * start, int sock) {
  int fdmax, i;
  fd_set readset;
  for (;;) {
    FD_ZERO(&readset);
    FD_SET(sock, &readset);
    FD_SET(fdmax = * out, &readset);
    if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
      ERR("select failed");
      return;
    }
    for (i = 0; i <= fdmax; i++) {
      if (!FD_ISSET(i, &readset)) continue;
      if (i == * out) read_sender(* out, start);
      if (i == sock) do_accept_socket(sender, out, start, sock);
    }
  }
}

void
accept_socket(int sock, addr_info_t * addr) {
  pid_t sender;
  int leave, start = 0, out;
  run_sender(0, &sender, &out, 0);
  set_interrupt(sock, addr), leave = setjmp(catch);
  while (!leave) do_select(&sender, &out, &start, sock);
  kill_sender(0, sender, out, &start, 0);
}

int
main(void) {
  char * addr = "127.0.0.1", * port = "60000";
  addr_info_t * info = build_addr(&addr, &port);
  int sock = create_socket(info);
  set_socket_opt(sock, info);
  bind_socket(sock, info);
  listen_socket(sock, info, addr, port);
  accept_socket(sock, info);
  close_server(0, sock, info);
  return 0;
}
