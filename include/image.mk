#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

override TARGET_BUILD=
include $(INCLUDE_DIR)/prereq.mk
include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/host.mk

.NOTPARALLEL:
override MAKEFLAGS=
override MAKE:=$(SUBMAKE)
KDIR=$(KERNEL_BUILD_DIR)
DTS_DIR:=$(LINUX_DIR)/arch/$(ARCH)/boot/dts/

IMG_PREFIX:=openwrt-$(BOARD)$(if $(SUBTARGET),-$(SUBTARGET))

MKFS_DEVTABLE_OPT := -D $(INCLUDE_DIR)/device_table.txt

ifneq ($(CONFIG_BIG_ENDIAN),)
  JFFS2OPTS     :=  --big-endian --squash-uids -v
else
  JFFS2OPTS     :=  --little-endian --squash-uids -v
endif

ifeq ($(CONFIG_JFFS2_RTIME),y)
  JFFS2OPTS += -X rtime
endif
ifeq ($(CONFIG_JFFS2_ZLIB),y)
  JFFS2OPTS += -X zlib
endif
ifeq ($(CONFIG_JFFS2_LZMA),y)
  JFFS2OPTS += -X lzma --compression-mode=size
endif
ifneq ($(CONFIG_JFFS2_RTIME),y)
  JFFS2OPTS += -x rtime
endif
ifneq ($(CONFIG_JFFS2_ZLIB),y)
  JFFS2OPTS += -x zlib
endif
ifneq ($(CONFIG_JFFS2_LZMA),y)
  JFFS2OPTS += -x lzma
endif

JFFS2OPTS += $(MKFS_DEVTABLE_OPT)

SQUASHFS_BLOCKSIZE := $(CONFIG_TARGET_SQUASHFS_BLOCK_SIZE)k
SQUASHFSOPT := -b $(SQUASHFS_BLOCKSIZE)
SQUASHFSOPT += -p '/dev d 755 0 0' -p '/dev/console c 600 0 0 5 1'
SQUASHFSCOMP := gzip
#LZMA_XZ_OPTIONS := -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2
ifeq ($(CONFIG_SQUASHFS_LZMA),y)
  SQUASHFSCOMP := lzma $(LZMA_XZ_OPTIONS)
endif
ifeq ($(CONFIG_SQUASHFS_XZ),y)
  ifneq ($(filter arm x86 powerpc sparc,$(LINUX_KARCH)),)
    BCJ_FILTER:=-Xbcj $(LINUX_KARCH)
  endif
  SQUASHFSCOMP := xz $(LZMA_XZ_OPTIONS) $(BCJ_FILTER)
endif

JFFS2_BLOCKSIZE ?= 64k 128k

define add_jffs2_mark
	echo -ne '\xde\xad\xc0\xde' >> $(1)
endef

define toupper
$(shell echo $(1) | tr '[:lower:]' '[:upper:]')
endef

# pad to 4k, 8k, 16k, 64k, 128k, 256k and add jffs2 end-of-filesystem mark
define prepare_generic_squashfs
	$(STAGING_DIR_HOST)/bin/padjffs2 $(1) 4 8 16 64 128 256
endef

define Image/BuildKernel/Initramfs
	cp $(KDIR)/vmlinux-initramfs.elf $(BIN_DIR)/$(IMG_PREFIX)-vmlinux-initramfs.elf
	$(call Image/Build/Initramfs)
endef

define Image/BuildKernel/MkuImage
	mkimage -A $(ARCH) -O linux -T kernel -C $(1) -a $(2) -e $(3) \
		-n '$(call toupper,$(ARCH)) OpenWrt Linux-$(LINUX_VERSION)' -d $(4) $(5)
endef

define Image/BuildKernel/MkFIT
	$(TOPDIR)/scripts/mkits.sh \
		-D $(1) -o $(KDIR)/fit-$(1).its -k $(2) -d $(3) -C $(4) -a $(5) -e $(6) \
		-A $(ARCH) -v $(LINUX_VERSION)
	PATH=$(LINUX_DIR)/scripts/dtc:$(PATH) mkimage -f $(KDIR)/fit-$(1).its $(KDIR)/fit-$(1)$(7).itb
endef

define Image/mkfs/jffs2/sub
		# FIXME: removing this line will cause strange behaviour in the foreach loop below
		$(STAGING_DIR_HOST)/bin/mkfs.jffs2 $(3) --pad -e $(patsubst %k,%KiB,$(1)) -o $(KDIR)/root.jffs2-$(2) -d $(TARGET_DIR) -v 2>&1 1>/dev/null | awk '/^.+$$$$/'
		$(STAGING_DIR_HOST)/bin/mkfs.jffs2 $(3) -e $(patsubst %k,%KiB,$(1)) -o $(KDIR)/root.jffs2-$(2)-raw -d $(TARGET_DIR) -v 2>&1 1>/dev/null | awk '/^.+$$$$/'
		$(call add_jffs2_mark,$(KDIR)/root.jffs2-$(2))
		$(call Image/Build,jffs2-$(2))
