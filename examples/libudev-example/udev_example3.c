/*
 * @Author: Meha555
 * @Date: 2024-04-08 09:13:14
 * @Description:监控input子系统的热插拔事件
 */
#include <errno.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main()
{
    if (getuid() == 0)
        printf("Run as root, euid:%ld\n", (long)geteuid());
    else {
        printf("Not Run as root, euid:%ld\n", (long)geteuid());
        return 0;
    }

    struct udev* udev;
    struct udev_device* dev;
    struct udev_monitor* mon;
    int fd;

    /* 创建 udev 对象 */
    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Can't create udev\n");
        return 1;
    }

    /* 通过netlink拿到事件监听器(udev_monitor) */
    mon = udev_monitor_new_from_netlink(udev, "udev");
    /* 设置事件监视器过滤条件为input子系统，从而监听input子系统的事件 */
    // udev_monitor_filter_add_match_subsystem_devtype(mon, "input", "event*"); // 为什么加上这句就不行了
    /* bind 被监听子系统的socket到相应的udev_monitor */
    udev_monitor_enable_receiving(mon);
    /* 设置事件监听器的接收缓冲区大小 */
    udev_monitor_set_receive_buffer_size(mon, BUF_SIZE); // 好像不设置也可以
    fd = udev_monitor_get_fd(mon);

    // 监听这个内部fd来获取设备上发生的事件
    int epfd = epoll_create(1);
    struct epoll_event* events = (struct epoll_event*)malloc(sizeof(struct epoll_event)); // 只用监听一个fd
    struct epoll_event ev = {
        .data.fd = fd,
        .events = EPOLLIN
    };
    epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);

    while (1) {
        int ret = epoll_wait(epfd, events, 1, -1); // 阻塞等待
        if (ret > 0) {
            dev = udev_monitor_receive_device(mon);
            if (dev) {
                printf("I: ACTION=%s\n", udev_device_get_action(dev));
                printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
                printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
                printf("I: MACADDR=%s\n", udev_device_get_sysattr_value(dev, "address"));
                printf("----------\n");

                /* free dev */
                udev_device_unref(dev);
            }
        } else
            fprintf(stderr, "Error: %s\n", strerror(errno));
        /* 500 milliseconds */
        usleep(500 * 1000);
    }
    free(events);
    close(epfd);

    // while (1) {
    //     fd_set fds;
    //     int ret;

    //     FD_ZERO(&fds);
    //     FD_SET(fd, &fds);

    //     ret = select(fd + 1, &fds, NULL, NULL, NULL);
    //     if (ret > 0 && FD_ISSET(fd, &fds)) {
    //         dev = udev_monitor_receive_device(mon);
    //         if (dev) {
    //             printf("I: ACTION=%s\n", udev_device_get_action(dev));
    //             printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
    //             printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
    //             printf("I: MACADDR=%s\n", udev_device_get_sysattr_value(dev, "address"));
    //             printf("---\n");

    //             /* free dev */
    //             udev_device_unref(dev);
    //         }
    //     } else
    //         fprintf(stderr, "Error: %s\n", strerror(errno));
    //     /* 500 milliseconds */
    //     usleep(500 * 1000);
    // }

    /* free udev */
    udev_unref(udev);

    return 0;
}
