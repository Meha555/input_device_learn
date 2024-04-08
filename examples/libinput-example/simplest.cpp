/***
 * @Author: Meha555
 * @Date: 2024-04-07 13:43:45
 * @Description:最简单的libinput程序，只是输出监听到的输入设备事件
 */
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>

#define UNUSED(x) (void)(x)

// 打开设备文件（可用的设备文件在/dev/inputs/event*，可以用sudo evtest查看）
static int open_restricted(const char* path, int flags, void* user_data)
{
    UNUSED(user_data);
    int fd = open(path, flags);
    printf("open device fd %d\n", fd);
    return fd < 0 ? -errno : fd;
}

// 关闭设备文件
static void close_restricted(int fd, void* user_data)
{
    UNUSED(user_data);
    printf("close device fd %d\n", fd);
    close(fd);
}

const static struct libinput_interface interface = {
    .open_restricted = open_restricted, // 会在设备插入时调用
    .close_restricted = close_restricted, // 会在设备拔出时调用
};

void handleEvent(struct libinput_event* event)
{
    libinput_event_type eventType = libinput_event_get_type(event);
    switch (eventType) { // 下面给出了一些事件示例
    case LIBINPUT_EVENT_DEVICE_ADDED:
        printf("LIBINPUT_EVENT_DEVICE_ADDED\n");
        break;
    case LIBINPUT_EVENT_DEVICE_REMOVED:
        printf("LIBINPUT_EVENT_DEVICE_REMOVED\n");
        break;
    case LIBINPUT_EVENT_KEYBOARD_KEY:
        printf("LIBINPUT_EVENT_KEYBOARD_KEY\n");
        break;
    case LIBINPUT_EVENT_POINTER_MOTION:
        printf("LIBINPUT_EVENT_POINTER_MOTION\n");
        break;
    case LIBINPUT_EVENT_POINTER_BUTTON:
        printf("LIBINPUT_EVENT_POINTER_BUTTON\n");
        break;
    case LIBINPUT_EVENT_POINTER_AXIS:
        printf("LIBINPUT_EVENT_POINTER_AXIS\n");
        break;
    case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        printf("LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN\n");
        break;
    case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        printf("LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE\n");
        break;
    case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        printf("LIBINPUT_EVENT_GESTURE_SWIPE_END\n");
        break;
    case LIBINPUT_EVENT_GESTURE_PINCH_BEGIN:
        printf("LIBINPUT_EVENT_GESTURE_PINCH_BEGIN\n");
        break;
    case LIBINPUT_EVENT_GESTURE_PINCH_UPDATE:
        printf("LIBINPUT_EVENT_GESTURE_PINCH_UPDATE\n");
        break;
    case LIBINPUT_EVENT_GESTURE_PINCH_END:
        printf("LIBINPUT_EVENT_GESTURE_PINCH_END\n");
        break;
    case LIBINPUT_EVENT_TOUCH_DOWN:
        printf("LIBINPUT_EVENT_TOUCH_DOWN\n");
        break;
    case LIBINPUT_EVENT_TOUCH_UP:
        printf("LIBINPUT_EVENT_TOUCH_UP\n");
        break;
    case LIBINPUT_EVENT_TOUCH_MOTION:
        printf("LIBINPUT_EVENT_TOUCH_MOTION\n");
        break;

    default:
        printf("Other Event\n");
    }
}

int main(void)
{
    if (geteuid() == 0)
        printf("Run as root, euid:%ld\n", (long)geteuid());
    else {
        printf("Not Run as root, euid:%ld\n", (long)geteuid());
        return 0;
    }

    struct libinput* li;
    struct libinput_event* event = nullptr;
    udev* udev_context = udev_new();

    li = libinput_udev_create_context(&interface, nullptr, udev_context); // 创建libinput上下文，需要分配一个seat才会被激活，才可以使用
    libinput_udev_assign_seat(li, "seat0"); // 分配一个seat，对于每个context只能调用一次

    int fd = libinput_get_fd(li); // libinput将所有其捕获的事件都写入这个fd，我们只需要监听这个fd的可读事件即可

    // 注释掉的代码采用了学习xcb、wayland时使用的写法，这里如果这样写则只会执行一次就退出了。原因是libinput_get_event是非阻塞的，会直接返回
    // libinput_dispatch(li);
    // while ((event = libinput_get_event(li)) != nullptr) {

    //     if (event != nullptr) {
    //         handleEvent(event);
    //         libinput_event_destroy(event); // 处理完获取到的事件后，必须调用这个函数来销毁
    //     } else
    //         break;

    //     libinput_event_destroy(event);
    //     libinput_dispatch(li);
    // }

    std::array<struct pollfd, 1> pollFds({ { fd, POLLIN, 0 } });
    // 外层使用阻塞监听
    while (poll(pollFds.data(), pollFds.size(), -1) >= 0) {
        bool hasEvents = true;
        do {
            libinput_dispatch(li); // 从前面的fd中读取事件并分发到libinput内部的事件队列
            event = libinput_get_event(li); // 从libinput内部的事件队列中读取分发的事件
            if (event != nullptr) {
                handleEvent(event);
                libinput_event_destroy(event); // 处理完获取到的事件后，必须调用这个函数来销毁
            } else
                hasEvents = false;
        } while (hasEvents);
    }
    libinput_unref(li);

    return 0;
}