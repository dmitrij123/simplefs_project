#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/module.h>
#include <linux/dcache.h>
#include <linux/init.h>
#include <linux/blkdev.h>

// Параметры модуля
static char *disk_name = NULL;
module_param(disk_name, charp, 0644);
MODULE_PARM_DESC(disk_name, "Name of the disk device");

static int sb_first_offset = 0;
module_param(sb_first_offset, int, 0644);
MODULE_PARM_DESC(sb_first_offset, "First superblock offset in sectors");

static int sb_second_offset = 1024;
module_param(sb_second_offset, int, 0644);
MODULE_PARM_DESC(sb_second_offset, "Second superblock offset in sectors");

static int max_filename_len = 256;
module_param(max_filename_len, int, 0644);
MODULE_PARM_DESC(max_filename_len, "Maximum filename length");

static int max_file_size_sectors = 10;
module_param(max_file_size_sectors, int, 0644);
MODULE_PARM_DESC(max_file_size_sectors, "Maximum file size in sectors");

// Глобальные переменные
static struct super_block *simplefs_sb;
static struct block_device *simplefs_bdev;
static int file_count;

// Прототипы функций
static int simplefs_readdir(struct file *file, struct dir_context *ctx);
static int simplefs_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl);
static int simplefs_fill_super(struct super_block *sb, void *data, int silent);
static struct dentry *simplefs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

// Структура суперблока
struct simplefs_superblock {
    __u32 magic;                  // Магическое число (0x5F5F5F5F)
    __u32 hash;                   // Хеш для проверки целостности
    __u32 file_count;             // Количество файлов
    __u32 max_filename_len;       // Максимальная длина имени файла
    __u32 max_file_size_sectors;  // Максимальный размер файла в секторах
    char reserved[512 - 20];      // Заполняем до размера сектора (512 байт)
};

// Структура inode для хранения файлов
struct simplefs_inode {
    __u32 i_ino;                  // Номер inode
    __u32 i_mode;                 // Режим (права доступа и тип)
    __u32 i_size;                 // Размер файла в байтах
    __u32 i_blocks;               // Количество блоков
    __u32 i_block[10];            // Указатели на блоки данных (прямые)
};