endef

ifneq ($(CONFIG_TARGET_ROOTFS_JFFS2),)
    define Image/mkfs/jffs2
		$(foreach SZ,$(JFFS2_BLOCKSIZE),$(call Image/mkfs/jffs2/sub,$(SZ),$(SZ),$(JFFS2OPTS)))
    endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_JFFS2_NAND),)
    define Image/mkfs/jffs2_nand
		$(foreach SZ,$(NAND_BLOCKSIZE), $(call Image/mkfs/jffs2/sub, \
			$(word 2,$(subst :, ,$(SZ))),nand-$(subst :,-,$(SZ)), \
			$(JFFS2OPTS) --no-cleanmarkers --pagesize=$(word 1,$(subst :, ,$(SZ)))) \
		)
    endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_SQUASHFS),)
    define Image/mkfs/squashfs
		@mkdir -p $(TARGET_DIR)/overlay
		$(STAGING_DIR_HOST)/bin/mksquashfs4 $(TARGET_DIR) $(KDIR)/root.squashfs -noappend -root-owned -comp xz
		$(call Image/Build,squashfs)
    endef
endif

# $(1): board name
# $(2): rootfs type
# $(3): kernel image
ifneq ($(CONFIG_NAND_SUPPORT),)
   define Image/Build/SysupgradeNAND
	mkdir -p "$(KDIR_TMP)/sysupgrade-$(1)/"
	echo "BOARD=$(1)" > "$(KDIR_TMP)/sysupgrade-$(1)/CONTROL"
	[ -z "$(2)" ] || $(CP) "$(KDIR)/root.$(2)" "$(KDIR_TMP)/sysupgrade-$(1)/root"
	[ -z "$(3)" ] || $(CP) "$(3)" "$(KDIR_TMP)/sysupgrade-$(1)/kernel"
	(cd "$(KDIR_TMP)"; $(TAR) cvf \
		"$(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(2)-sysupgrade.tar" sysupgrade-$(1))
   endef
# $(1) board name
# $(2) ubinize-image options (e.g. --uboot-env and/or --kernel kernelimage)
# $(3) rootfstype (e.g. squashfs or ubifs)
# $(4) options to pass-through to ubinize (i.e. $($(PROFILE)_UBI_OPTS)))
   define Image/Build/UbinizeImage
	sh $(TOPDIR)/scripts/ubinize-image.sh $(2) \
		"$(KDIR)/root.$(3)" \
		"$(BIN_DIR)/$(IMG_PREFIX)-$(1)-$(3)-ubinized.bin" \
		$(4)
   endef

endif

ifneq ($(CONFIG_TARGET_ROOTFS_UBIFS),)
    define Image/mkfs/ubifs/generate
	$(CP) ./ubinize$(1).cfg $(KDIR)
	( cd $(KDIR); \
		$(STAGING_DIR_HOST)/bin/ubinize \
		$(if $($(PROFILE)_UBI_OPTS), \
			$(shell echo $($(PROFILE)_UBI_OPTS)), \
			$(shell echo $(UBI_OPTS)) \
		) \
		-o $(KDIR)/root$(1).ubi \
		ubinize$(1).cfg \
	)
    endef

    define Image/mkfs/ubifs

        ifneq ($($(PROFILE)_UBIFS_OPTS)$(UBIFS_OPTS),)
		$(STAGING_DIR_HOST)/bin/mkfs.ubifs \
			$(if $($(PROFILE)_UBIFS_OPTS), \
				$(shell echo $($(PROFILE)_UBIFS_OPTS)), \
				$(shell echo $(UBIFS_OPTS)) \
			) \
			$(if $(CONFIG_TARGET_UBIFS_FREE_SPACE_FIXUP),--space-fixup) \
			$(if $(CONFIG_TARGET_UBIFS_COMPRESSION_NONE),--force-compr=none) \
			$(if $(CONFIG_TARGET_UBIFS_COMPRESSION_LZO),--force-compr=lzo) \
			$(if $(CONFIG_TARGET_UBIFS_COMPRESSION_ZLIB),--force-compr=zlib) \
			$(if $(shell echo $(CONFIG_TARGET_UBIFS_JOURNAL_SIZE)),--jrn-size=$(CONFIG_TARGET_UBIFS_JOURNAL_SIZE)) \
			--squash-uids \
			-o $(KDIR)/root.ubifs \
			-d $(TARGET_DIR)
        endif
	$(call Image/Build,ubifs)

        ifneq ($($(PROFILE)_UBI_OPTS)$(UBI_OPTS),)
		$(call Image/mkfs/ubifs/generate,)
		$(if $(wildcard ./ubinize-overlay.cfg),$(call Image/mkfs/ubifs/generate,-overlay))
        endif
	$(call Image/Build,ubi)
    endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_CPIOGZ),)
  define Image/mkfs/cpiogz
		( cd $(TARGET_DIR); find . | cpio -o -H newc | gzip -9 >$(BIN_DIR)/$(IMG_PREFIX)-rootfs.cpio.gz )
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_TARGZ),)
  define Image/mkfs/targz
		# Preserve permissions (-p) when building as non-root user
		$(TAR) -czpf $(BIN_DIR)/$(IMG_PREFIX)$(if $(PROFILE),-$(PROFILE))-rootfs.tar.gz --numeric-owner --owner=0 --group=0 -C $(TARGET_DIR)/ .
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_EXT4FS),)
  E2SIZE=$(shell echo $$(($(CONFIG_TARGET_ROOTFS_PARTSIZE)*1024*1024/$(CONFIG_TARGET_EXT4_BLOCKSIZE))))

  define Image/mkfs/ext4
