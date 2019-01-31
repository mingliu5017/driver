################################################################################
#
# bluez-alsa
#
################################################################################

#BLUEZ_ALSA_VERSION = c3a1c3520b1ec8bb9856492d26a73805dfa6b19c
#BLUEZ_ALSA_SITE = git://github.com/Arkq/bluez-alsa.git
BLUEZ_ALSA_VERSION = 1.2.0
BLUEZ_ALSA_SITE = $(TOPDIR)/../vendor/amlogic/bluez-alsa
BLUEZ_ALSA_SITE_METHOD = local
BLUEZ_ALSA_LICENSE = GPLv2+, GPLv2 (py-smbus)
BLUEZ_ALSA_LICENSE_FILES = COPYING
BLUEZ_ALSA_INSTALL_STAGING = YES
BLUEZ_ALSA_AUTORECONF = YES

BLUEZ_ALSA_DEPENDENCIES += bluez5_utils
BLUEZ_ALSA_DEPENDENCIES += alsa-lib
BLUEZ_ALSA_DEPENDENCIES += sbc
BLUEZ_ALSA_INSTALL_STAGING_OPTS = \
    prefix=$(STAGING_DIR)/usr \
    exec_prefix=$(STAGING_DIR)/usr \
    PKG_DEVLIB_DIR=$(STAGING_DIR)/usr/lib \
    install

BLUEZ_ALSA_INSTALL_TARGET_OPTS = \
    prefix=$(TARGET_DIR)/usr \
    exec_prefix=$(TARGET_DIR)/usr \
    install

define BLUEZ_ALSA_LIB_INSTALL_CMD
	mkdir -p $(TARGET_DIR)/usr/lib/alsa-lib
	$(INSTALL) -D -m 0755 $(STAGING_DIR)/usr/lib/alsa-lib/*.so $(TARGET_DIR)/usr/lib/alsa-lib

endef
BLUEZ_ALSA_CONF_OPTS = --disable-payloadcheck --prefix=$(TARGET_DIR)/usr
BLUEZ_ALSA_POST_INSTALL_TARGET_HOOKS += BLUEZ_ALSA_LIB_INSTALL_CMD

$(eval $(autotools-package))
