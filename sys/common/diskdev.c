#include <sys/diskdev.h>
#include <sys/disk.h>
#include <sys/bdev.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/major.h>

#include <stdlib.h>
#include <stdio.h>

static struct disk* diskdev_lookup(dev_t dev) {
	struct disk*   disk;
	struct disk*   view;
	struct device* device;

	device = bdev_lookup_device(dev);
	if(device == NULL) return NULL;

	disk = (struct disk*)device->softc;

	if(disk->dev == dev) return disk;

	for(view = disk->partitions; view != NULL; view = view->next) {
		if(view->dev == dev) return view;
	}

	return NULL;
}

static int diskdev_open(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_open(disk, flags, mode);
}

static int diskdev_close(dev_t dev, int flags, int mode) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_close(disk, flags, mode);
}

static void diskdev_strategy(struct buf* buf) {
	struct disk* disk;

	buf->error = 0;
	buf->resid = buf->bcount;
	buf->flags &= ~(BUF_DONE | BUF_ERROR);

	disk = diskdev_lookup(buf->dev);
	if(disk == NULL) {
		buf->error = ENXIO;
		buf->flags |= BUF_ERROR | BUF_DONE;
		return;
	}

	return disk_strategy(disk, buf);
}

static int diskdev_ioctl(dev_t dev, unsigned long cmd, void* data, int flags) {
	struct disk* disk;

	disk = diskdev_lookup(dev);
	if(disk == NULL) return ENXIO;

	return disk_ioctl(disk, cmd, data, flags);
}

struct bdevsw disk_bdevsw = {
    diskdev_open,
    diskdev_close,
    diskdev_strategy,
    diskdev_ioctl};

dev_t diskdev_init(struct device* device, const char* name, const struct disk_ops* ops, unsigned int nsectors) {
	struct disk* disk = device->softc;

	disk_init(disk, ops);

	disk->device   = device;
	disk->nsectors = nsectors;

	mutex_init(&disk->lock);

	if((disk->dev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_DISK, DEVICE_MINOR_ANY), device, name)) == DEVICE_ERROR) return DEVICE_ERROR;

	diskdev_try_mbr(disk->dev);

	return disk->dev;
}

dev_t diskdev_partition_init(struct disk* disk, const char* name, unsigned int start, unsigned int nsectors) {
	struct disk* view;
	dev_t	     dev = DEVICE_ERROR;
	char	     devname[33];
	char	     alpha;

	if(nsectors == 0 || start > disk->nsectors || nsectors > disk->nsectors - start) return DEVICE_ERROR;

	view = malloc(sizeof(*view));
	if(view == NULL) return DEVICE_ERROR;

	disk_init(view, disk->ops);

	view->device   = disk->device;
	view->parent   = disk;
	view->start    = start;
	view->nsectors = nsectors;

	mutex_init(&view->lock);

	if(name == NULL) {
		char* base = bdev_device_name(disk->dev);

		for(alpha = 'a'; alpha <= 'z'; alpha++) {
			dev_t d;

			snprintf(devname, sizeof(devname), "%s%c", base, alpha);

			if(bdev_find_device(devname, &d) == 0) continue;

			break;
		}

		if(alpha > 'z') {
			free(view);
			return DEVICE_ERROR;
		}
	} else {
		snprintf(devname, sizeof(devname), "%s", name);
	}

	if((dev = bdev_register_device(DEVICE_MAKE(BDEV_MAJOR_DISK, DEVICE_MINOR_ANY), disk->device, devname)) == DEVICE_ERROR) {
		free(view);
		return DEVICE_ERROR;
	}

	view->dev	 = dev;
	view->next	 = disk->partitions;
	disk->partitions = view;

	return dev;
}

void diskdev_partition_detach(dev_t dev) {
	struct device* device;
	struct disk*   disk;
	struct disk**  link;
	struct disk*   view;

	device = bdev_lookup_device(dev);
	if(device == NULL) return;

	disk = (struct disk*)device->softc;

	for(link = &disk->partitions; (view = *link) != NULL; link = &view->next) {
		if(view->dev == dev) {
			*link = view->next;

			bdev_unregister_device(dev);

			free(view);

			return;
		}
	}
}

void diskdev_partition_reset(struct disk* disk) {
	struct disk* view;
	struct disk* next;

	for(view = disk->partitions; view != NULL; view = next) {
		next = view->next;

		diskdev_partition_detach(view->dev);
	}
}

void diskdev_detach(struct disk* disk) {
	diskdev_partition_reset(disk);

	bdev_unregister_device(disk->dev);
}
