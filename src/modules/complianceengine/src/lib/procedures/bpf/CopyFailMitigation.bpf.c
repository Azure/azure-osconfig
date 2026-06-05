#include "vmlinux.h"

#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define AF_ALG 38
#define EPERM 1
#define MAX_ALLOWED_PATHS 16
#define MAX_PATH_LEN 256

struct AllowedPath
{
    char path[MAX_PATH_LEN];
};

struct
{
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, MAX_ALLOWED_PATHS);
    __type(key, struct AllowedPath);
    __type(value, __u8);
} allowed_paths SEC(".maps");

static __always_inline int is_allowed_path(const struct AllowedPath* path)
{
    __u8* value = bpf_map_lookup_elem(&allowed_paths, path);
    return value != 0;
}

SEC("lsm/socket_bind")
int BPF_PROG(copyfail_afalg, struct socket* sock, struct sockaddr* address, int addrlen, int ret)
{
    if (ret != 0)
    {
        return ret;
    }

    unsigned short family = BPF_CORE_READ(sock, sk, __sk_common.skc_family);
    if (family != AF_ALG)
    {
        return 0;
    }

    struct task_struct* task = (struct task_struct*) bpf_get_current_task_btf();
    struct mm_struct* mm = __builtin_preserve_access_index(task->mm);
    if (!mm)
    {
        return -EPERM;
    }

    struct file* exe_file = __builtin_preserve_access_index(mm->exe_file);
    if (!exe_file)
    {
        return -EPERM;
    }

    struct AllowedPath path = {};
    struct path* exe_path = __builtin_preserve_access_index(&exe_file->f_path);
    long path_len = bpf_d_path(exe_path, path.path, sizeof(path.path));
    if (path_len < 0)
    {
        return -EPERM;
    }

    if (is_allowed_path(&path))
    {
        return 0;
    }

    return -EPERM;
}

char LICENSE[] SEC("license") = "GPL";