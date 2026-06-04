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
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MAX_ALLOWED_PATHS);
    __type(key, __u32);
    __type(value, struct AllowedPath);
} allowed_paths SEC(".maps");

struct
{
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u32);
} allowed_path_count SEC(".maps");

static __always_inline int path_equals(const char* left, const char* right)
{
#pragma unroll
    for (int i = 0; i < MAX_PATH_LEN; i++)
    {
        if (left[i] != right[i])
        {
            return 0;
        }

        if (left[i] == '\0')
        {
            return 1;
        }
    }

    return 0;
}

static __always_inline int is_allowed_path(const char* path)
{
    __u32 count_key = 0;
    __u32* configured_count = bpf_map_lookup_elem(&allowed_path_count, &count_key);
    if (!configured_count)
    {
        return 0;
    }

    __u32 count = *configured_count;
    if (count > MAX_ALLOWED_PATHS)
    {
        count = MAX_ALLOWED_PATHS;
    }

    for (__u32 i = 0; i < MAX_ALLOWED_PATHS; i++)
    {
        if (i >= count)
        {
            break;
        }

        struct AllowedPath* allowed_path = bpf_map_lookup_elem(&allowed_paths, &i);
        if (allowed_path && path_equals(path, allowed_path->path))
        {
            return 1;
        }
    }

    return 0;
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

    char path[MAX_PATH_LEN];
    struct path* exe_path = __builtin_preserve_access_index(&exe_file->f_path);
    long path_len = bpf_d_path(exe_path, path, sizeof(path));
    if (path_len < 0)
    {
        return -EPERM;
    }

    if (is_allowed_path(path))
    {
        return 0;
    }

    return -EPERM;
}

char LICENSE[] SEC("license") = "GPL";