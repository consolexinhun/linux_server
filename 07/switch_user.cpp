#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>

/* root 切换到目标用户 */
static bool switch_to_user(uid_t user_id, gid_t gp_id) {
    /* 先保证目标用户不是 root */
    if (user_id == 0 && gp_id == 0) {
        return false;
    }

    /* 当前用户是合法用户：root 或者目标用户 */
    gid_t gid = getgid();
    uid_t uid = getuid();

    if (((gid != 0) || uid != 0) && ((gid != gp_id) || uid != user_id)) {
        return false;  // 当前用户不是 root 又不是目标那么返回 false
    }

    if (uid != 0) {  // 不是 root 那么已经是目标用户
        return true;
    }

    /* 切换到目标用户 */
    if (setgid(gp_id) < 0 || setuid(user_id) < 0) {
        return false;
    }
    return true;
}

int main() {
    struct passwd* user;
    user = getpwnam_r("consolexin");
    bool ret = switch_to_user(user->pw_uid, user->pw_gid);
    if (ret) {
        puts("ok");
    } else {
        puts("error");
    }
    return 0;
}
