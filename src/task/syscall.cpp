// System Call Interface

#include "task.hpp"
#include "arch.hpp"
#include "common.hpp"

namespace task {
namespace syscall {

// System call numbers
enum class Syscall : u64 {
    Exit = 0,
    Fork = 1,
    Exec = 2,
    Wait = 3,
    Read = 4,
    Write = 5,
    Open = 6,
    Close = 7,
    Seek = 8,
    Stat = 9,
    Mmap = 10,
    Munmap = 11,
    Brk = 12,
    Getpid = 13,
    Getppid = 14,
    Getuid = 15,
    Getgid = 16,
    Setuid = 17,
    Setgid = 18,
    Kill = 19,
    Signal = 20,
    Sigaction = 21,
    Sigprocmask = 22,
    Ioctl = 23,
    Fcntl = 24,
    Getdents = 25,
    Mkdir = 26,
    Rmdir = 27,
    Unlink = 28,
    Rename = 29,
    Chdir = 30,
    Getcwd = 31,
    Time = 32,
    Nanosleep = 33,
    SchedYield = 34,
    SchedGetPriority = 35,
    SchedSetPriority = 36,
    Futex = 37,
    ShmGet = 38,
    ShmAt = 39,
    ShmDt = 40,
    ShmCtl = 41,
};

// Error numbers (POSIX-compatible)
enum class Errno : i32 {
    EPERM  = 1,   // Operation not permitted
    ENOENT = 2,   // No such file or directory
    ESRCH  = 3,   // No such process
    EINTR  = 4,   // Interrupted system call
    EIO    = 5,   // I/O error
    ENXIO  = 6,   // No such device or address
    E2BIG  = 7,   // Argument list too long
    ENOEXEC = 8,  // Exec format error
    EBADF  = 9,   // Bad file descriptor
    ECHILD = 10,  // No child processes
    EAGAIN = 11,  // Try again
    ENOMEM = 12,  // Out of memory
    EACCES = 13,  // Permission denied
    EFAULT = 14,  // Bad address
    EBUSY  = 16,  // Device or resource busy
    EEXIST = 17,  // File exists
    EXDEV  = 18,  // Cross-device link
    ENODEV = 19,  // No such device
    ENOTDIR = 20, // Not a directory
    EISDIR = 21,  // Is a directory
    EINVAL = 22,  // Invalid argument
    ENFILE = 23,  // File table overflow
    EMFILE = 24,  // Too many open files
    ENOTTY = 25,  // Not a typewriter
    ETXTBSY = 26, // Text file busy
    EFBIG  = 27,  // File too large
    ENOSPC = 28,  // No space left on device
    ESPIPE = 29,  // Illegal seek
    EROFS  = 30,  // Read-only file system
    EMLINK = 31,  // Too many links
    EPIPE  = 32,  // Broken pipe
    EDOM   = 33,  // Math argument out of domain
    ERANGE = 34,  // Math result not representable
    ENOSYS = 38,  // Function not implemented
};

// System call arguments (x86_64 System V ABI: RDI, RSI, RDX, R10, R8, R9)
struct SyscallArgs {
    u64 arg0, arg1, arg2, arg3, arg4, arg5;
};

// System call result
struct SyscallResult {
    i64 value;
    Errno error;
};

// Dispatch system call
extern "C" i64 dispatch(Syscall num, SyscallArgs args) {
    switch (num) {
        case Syscall::Exit:
            return sys_exit(args.arg0);
        case Syscall::Fork:
            return sys_fork();
        case Syscall::Exec:
            return sys_exec(args.arg0, args.arg1);
        case Syscall::Wait:
            return sys_wait(args.arg0);
        case Syscall::Read:
            return sys_read(args.arg0, args.arg1, args.arg2);
        case Syscall::Write:
            return sys_write(args.arg0, args.arg1, args.arg2);
        case Syscall::Open:
            return sys_open(args.arg0, args.arg1, args.arg2);
        case Syscall::Close:
            return sys_close(args.arg0);
        case Syscall::Seek:
            return sys_seek(args.arg0, args.arg1, args.arg2);
        case Syscall::Stat:
            return sys_stat(args.arg0, args.arg1);
        case Syscall::Mmap:
            return sys_mmap(args.arg0, args.arg1, args.arg2, args.arg3, args.arg4, args.arg5);
        case Syscall::Munmap:
            return sys_munmap(args.arg0, args.arg1);
        case Syscall::Brk:
            return sys_brk(args.arg0);
        case Syscall::Getpid:
            return sys_getpid();
        case Syscall::Getppid:
            return sys_getppid();
        case Syscall::Getuid:
            return sys_getuid();
        case Syscall::Getgid:
            return sys_getgid();
        case Syscall::Setuid:
            return sys_setuid(args.arg0);
        case Syscall::Setgid:
            return sys_setgid(args.arg0);
        case Syscall::Kill:
            return sys_kill(args.arg0, args.arg1);
        case Syscall::Signal:
            return sys_signal(args.arg0, args.arg1);
        case Syscall::Sigaction:
            return sys_sigaction(args.arg0, args.arg1, args.arg2);
        case Syscall::Sigprocmask:
            return sys_sigprocmask(args.arg0, args.arg1, args.arg2);
        case Syscall::Ioctl:
            return sys_ioctl(args.arg0, args.arg1, args.arg2);
        case Syscall::Fcntl:
            return sys_fcntl(args.arg0, args.arg1, args.arg2);
        case Syscall::Getdents:
            return sys_getdents(args.arg0, args.arg1, args.arg2);
        case Syscall::Mkdir:
            return sys_mkdir(args.arg0, args.arg1);
        case Syscall::Rmdir:
            return sys_rmdir(args.arg0);
        case Syscall::Unlink:
            return sys_unlink(args.arg0);
        case Syscall::Rename:
            return sys_rename(args.arg0, args.arg1);
        case Syscall::Chdir:
            return sys_chdir(args.arg0);
        case Syscall::Getcwd:
            return sys_getcwd(args.arg0, args.arg1);
        case Syscall::Time:
            return sys_time(args.arg0);
        case Syscall::Nanosleep:
            return sys_nanosleep(args.arg0, args.arg1);
        case Syscall::SchedYield:
            return sys_sched_yield();
        case Syscall::SchedGetPriority:
            return sys_sched_getpriority(args.arg0, args.arg1);
        case Syscall::SchedSetPriority:
            return sys_sched_setpriority(args.arg0, args.arg1, args.arg2);
        case Syscall::Futex:
            return sys_futex(args.arg0, args.arg1, args.arg2, args.arg3, args.arg4, args.arg5);
        case Syscall::ShmGet:
            return sys_shmget(args.arg0, args.arg1, args.arg2);
        case Syscall::ShmAt:
            return sys_shmat(args.arg0, args.arg1, args.arg2);
        case Syscall::ShmDt:
            return sys_shmdt(args.arg0);
        case Syscall::ShmCtl:
            return sys_shmctl(args.arg0, args.arg1, args.arg2);
        default:
            return static_cast<i64>(Errno::ENOSYS);
    }
}

// System call implementations

i64 sys_exit(u64 code) {
    Task* current = task::current_task();
    if (current && current != task::get_idle_task()) {
        current->exit_code = static_cast<i32>(code);
        task::exit(static_cast<i32>(code));
    }
    return 0;
}

i64 sys_fork() {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_exec(u64 path, u64 argv) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_wait(u64 pid) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_read(u64 fd, u64 buf, u64 count) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_write(u64 fd, u64 buf, u64 count) {
    // Write to stdout/stderr (serial)
    if (fd == 1 || fd == 2) {
        const char* ptr = reinterpret_cast<const char*>(buf);
        usize len = static_cast<usize>(count);
        for (usize i = 0; i < len; i++) {
            arch::serial::write_char(ptr[i]);
        }
        return static_cast<i64>(len);
    }
    return static_cast<i64>(Errno::EBADF);
}

i64 sys_open(u64 path, u64 flags, u64 mode) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_close(u64 fd) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_seek(u64 fd, u64 offset, u64 whence) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_stat(u64 path, u64 statbuf) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_mmap(u64 addr, u64 len, u64 prot, u64 flags, u64 fd, u64 offset) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_munmap(u64 addr, u64 len) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_brk(u64 addr) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_getpid() {
    Task* current = task::current_task();
    return current ? static_cast<i64>(current->id) : 1;
}

i64 sys_getppid() {
    Task* current = task::current_task();
    return current ? static_cast<i64>(current->parent) : 0;
}

i64 sys_getuid() {
    return 0;  // Root
}

i64 sys_getgid() {
    return 0;  // Root
}

i64 sys_setuid(u64 uid) {
    return static_cast<i64>(Errno::EPERM);
}

i64 sys_setgid(u64 gid) {
    return static_cast<i64>(Errno::EPERM);
}

i64 sys_kill(u64 pid, u64 sig) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_signal(u64 signum, u64 handler) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_sigaction(u64 signum, u64 act, u64 oldact) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_sigprocmask(u64 how, u64 set, u64 oldset) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_ioctl(u64 fd, u64 request, u64 arg) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_fcntl(u64 fd, u64 cmd, u64 arg) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_getdents(u64 fd, u64 buf, u64 count) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_mkdir(u64 path, u64 mode) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_rmdir(u64 path) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_unlink(u64 path) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_rename(u64 oldpath, u64 newpath) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_chdir(u64 path) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_getcwd(u64 buf, u64 size) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_time(u64 tloc) {
    u64 secs = arch::time::uptime_ms() / 1000;
    if (tloc) {
        *reinterpret_cast<u64*>(tloc) = secs;
    }
    return static_cast<i64>(secs);
}

i64 sys_nanosleep(u64 req, u64 rem) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_sched_yield() {
    task::yield();
    return 0;
}

i64 sys_sched_getpriority(u64 which, u64 who) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_sched_setpriority(u64 which, u64 who, u64 prio) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_futex(u64 uaddr, u64 op, u64 val, u64 timeout, u64 uaddr2, u64 val3) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_shmget(u64 key, u64 size, u64 flags) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_shmat(u64 shmid, u64 shmaddr, u64 flags) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_shmdt(u64 shmaddr) {
    return static_cast<i64>(Errno::ENOSYS);
}

i64 sys_shmctl(u64 shmid, u64 cmd, u64 buf) {
    return static_cast<i64>(Errno::ENOSYS);
}

} // namespace syscall
} // namespace task