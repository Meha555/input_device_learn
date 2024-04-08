/*
 * @Author: Meha555
 * @Date: 2024-04-08 09:13:14
 * @Description:获取网络设备的信息，可以用 `ip addr` 验证
 */
#include <libudev.h>
#include <stdio.h>
#include <unistd.h>

#define SYSPATH "/sys/class/net"

int main(int argc, char* argv[])
{
    if (getuid() == 0)
        printf("Run as root, euid:%ld\n", (long)geteuid());
    else {
        printf("Not Run as root, euid:%ld\n", (long)geteuid());
        return 0;
    }

    struct udev* udev;
    struct udev_device *dev, *dev_parent;
    char device_path[128];

    if (!argv[1]) {
        fprintf(stderr, "Missing network interface name.\nexample: %s eth0\n", argv[0]);
        return 1;
    }

    /* 拼接设备路径 */
    // int snprintf ( char * s, size_t n, const char * format, ... );
    // 将格式化字符串写入s所指向的缓冲区，n为缓冲区最大容量
    snprintf(device_path, sizeof(device_path), "%s/%s", SYSPATH, argv[1]);

    /* 创建 udev 对象 */
    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Cannot create udev context.\n");
        return 1;
    }

    /* 通过路径拿到设备(udev_device) */
    dev = udev_device_new_from_syspath(udev, device_path);
    if (!dev) {
        fprintf(stderr, "Failed to get device.\n");
        return 1;
    }

    printf("I: DEVNAME=%s\n", udev_device_get_sysname(dev));
    printf("I: DEVPATH=%s\n", udev_device_get_devpath(dev));
    printf("I: MACADDR=%s\n", udev_device_get_sysattr_value(dev, "address"));

    dev_parent = udev_device_get_parent(dev);
    if (dev_parent) {
        printf("I: DRIVER=%s\n", udev_device_get_driver(dev_parent));
        printf("\tI: DEVNAME=%s\n", udev_device_get_sysname(dev_parent));
        printf("\tI: DEVPATH=%s\n", udev_device_get_devpath(dev_parent));
    }

    /* free dev */
    udev_device_unref(dev);

    /* free udev */
    udev_unref(udev);

    return 0;
}