// Вычисление хеша
static __u32 calculate_hash(void *data, size_t len) {
    __u32 hash = 0;
    unsigned char *ptr = data;
    for (size_t i = 0; i < len; i++) {
        hash += ptr[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    return hash;
}

// Операции для файлов
static const struct file_operations simplefs_file_operations = {
    .read_iter = generic_file_read_iter,
    .write_iter = generic_file_write_iter,
    .mmap = generic_file_mmap,
    .fsync = generic_file_fsync,
};

// Операции для каталогов
static const struct file_operations simplefs_dir_operations = {
    .read = generic_read_dir,
    .iterate_shared = simplefs_readdir,
};

// Операции с inode
static const struct inode_operations simplefs_inode_ops = {
    .lookup = simple_lookup,
    .create = simplefs_create,
};

// Форматирование диска
static int simplefs_format_disk(struct super_block *sb) {
    struct buffer_head *bh;
    struct simplefs_superblock *sfs_sb;
    loff_t size = get_capacity(sb->s_bdev->bd_disk) * 512; // Размер устройства в байтах

    pr_info("Device size: %lld bytes\n", size);

    pr_info("Formatting superblock at offset %d\n", sb_first_offset);
    bh = sb_bread(sb, sb_first_offset);
    if (!bh) {
        pr_err("Failed to read buffer at offset %d\n", sb_first_offset);
        return -EIO;
    }
    sfs_sb = (struct simplefs_superblock *)bh->b_data;
    memset(bh->b_data, 0, 512); // Очистка буфера
    sfs_sb->magic = 0x5F5F5F5F;
    sfs_sb->file_count = 5;
    sfs_sb->max_filename_len = max_filename_len;
    sfs_sb->max_file_size_sectors = max_file_size_sectors;
    sfs_sb->hash = 0; // Обнуляем хеш перед вычислением
    __u32 computed_hash = calculate_hash(sfs_sb, sizeof(struct simplefs_superblock) - sizeof(__u32));
    sfs_sb->hash = computed_hash;
    pr_info("Calculated hash for offset %d: %u (size: %zu bytes)\n", sb_first_offset, sfs_sb->hash, sizeof(struct simplefs_superblock) - sizeof(__u32));
    pr_info("Superblock before write: first 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            ((unsigned char *)sfs_sb)[0], ((unsigned char *)sfs_sb)[1],
            ((unsigned char *)sfs_sb)[2], ((unsigned char *)sfs_sb)[3],
            ((unsigned char *)sfs_sb)[4], ((unsigned char *)sfs_sb)[5],
            ((unsigned char *)sfs_sb)[6], ((unsigned char *)sfs_sb)[7],
            ((unsigned char *)sfs_sb)[8], ((unsigned char *)sfs_sb)[9],
            ((unsigned char *)sfs_sb)[10], ((unsigned char *)sfs_sb)[11],
            ((unsigned char *)sfs_sb)[12], ((unsigned char *)sfs_sb)[13],
            ((unsigned char *)sfs_sb)[14], ((unsigned char *)sfs_sb)[15]);
    pr_info("Superblock before write: magic=0x%X, file_count=%d, max_filename_len=%d, max_file_size_sectors=%d, hash=%u\n",
            sfs_sb->magic, sfs_sb->file_count, sfs_sb->max_filename_len, sfs_sb->max_file_size_sectors, sfs_sb->hash);
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    if (sb_second_offset * 512 >= size) {
        pr_err("Second superblock offset %d exceeds device size (%lld bytes)\n", sb_second_offset * 512, size);
        return -ENOSPC;
    }

    pr_info("Formatting superblock at offset %d\n", sb_second_offset);
    bh = sb_bread(sb, sb_second_offset);
    if (!bh) {
        pr_err("Failed to read buffer at offset %d\n", sb_second_offset);
        return -EIO;
    }
    memset(bh->b_data, 0, 512); // Очистка буфера
    memcpy(bh->b_data, sfs_sb, 512);
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    blkdev_issue_flush(sb->s_bdev);

    return 0;
}

// Чтение суперблока
static int simplefs_read_superblock(struct super_block *sb, int offset) {
    struct buffer_head *bh;
    struct simplefs_superblock *sfs_sb;

    bh = sb_bread(sb, offset);
    if (!bh) {
        pr_err("Failed to read superblock at offset %d\n", offset);
        return -EIO;
    }

    sfs_sb = (struct simplefs_superblock *)bh->b_data;
    if (sfs_sb->magic != 0x5F5F5F5F) {
        pr_err("Invalid magic number at offset %d: 0x%X\n", offset, sfs_sb->magic);
        brelse(bh);
        return -EINVAL;
    }

    pr_info("Superblock at offset %d: first 16 bytes: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            offset,
            ((unsigned char *)sfs_sb)[0], ((unsigned char *)sfs_sb)[1],
            ((unsigned char *)sfs_sb)[2], ((unsigned char *)sfs_sb)[3],
            ((unsigned char *)sfs_sb)[4], ((unsigned char *)sfs_sb)[5],
            ((unsigned char *)sfs_sb)[6], ((unsigned char *)sfs_sb)[7],
            ((unsigned char *)sfs_sb)[8], ((unsigned char *)sfs_sb)[9],
            ((unsigned char *)sfs_sb)[10], ((unsigned char *)sfs_sb)[11],
            ((unsigned char *)sfs_sb)[12], ((unsigned char *)sfs_sb)[13],
            ((unsigned char *)sfs_sb)[14], ((unsigned char *)sfs_sb)[15]);

    __u32 stored_hash = sfs_sb->hash; // Сохраняем хеш
    sfs_sb->hash = 0; // Убедимся, что хеш не включается в расчёт
    __u32 computed_hash = calculate_hash(sfs_sb, sizeof(struct simplefs_superblock) - sizeof(__u32));
    sfs_sb->hash = stored_hash; // Восстанавливаем хеш
    pr_info("Read superblock at offset %d: stored hash=%u, computed hash=%u\n", offset, stored_hash, computed_hash);
    if (computed_hash != stored_hash) {
        pr_err("Superblock hash mismatch at offset %d\n", offset);
        brelse(bh);
        return -EIO;
    }

    file_count = sfs_sb->file_count;
    brelse(bh);
    return 0;
}

// Итерация по каталогу
static int simplefs_readdir(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);

    if (ctx->pos >= 2) {
        return 0; // Только . и .. в корневом каталоге
    }

    if (ctx->pos == 0) {
        if (!dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR))
            return 0;
        ctx->pos++;
    }

    if (ctx->pos == 1) {
        if (!dir_emit(ctx, "..", 2, inode->i_ino, DT_DIR))
            return 0;
        ctx->pos++;
    }

    return 0;
}

