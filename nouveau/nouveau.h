#ifndef __NOUVEAU_H__
#define __NOUVEAU_H__

#include <stdint.h>
#include <stdbool.h>

#define NOUVEAU_DEVICE_CLASS       0x80000000
#define NOUVEAU_FIFO_CHANNEL_CLASS 0x80000001
#define NOUVEAU_NOTIFIER_CLASS     0x80000002
#define NOUVEAU_PARENT_CLASS       0xffffffff

struct nouveau_list {
	struct nouveau_list *prev;
	struct nouveau_list *next;
};

struct nouveau_object {
	struct nouveau_object *parent;
	uint64_t handle;
	uint32_t oclass;
	uint32_t length;
	void *data;
};

struct nouveau_fifo {
	struct nouveau_object *object;
	uint32_t channel;
	uint32_t pushbuf;
	uint64_t unused1[3];
};

struct nv04_fifo {
	struct nouveau_fifo base;
	uint32_t vram;
	uint32_t gart;
	uint32_t notify;
};

struct nvc0_fifo {
	struct nouveau_fifo base;
	uint32_t notify;
};

struct nv04_notify {
	struct nouveau_object *object;
	uint32_t offset;
	uint32_t length;
};

int  nouveau_object_new(struct nouveau_object *parent, uint64_t handle,
			uint32_t oclass, void *data, uint32_t length,
			struct nouveau_object **);
void nouveau_object_del(struct nouveau_object **);
void *nouveau_object_find(struct nouveau_object *, uint32_t parent_class);

struct nouveau_device {
	struct nouveau_object object;
	int fd;
	uint32_t lib_version;
	uint32_t drm_version;
	uint32_t chipset;
	uint64_t vram_size;
	uint64_t gart_size;
	uint64_t vram_limit;
	uint64_t gart_limit;
};

int  nouveau_device_wrap(int fd, int close, struct nouveau_device **);
int  nouveau_device_open(const char *busid, struct nouveau_device **);
void nouveau_device_del(struct nouveau_device **);
int  nouveau_getparam(struct nouveau_device *, uint64_t param, uint64_t *value);
int  nouveau_setparam(struct nouveau_device *, uint64_t param, uint64_t value);

struct nouveau_client {
	struct nouveau_device *device;
	int id;
};

int  nouveau_client_new(struct nouveau_device *, struct nouveau_client **);
void nouveau_client_del(struct nouveau_client **);

union nouveau_bo_config {
	struct {
#define NV04_BO_16BPP 0x00000001
#define NV04_BO_32BPP 0x00000002
#define NV04_BO_ZETA  0x00000004
		uint32_t surf_flags;
		uint32_t surf_pitch;
	} nv04;
	struct {
		uint32_t memtype;
		uint32_t tile_mode;
	} nv50;
	struct {
		uint32_t memtype;
		uint32_t tile_mode;
	} nvc0;
	uint32_t data[8];
};

#define NOUVEAU_BO_VRAM    0x00000001
#define NOUVEAU_BO_GART    0x00000002
#define NOUVEAU_BO_APER   (NOUVEAU_BO_VRAM | NOUVEAU_BO_GART)
#define NOUVEAU_BO_RD      0x00000100
#define NOUVEAU_BO_WR      0x00000200
#define NOUVEAU_BO_RDWR   (NOUVEAU_BO_RD | NOUVEAU_BO_WR)
#define NOUVEAU_BO_NOBLOCK 0x00000400
#define NOUVEAU_BO_LOW     0x00001000
#define NOUVEAU_BO_HIGH    0x00002000
#define NOUVEAU_BO_OR      0x00004000
#define NOUVEAU_BO_MAP     0x80000000
#define NOUVEAU_BO_CONTIG  0x40000000
#define NOUVEAU_BO_NOSNOOP 0x20000000

struct nouveau_bo {
	struct nouveau_device *device;
	uint32_t handle;
	uint64_t size;
	uint32_t flags;
	uint64_t offset;
	void *map;
	union nouveau_bo_config config;
};