# generate an ext2 fs
	$(STAGING_DIR_HOST)/bin/genext2fs -U -B $(CONFIG_TARGET_EXT4_BLOCKSIZE) -b $(E2SIZE) -N $(CONFIG_TARGET_EXT4_MAXINODE) -d $(TARGET_DIR)/ $(KDIR)/root.ext4 -m $(CONFIG_TARGET_EXT4_RESERVED_PCT) $(MKFS_DEVTABLE_OPT)
# convert it to ext4
	$(STAGING_DIR_HOST)/bin/tune2fs $(if $(CONFIG_TARGET_EXT4_JOURNAL),-j) -O extents,uninit_bg,dir_index $(KDIR)/root.ext4
# fix it up
	$(STAGING_DIR_HOST)/bin/e2fsck -fy $(KDIR)/root.ext4
	$(call Image/Build,ext4)
  endef
endif

ifneq ($(CONFIG_TARGET_ROOTFS_ISO),)
  define Image/mkfs/iso
		$(call Image/Build,iso)
  endef
endif


define Image/mkfs/prepare/default
	# Use symbolic permissions to avoid clobbering SUID/SGID/sticky bits
	- $(FIND) $(TARGET_DIR) -type f -not -perm +0100 -not -name 'ssh_host*' -not -name 'shadow' -print0 | $(XARGS) -0 chmod u+rw,g+r,o+r
	- $(FIND) $(TARGET_DIR) -type f -perm +0100 -print0 | $(XARGS) -0 chmod u+rwx,g+rx,o+rx
	- $(FIND) $(TARGET_DIR) -type d -print0 | $(XARGS) -0 chmod u+rwx,g+rx,o+rx
	$(INSTALL_DIR) $(TARGET_DIR)/tmp
	chmod 1777 $(TARGET_DIR)/tmp
endef

define Image/mkfs/prepare
	$(call Image/mkfs/prepare/default)
endef


define Image/Checksum
	( cd ${BIN_DIR} ; \
		$(FIND) -maxdepth 1 -type f \! -name 'md5sums'  -printf "%P\n" | sort | xargs \
		md5sum --binary > md5sums \
	)
endef


define BuildImage

  download:
  prepare:

  ifeq ($(IB),)
    compile: compile-targets FORCE
		$(call Build/Compile)
  else
    compile:
  endif

  ifeq ($(IB),)
    install: compile install-targets FORCE
		$(call Image/Prepare)
		$(call Image/mkfs/prepare)
		$(call Image/BuildKernel)
		$(if $(CONFIG_TARGET_ROOTFS_INITRAMFS),$(call Image/BuildKernel/Initramfs))
		$(call Image/InstallKernel)
		$(call Image/mkfs/cpiogz)
		$(call Image/mkfs/targz)
		$(call Image/mkfs/ext4)
		$(call Image/mkfs/iso)
		$(call Image/mkfs/jffs2)
		$(call Image/mkfs/jffs2_nand)
		$(call Image/mkfs/squashfs)
		$(call Image/mkfs/ubifs)
		$(call Image/Checksum)
  else
    install: compile install-targets
		$(call Image/BuildKernel)
		$(if $(CONFIG_TARGET_ROOTFS_INITRAMFS),$(call Image/BuildKernel/Initramfs))
		$(call Image/InstallKernel)
		$(call Image/mkfs/cpiogz)
		$(call Image/mkfs/targz)
		$(call Image/mkfs/ext4)
		$(call Image/mkfs/iso)
		$(call Image/mkfs/jffs2)
		$(call Image/mkfs/jffs2_nand)
		$(call Image/mkfs/squashfs)
		$(call Image/mkfs/ubifs)
		$(call Image/Checksum)
  endif

  ifeq ($(IB),)
    clean: clean-targets
		$(call Build/Clean)
  else
    clean:
  endif

  compile-targets:
  install-targets:
  clean-targets:

endef