// Функция для создания нового inode
static struct inode *simplefs_new_inode(struct super_block *sb, umode_t mode)
{
    struct inode *inode = new_inode(sb);
    if (!inode) {
        pr_err("Failed to allocate new inode\n");
        return ERR_PTR(-ENOMEM);
    }

    inode->i_ino = file_count + 2; // Следующий inode (1 зарезервирован для корня)
    inode_init_owner(&nop_mnt_idmap, inode, NULL, mode);
    inode->i_size = 0;
    inode->i_blocks = 0;
    inode->i_op = &simplefs_inode_ops;
    inode->i_fop = &simplefs_file_operations;
    pr_info("New inode created: ino=%lu, mode=0%o\n", inode->i_ino, inode->i_mode);

    return inode;
}

// Функция для создания файла
static int simplefs_create(struct mnt_idmap *idmap, struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
    struct inode *inode;
    struct super_block *sb = dir->i_sb;

    inode = simplefs_new_inode(sb, S_IFREG | mode);
    if (IS_ERR(inode)) {
        return PTR_ERR(inode);
    }

    d_instantiate(dentry, inode);
    file_count++; // Увеличиваем счётчик файлов
    pr_info("File created: name=%s, ino=%lu\n", dentry->d_name.name, inode->i_ino);

    return 0;
}

// Операции с суперблоком
static const struct super_operations simplefs_super_ops = {
};

// Заполнение суперблока
static int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    int ret;
    loff_t size = get_capacity(sb->s_bdev->bd_disk) * 512; // Размер устройства в байтах

    pr_info("Filling superblock for device size: %lld bytes\n", size);

    simplefs_sb = sb;
    simplefs_bdev = sb->s_bdev;

    sb->s_op = &simplefs_super_ops;
    sb->s_magic = 0x5F5F5F5F; // Магическое число

    if (size < (sb_second_offset + 1) * 512) {
        pr_err("Device size (%lld bytes) too small for filesystem (needs at least %d bytes)\n", size, (sb_second_offset + 1) * 512);
        return -ENOSPC;
    }

    ret = simplefs_read_superblock(sb, sb_first_offset);
    if (ret) {
        pr_info("Formatting disk...\n");
        ret = simplefs_format_disk(sb);
        if (ret) {
            pr_err("Failed to format disk: %d\n", ret);
            return ret;
        }
        pr_info("Verifying formatted superblock...\n");
        ret = simplefs_read_superblock(sb, sb_first_offset);
        if (ret) {
            pr_err("Failed to read formatted superblock: %d\n", ret);
            return ret;
        }
    }

    root_inode = new_inode(sb);
    if (!root_inode) {
        pr_err("Failed to allocate root inode\n");
        return -ENOMEM;
    }

    inode_init_owner(&nop_mnt_idmap, root_inode, NULL, S_IFDIR | 0755);
    root_inode->i_ino = 1;
    root_inode->i_op = &simplefs_inode_ops;
    root_inode->i_fop = &simplefs_dir_operations;
    pr_info("Root inode created: mode=0%o, ino=%lu\n", root_inode->i_mode, root_inode->i_ino);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        pr_err("Failed to create root dentry\n");
        iput(root_inode);
        return -ENOMEM;
    }

    pr_info("Superblock filled successfully\n");
    return 0;
}

// Кастомная функция монтирования
static struct dentry *simplefs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    return mount_bdev(fs_type, flags, dev_name, data, simplefs_fill_super);
}

// Определение файловой системы
static struct file_system_type simplefs_type = {
    .owner = THIS_MODULE,
    .name = "simplefs",
    .mount = simplefs_mount,
    .kill_sb = kill_block_super,
    .fs_flags = FS_REQUIRES_DEV,
};

// Инициализация модуля
static int __init simplefs_module_init(void)
{
    int ret;

    if (!disk_name) {
        pr_err("Disk name not specified\n");
        return -EINVAL;
    }

    ret = register_filesystem(&simplefs_type);
    if (ret) {
        pr_err("Failed to register filesystem: %d\n", ret);
        return ret;
    }

    pr_info("Module loaded successfully\n");
    return 0;
}

// Очистка модуля
static void __exit simplefs_module_exit(void)
{
    unregister_filesystem(&simplefs_type);
    pr_info("Module unloaded\n");
}

module_init(simplefs_module_init);
module_exit(simplefs_module_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SimpleFS Author");
MODULE_DESCRIPTION("Simple Filesystem Module");