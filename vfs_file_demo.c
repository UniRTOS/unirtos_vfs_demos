/*****************************************************************/ /**
* @file vfs_file_demo.c
* @brief VFS file system demonstration
* @author larson.li@quectel.com
* @date 2023-06-07
*
* @copyright Copyright (c) 2023 Quectel Wireless Solution, Co., Ltd.
* All Rights Reserved. Quectel Wireless Solution Proprietary and Confidential.
*
* @par EDIT HISTORY FOR MODULE
* <table>
* <tr><th>Date <th>Version <th>Author <th>Description"
* <tr><td>2023-06-07 <td>1.0 <td>Larson.Li <td> Initial version
* </table>
**********************************************************************/
#include "qosa_log.h"
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_virtual_file.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG                   LOG_TAG

#define UNIR_VFS_DEMO_TASK_STACK_SIZE 4096

/**
 * @brief List all files and subdirectories in the specified directory and output detailed information.
 *
 * This function opens the directory at the specified path and iterates through each directory entry.
 * For regular files, it outputs their name, size, and permission mode; for subdirectories,
 * it outputs their name and total size.
 *
 * @param file_path Pointer to a null-terminated string representing the directory path to list contents from.
 */
static void unir_vfs_demo_dir_list(char *file_path)
{
    QOSA_VFS_DIR             *dir = QOSA_NULL;                 // Directory handle
    struct qosa_vfs_dirent_t *entry = QOSA_NULL;               // Directory entry information storage
    struct qosa_vfs_stat_t    st = {0};                        // File status information storage
    qosa_int64_t              size = 0;                        // Directory total size storage
    qosa_int32_t              ret = 0;                         // Function return value
    char                      child[QOSA_VFS_PATH_MAX] = {0};  // Subdirectory absolute path

    // Open test directory "./testdir"
    dir = qosa_vfs_opendir(file_path);
    if (dir == QOSA_NULL)
    {
        QLOGE("dir open err=%d", qosa_get_errno());
        return;
    }

    // Loop through each item in the directory
    while ((entry = qosa_vfs_readdir(dir)) != QOSA_NULL)
    {
        // Output the name of the current directory entry
        QLOGV("%s\n", entry->d_name);

        // Check if it's a regular file
        if (entry->d_type == QOSA_VFS_DT_REG)
        {
            char path[1024] = {0};

            // Concatenate the complete file path
            qosa_snprintf(path, sizeof(path), "./%s", entry->d_name);

            // Get file status information
            if (qosa_vfs_stat(path, &st) == -1)
            {
                QLOGV("stat");
            }
            else
            {
                QLOGV("File name: %s", entry->d_name);
                QLOGV("File size: %ld bytes", (long)st.st_size);
                QLOGV("File mode: %o", (int)st.st_mode);
            }
        }
        // Check if it's a subdirectory
        else if (entry->d_type == QOSA_VFS_DT_DIR)
        {
            // Complete subdirectory to absolute path
            qosa_memset(child, 0, sizeof(child));
            qosa_snprintf(child, sizeof(child), "%s/%s", file_path, entry->d_name);
            // Get the total size of the subdirectory
            size = qosa_vfs_dir_total_size(child);
            QLOGV("Dir name: [%s],[%s]", entry->d_name, child);
            qosa_int32_t high = 0;
            qosa_int32_t low = 0;
            high = ((size >> 32) & 0xFFFFFFFF);
            low = (size & 0xFFFFFFFF);
            // eigen doesn't support directly printing 64-bit types, so split into high and low 32-bit parts for printing
            QLOGV("Dir size: %d,%d bytes", high, low);
        }
    }
    ret = qosa_vfs_closedir(dir);
    QLOGD("close ret=%d", ret);
}

/**
 * @brief Test directory operation functions, including opening directories, reading directory contents,
 *        getting file information, and deleting directories.
 *
 * This function demonstrates how to use the VFS interface to traverse files and subdirectories
 * in the specified directory ("./testdir"), and output each entry's name, type, size, and other information.
 * Finally, it closes and deletes the test directory.
 *
 * @note This function is static and only visible within the current file.
 * @note If directory opening fails, it returns directly.
 * @note During traversal, it distinguishes between regular files and subdirectories and handles them separately.
 * @note Finally, it attempts to close and remove the test directory.
 */