int  nouveau_bo_new(struct nouveau_device *, uint32_t flags, uint32_t align,
		    uint64_t size, union nouveau_bo_config *,
		    struct nouveau_bo **);
int  nouveau_bo_wrap(struct nouveau_device *, uint32_t handle,
		     struct nouveau_bo **);
int  nouveau_bo_name_ref(struct nouveau_device *dev, uint32_t name,
			 struct nouveau_bo **);
int  nouveau_bo_name_get(struct nouveau_bo *, uint32_t *name);
void nouveau_bo_ref(struct nouveau_bo *, struct nouveau_bo **);
int  nouveau_bo_map(struct nouveau_bo *, uint32_t access,
		    struct nouveau_client *);
int  nouveau_bo_wait(struct nouveau_bo *, uint32_t access,
		     struct nouveau_client *);

struct nouveau_bufref {
	struct nouveau_list thead;
	struct nouveau_bo *bo;
	uint32_t packet;
	uint32_t flags;
	uint32_t data;
	uint32_t vor;
	uint32_t tor;
	uint32_t priv_data;
	void *priv;
};

struct nouveau_bufctx {
	struct nouveau_client *client;
	struct nouveau_list head;
	struct nouveau_list pending;
	struct nouveau_list current;
	int relocs;
};

int  nouveau_bufctx_new(struct nouveau_client *, int bins,
			struct nouveau_bufctx **);
void nouveau_bufctx_del(struct nouveau_bufctx **);
struct nouveau_bufref *
nouveau_bufctx_refn(struct nouveau_bufctx *, int bin,
		    struct nouveau_bo *, uint32_t flags);
struct nouveau_bufref *
nouveau_bufctx_mthd(struct nouveau_bufctx *, int bin,  uint32_t packet,
		    struct nouveau_bo *, uint64_t data, uint32_t flags,
		    uint32_t vor, uint32_t tor);
void nouveau_bufctx_reset(struct nouveau_bufctx *, int bin);

struct nouveau_pushbuf_krec;
struct nouveau_pushbuf {
	struct nouveau_client *client;
	struct nouveau_object *channel;
	struct nouveau_bufctx *bufctx;
	void (*kick_notify)(struct nouveau_pushbuf *);
	void *user_priv;
	uint32_t rsvd_kick;
	uint32_t flags;
	uint32_t *cur;
	uint32_t *end;
};

struct nouveau_pushbuf_refn {
	struct nouveau_bo *bo;
	uint32_t flags;
};

int  nouveau_pushbuf_new(struct nouveau_client *, struct nouveau_object *channel,
			 int nr, uint32_t size, bool immediate,
			 struct nouveau_pushbuf **);
void nouveau_pushbuf_del(struct nouveau_pushbuf **);
int  nouveau_pushbuf_space(struct nouveau_pushbuf *, uint32_t dwords,
			   uint32_t relocs, uint32_t pushes);
void nouveau_pushbuf_data(struct nouveau_pushbuf *, struct nouveau_bo *,
			  uint64_t offset, uint64_t length);
int  nouveau_pushbuf_refn(struct nouveau_pushbuf *,
			  struct nouveau_pushbuf_refn *, int nr);
/* Emits a reloc into the push buffer at the current position, you *must*
 * have previously added the referenced buffer to a buffer context, and
 * validated it against the current push buffer.
 */
void nouveau_pushbuf_reloc(struct nouveau_pushbuf *, struct nouveau_bo *,
			   uint32_t data, uint32_t flags,
			   uint32_t vor, uint32_t tor);
int  nouveau_pushbuf_validate(struct nouveau_pushbuf *);
uint32_t nouveau_pushbuf_refd(struct nouveau_pushbuf *, struct nouveau_bo *);
int  nouveau_pushbuf_kick(struct nouveau_pushbuf *, struct nouveau_object *channel);
struct nouveau_bufctx *
nouveau_pushbuf_bufctx(struct nouveau_pushbuf *, struct nouveau_bufctx *);

#endif