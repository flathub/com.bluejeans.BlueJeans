/*
 * Copyright (c) 2020 Bastien Nocera <hadess@hadess.net>
 *
 * This program is not free software, all rights reserved.
 *
 */

#include <pthread.h>
#include <dlfcn.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define __attribute__(v)
#include <libudev.h>

#if DEBUG > 0
#define D(x...) printf(x)
#else
#define D(x...)
#endif

static void *udev_symbol(const char *symbol)
{
	static void *libudev;
	void *sym;
	if (!libudev)
		libudev = dlopen("libudev.so.1.6.18", RTLD_LOCAL | RTLD_NOW);
	sym = dlsym(libudev, symbol);
	return sym;
}

const char *udev_device_get_property_value(struct udev_device *udev_device, const char *key)
{
	const char *ret;
	static const char *(*f)(struct udev_device *udev_device, const char *key) = 0;

        D("%s(udev_device, %s)\n", __func__, key);
	if (!f)
		f = udev_symbol(__func__);
	ret = (*f) (udev_device, key);
	D("property %s = %s\n", key, ret);
	/* override ID_V4L_PRODUCT */
	if (strcmp(key, "ID_V4L_PRODUCT") == 0) {
		static const char *(*g)(struct udev_device *udev_device, const char *sysattr) = 0;
		if (!g)
			g = udev_symbol("udev_device_get_sysattr_value");
		ret = (*g)(udev_device, "name");
		D("property %s overridden = %s\n", key, ret);
	}
	return ret;
}