static void unir_vfs_demo_dir_test(void)
{
    QOSA_VFS_DIR *dir = QOSA_NULL;  // Directory handle
    qosa_int32_t  ret = 0;          // Function return value

    QLOGV("test dir");

    // Open test directory "./testdir"
    dir = qosa_vfs_opendir("./testdir");
    if (dir == QOSA_NULL)
    {
        QLOGE("dir not exist");
        // Create test directory
        ret = qosa_vfs_mkdir("./testdir", 0);
        if (ret != 0)
        {
            QLOGE("mkdir err=%d", qosa_get_errno());
            return;
        }
    }

    // Create test files and directories
    ret = qosa_vfs_creat("./testdir/vfs_test1.txt", 0);
    if (ret < 0)
    {
        QLOGE("creat file err=%d", qosa_get_errno());
    }
    ret = qosa_vfs_creat("./testdir/vfs_test2.txt", 0);
    if (ret < 0)
    {
        QLOGE("creat file2 err=%d", qosa_get_errno());
    }
    ret = qosa_vfs_mkdir("./testdir/subdir", 0);
    if (ret != 0)
    {
        QLOGE("subdir mkdir err=%d", qosa_get_errno());
    }

    // List all files and subdirectories in the test directory
    unir_vfs_demo_dir_list("./testdir");

    // Open test directory "./testdir"
    dir = qosa_vfs_opendir("./testdir");
    if (dir == QOSA_NULL)
    {
        QLOGE("dir open err=%d", qosa_get_errno());
        return;
    }
    // Close directory and output result
    ret = qosa_vfs_closedir(dir);
    QLOGD("close dir ret=%d", ret);

    // Delete test directory and output result, this API can only delete empty folders, here it should report an error
    ret = qosa_vfs_rmdir("./testdir");
    QLOGD("remove dir ret=%d", ret);

    // Delete test directory and all files under the test directory
    ret = qosa_vfs_rmdir_recursive("./testdir");
    QLOGD("remove dir ret=%d", ret);
}

/**
 * @brief VFS file operation test function
 * @details This function demonstrates the complete operation flow of the VFS virtual file system,
 *          including file creation, writing, reading, status query, and other operations
 * @param None
 * @return None
 */
static void unir_vfs_demo_file_test(void)
{
    int                    fd = 0;
    int                    ret = 0;
    char                   data[10 + 1] = {0};
    struct qosa_vfs_stat_t stat = {0};

    QLOGV("test file");
    /* Open or create test file, supporting read-write mode */
    fd = qosa_vfs_open("./vfs_test.txt", QOSA_VFS_O_CREAT | QOSA_VFS_O_RDWR);
    if (fd < 0)
    {
        QLOGE("open dir error!!");
        return;
    }

    /* Write test data to file */
    qosa_snprintf(data, 10, "%s", "1234567890");
    ret = qosa_vfs_write(fd, data, 10);
    QLOGD("write ret=%d", ret);

    /* Get file status information */
    ret = qosa_vfs_fstat(fd, &stat);
    if (ret == 0)
    {
        /* Print file size */
        QLOGD("size=%d", stat.st_size);
    }

    /* Seek file pointer to offset 0 from beginning of file */
    qosa_vfs_lseek(fd, 0, QOSA_VFS_SEEK_SET);

    /* Read data from file and display */
    qosa_memset(data, 0, sizeof(data));
    ret = qosa_vfs_read(fd, data, 10);
    QLOGD("read ret=%d,data=[%s]", ret, data);

    /* Close file and delete test file */
    qosa_vfs_close(fd);
    qosa_vfs_unlink("./vfs_test.txt");
}

static void unir_vfs_task_handler(void *argv)
{
    QOSA_UNUSED(argv);
    qosa_task_sleep_sec(10);
    unir_vfs_demo_file_test();
    unir_vfs_demo_dir_test();
}

void unir_vfs_demo_init(void)
{
    int         err = 0;
    qosa_task_t vfs_task = QOSA_NULL;

    err = qosa_task_create(&vfs_task, UNIR_VFS_DEMO_TASK_STACK_SIZE, QOSA_PRIORITY_NORMAL, "vfs_demo", unir_vfs_task_handler, QOSA_NULL);
    if (err != QOSA_OK)
    {
        QLOGE("task create error");
        return;
    }
}

UNIRTOS_APP_EXPORT(320, "vfs_demo", unir_vfs_demo_init);