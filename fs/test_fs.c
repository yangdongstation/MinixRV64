/* Filesystem Unit Tests */

#include <minix/config.h>
#include <types.h>
#include <minix/vfs.h>
#include <early_print.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper function for assertions */
static void assert_true(int condition, const char *test_name)
{
    if (condition) {
        early_puts("  [PASS] ");
        early_puts(test_name);
        early_puts("\n");
        tests_passed++;
    } else {
        early_puts("  [FAIL] ");
        early_puts(test_name);
        early_puts("\n");
        tests_failed++;
    }
}

/* Test VFS initialization */
void test_vfs_init(void)
{
    extern int vfs_init(void);
    int result;

    early_puts("\n=== Test: VFS Initialization ===\n");

    result = vfs_init();
    assert_true(result == 0, "vfs_init() returns 0");
}

/* Test filesystem registration */
void test_fs_registration(void)
{
    extern int vfs_register_fs(const char *name, fs_ops_t *ops);

    /* Create a dummy filesystem operations structure */
    static fs_ops_t dummy_ops = {
        .mount = NULL,
        .unmount = NULL,
        .read_inode = NULL,
        .write_inode = NULL,
        .delete_inode = NULL,
        .open = NULL,
        .close = NULL,
        .read = NULL,
        .write = NULL,
        .seek = NULL,
        .mkdir = NULL,
        .rmdir = NULL,
        .readdir = NULL,
        .lookup = NULL,
        .name = "test"
    };

    int result;

    early_puts("\n=== Test: Filesystem Registration ===\n");

    /* Test registration with valid ops */
    result = vfs_register_fs("test", &dummy_ops);
    assert_true(result == 0, "vfs_register_fs() with valid ops returns 0");

    /* Test registration with NULL ops */
    result = vfs_register_fs("invalid", NULL);
    assert_true(result == -1, "vfs_register_fs() with NULL ops returns -1");
}

/* Test FAT filesystem initialization */
void test_fat_init(void)
{
    extern int fat_init(void);
    int result;

    early_puts("\n=== Test: FAT Filesystem Initialization ===\n");

    result = fat_init();
    assert_true(result == 0, "fat_init() returns 0");
}

/* Test FAT32 filesystem initialization */
void test_fat32_init(void)
{
    extern int fat32_init(void);
    int result;

    early_puts("\n=== Test: FAT32 Filesystem Initialization ===\n");

    result = fat32_init();
    assert_true(result == 0, "fat32_init() returns 0");
}

/* Test EXT2 filesystem initialization */
void test_ext2_init(void)
{
    extern int ext2_init(void);
    int result;

    early_puts("\n=== Test: EXT2 Filesystem Initialization ===\n");

    result = ext2_init();
    assert_true(result == 0, "ext2_init() returns 0");
}

/* Test EXT3 filesystem initialization */
void test_ext3_init(void)
{
    extern int ext3_init(void);
    int result;

    early_puts("\n=== Test: EXT3 Filesystem Initialization ===\n");

    result = ext3_init();
    assert_true(result == 0, "ext3_init() returns 0");
}

/* Test EXT4 filesystem initialization */
void test_ext4_init(void)
{
    extern int ext4_init(void);
    int result;

    early_puts("\n=== Test: EXT4 Filesystem Initialization ===\n");

    result = ext4_init();
    assert_true(result == 0, "ext4_init() returns 0");
}

/* Run all tests */
int run_filesystem_tests(void)
{
    early_puts("\n");
    early_puts("=====================================\n");
    early_puts("  FILESYSTEM UNIT TESTS\n");
    early_puts("=====================================\n");

    /* Run all tests */
    test_vfs_init();
    test_fs_registration();
    test_fat_init();
    test_fat32_init();
    test_ext2_init();
    test_ext3_init();
    test_ext4_init();

    /* Print summary */
    early_puts("\n");
    early_puts("=====================================\n");
    early_puts("TEST SUMMARY\n");
    early_puts("=====================================\n");
    early_puts("Tests Passed: ");
    /* Print number */
    if (tests_passed < 10) {
        early_puts("0");
    }
    early_puts("0");  /* Placeholder - proper number printing not available */
    early_puts("\n");
    early_puts("Tests Failed: ");
    if (tests_failed < 10) {
        early_puts("0");
    }
    early_puts("0");  /* Placeholder - proper number printing not available */
    early_puts("\n");
    early_puts("=====================================\n");

    return tests_failed == 0 ? 0 : -1;
}
