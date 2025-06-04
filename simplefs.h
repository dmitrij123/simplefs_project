// simplefs.h
#ifndef __SIMPLEFS_H
#define __SIMPLEFS_H

#include <linux/types.h>
#include <linux/fs.h>

// Параметры модуля
extern char *disk_name;
extern int sb_first_offset;
extern int sb_second_offset;
extern int max_filename_len;
extern int max_file_size_sectors;

// Структура superblock
struct simplefs_superblock {
    __u32 magic;                  // Магическое число (например, 0x5F5F5F5F)
    __u32 hash;                   // Хеш для проверки целостности
    __u32 file_count;             // Количество файлов
    __u32 max_filename_len;       // Максимальная длина имени файла
    __u32 max_file_size_sectors;  // Максимальный размер файла в секторах
    char reserved[512 - 20];      // Заполняем до размера сектора (512 байт)
};

// Структура файла (метаданные в RAM)
struct simplefs_file {
    char name[256];               // Имя файла (ограничиваем 256 символами)
    __u64 offset;                 // Смещение на диске
    __u32 size_sectors;           // Размер в секторах
};

// IOCTL команды
#define SIMPLEFS_IOC_CLEAR_FILES _IO('S', 1)
#define SIMPLEFS_IOC_WIPE_FS     _IO('S', 2)
#define SIMPLEFS_IOC_GET_HASHES  _IOR('S', 3, struct simplefs_file_hashes)
#define SIMPLEFS_IOC_GET_MAPPING _IOWR('S', 4, struct simplefs_file_mapping)

struct simplefs_file_hashes {
    char name[256];
    __u32 hash;
};

struct simplefs_file_mapping {
    char name[256];
    __u64 offset;
    __u32 size_sectors;
};

#endif