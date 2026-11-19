#include <sys/device.h>

static int mainbus_match(struct device* parent, struct device_attach_args* args) {
	return 1;
}

static int mainbus_attach(struct device* parent, struct device* self, struct device_attach_args* args) {
	return 0;
}

static void mainbus_detach(struct device* self) {
}

struct device_driver mainbus_driver = {
    "mainbus",
    mainbus_match,
    mainbus_attach,
    mainbus_detach};
