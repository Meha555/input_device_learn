/*
 * @Author: Meha555
 * @Date: 2024-04-07 16:45:33
 * @Description:实现了一个简单的evtest，需要打开一个/dev/input下的输入设备
 */

#include <errno.h>
#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void
print_abs_bits(struct libevdev* dev, int axis)
{
    const struct input_absinfo* abs;

    if (!libevdev_has_event_code(dev, EV_ABS, axis))
        return;

    abs = libevdev_get_abs_info(dev, axis);

    printf("	Value	%6d\n", abs->value);
    printf("	Min	%6d\n", abs->minimum);
    printf("	Max	%6d\n", abs->maximum);
    if (abs->fuzz)
        printf("	Fuzz	%6d\n", abs->fuzz);
    if (abs->flat)
        printf("	Flat	%6d\n", abs->flat);
    if (abs->resolution)
        printf("	Resolution	%6d\n", abs->resolution);
}

static void
print_code_bits(struct libevdev* dev, unsigned int type, unsigned int max)
{
    unsigned int i;
    for (i = 0; i <= max; i++) {
        if (!libevdev_has_event_code(dev, type, i))
            continue;

        printf("    Event code %i (%s)\n", i, libevdev_event_code_get_name(type, i));
        if (type == EV_ABS)
            print_abs_bits(dev, i);
    }
}

static void
print_bits(struct libevdev* dev)
{
    unsigned int i;
    printf("Supported events:\n");

    for (i = 0; i <= EV_MAX; i++) {
        if (libevdev_has_event_type(dev, i))
            printf("  Event type %d (%s)\n", i, libevdev_event_type_get_name(i));
        switch (i) {
        case EV_KEY:
            print_code_bits(dev, EV_KEY, KEY_MAX);
            break;
        case EV_REL:
            print_code_bits(dev, EV_REL, REL_MAX);
            break;
        case EV_ABS:
            print_code_bits(dev, EV_ABS, ABS_MAX);
            break;
        case EV_LED:
            print_code_bits(dev, EV_LED, LED_MAX);
            break;
        }
    }
}

static void
print_props(struct libevdev* dev)
{
    unsigned int i;
    printf("Properties:\n");

    for (i = 0; i <= INPUT_PROP_MAX; i++) {
        if (libevdev_has_property(dev, i))
            printf("  Property type %d (%s)\n", i, libevdev_property_get_name(i));
    }
}

static int
print_event(struct input_event* ev)
{
    if (ev->type == EV_SYN)
        printf("Event: time %ld.%06ld, -------------- %s --------------n",
            ev->input_event_sec,
            ev->input_event_usec,
            libevdev_event_type_get_name(ev->type));
    else
        printf("Event: time %ld.%06ld, type %d (%s), code %d (%s), value %d\n",
            ev->input_event_sec,
            ev->input_event_usec,
            ev->type,
            libevdev_event_type_get_name(ev->type),
            ev->code,
            libevdev_event_code_get_name(ev->type, ev->code),
            ev->value);
    return 0;
}

static int
print_sync_event(struct input_event* ev)
{
    printf("SYNC: ");
    print_event(ev);
    return 0;
}

// 打开设备文件（可用的设备文件在/dev/inputs/event*，可以用sudo evtest查看）
static int open_restricted(const char* path, int flags)
{
    int fd = open(path, flags);
    printf("open device fd %d\n", fd);
    return fd < 0 ? -errno : fd;
}

// 关闭设备文件
static void close_restricted(int fd)
{
    printf("close device fd %d\n", fd);
    close(fd);
}

int main(int argc, char** argv)
{
    if (geteuid() == 0)
        printf("Run as root, euid:%ld\n", (long)geteuid());
    else {
        printf("Not Run as root, euid:%ld\n", (long)geteuid());
        return 0;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <device_file>\n", argv[0]);
        return 0;
    }

    struct libevdev* dev = NULL;
    const char* file;
    int fd;
    int rc = 1;

    file = argv[1];
    if ((fd = open_restricted(file, O_RDONLY)) < 0) {
        perror("Failed to open device");
        goto out;
    }

    rc = libevdev_new_from_fd(fd, &dev); // 创建一个libevdev实例，关联着fd
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        goto out;
    }
    // 关于%#格式控制符：https://stackoverflow.com/questions/3529777/do-you-know-about-x-in-c-language-format-string
    printf("Input device ID: bus %#x vendor %#x product %#x\n",
        libevdev_get_id_bustype(dev),
        libevdev_get_id_vendor(dev),
        libevdev_get_id_product(dev));
    printf("Evdev version: %x\n", libevdev_get_driver_version(dev));
    printf("Input device name: \"%s\"\n", libevdev_get_name(dev));
    printf("Phys location: %s\n", libevdev_get_phys(dev));
    printf("Uniq identifier: %s\n", libevdev_get_uniq(dev));
    print_bits(dev);
    print_props(dev);

    do {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &ev);
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            printf("::::::::::::::::::::: dropped ::::::::::::::::::::::\n");
            while (rc == LIBEVDEV_READ_STATUS_SYNC) {
                print_sync_event(&ev);
                rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &ev);
            }
            printf("::::::::::::::::::::: re-synced ::::::::::::::::::::::\n");
        } else if (rc == LIBEVDEV_READ_STATUS_SUCCESS)
            print_event(&ev);
    } while (rc == LIBEVDEV_READ_STATUS_SYNC || rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == -EAGAIN);

    if (rc != LIBEVDEV_READ_STATUS_SUCCESS && rc != -EAGAIN)
        fprintf(stderr, "Failed to handle events: %s\n", strerror(-rc));

    rc = 0;
out:
    libevdev_free(dev);
    close_restricted(fd);

    return rc;
}
