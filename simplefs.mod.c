#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x27e56cd1, "new_inode" },
	{ 0x86c49e96, "nop_mnt_idmap" },
	{ 0x3b633d26, "inode_init_owner" },
	{ 0xcd08e937, "d_instantiate" },
	{ 0xd0b447cb, "__bread_gfp" },
	{ 0x7f03a8b7, "__brelse" },
	{ 0xa67636e9, "unregister_filesystem" },
	{ 0x7f03a8b7, "mark_buffer_dirty" },
	{ 0xe4b08d52, "sync_dirty_buffer" },
	{ 0x85220fe2, "blkdev_issue_flush" },
	{ 0x06e4b308, "d_make_root" },
	{ 0x982b44aa, "iput" },
	{ 0xc7001561, "kill_block_super" },
	{ 0xee420b08, "simple_lookup" },
	{ 0xd93291a3, "generic_read_dir" },
	{ 0x4805cbc6, "generic_file_read_iter" },
	{ 0x5f076770, "generic_file_write_iter" },
	{ 0x58dcc1b6, "generic_file_mmap" },
	{ 0x4c312f4d, "generic_file_fsync" },
	{ 0xa4edb905, "param_ops_int" },
	{ 0xa4edb905, "param_ops_charp" },
	{ 0xd272d446, "__fentry__" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x5a844b26, "__x86_indirect_thunk_rax" },
	{ 0xe8213e80, "_printk" },
	{ 0xa67636e9, "register_filesystem" },
	{ 0xc067eb5b, "mount_bdev" },
	{ 0xc773217c, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x27e56cd1,
	0x86c49e96,
	0x3b633d26,
	0xcd08e937,
	0xd0b447cb,
	0x7f03a8b7,
	0xa67636e9,
	0x7f03a8b7,
	0xe4b08d52,
	0x85220fe2,
	0x06e4b308,
	0x982b44aa,
	0xc7001561,
	0xee420b08,
	0xd93291a3,
	0x4805cbc6,
	0x5f076770,
	0x58dcc1b6,
	0x4c312f4d,
	0xa4edb905,
	0xa4edb905,
	0xd272d446,
	0xd272d446,
	0x5a844b26,
	0xe8213e80,
	0xa67636e9,
	0xc067eb5b,
	0xc773217c,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"new_inode\0"
	"nop_mnt_idmap\0"
	"inode_init_owner\0"
	"d_instantiate\0"
	"__bread_gfp\0"
	"__brelse\0"
	"unregister_filesystem\0"
	"mark_buffer_dirty\0"
	"sync_dirty_buffer\0"
	"blkdev_issue_flush\0"
	"d_make_root\0"
	"iput\0"
	"kill_block_super\0"
	"simple_lookup\0"
	"generic_read_dir\0"
	"generic_file_read_iter\0"
	"generic_file_write_iter\0"
	"generic_file_mmap\0"
	"generic_file_fsync\0"
	"param_ops_int\0"
	"param_ops_charp\0"
	"__fentry__\0"
	"__x86_return_thunk\0"
	"__x86_indirect_thunk_rax\0"
	"_printk\0"
	"register_filesystem\0"
	"mount_bdev\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "2A453070213955CB04531EF");
