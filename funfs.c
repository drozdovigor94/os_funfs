/* 
 * Filesystem for OS course to have some fun with
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/random.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/fsnotify.h>

#define FUNFS_MAGIC 0x13579BDF
#define MAX_FILE_NUM 100

static struct inode *funfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *inode = new_inode(sb);

	if (inode) 
	{
		inode->i_ino = get_next_ino();
		inode->i_mode = mode;
		inode_init_owner(inode, NULL, mode);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	}
	return inode;
}

static int funfs_open(struct inode *inode, struct file *fp)
{
	printk(KERN_INFO "[FUNFS] File opening is not supported\n");
	return -1;
}

static ssize_t funfs_read_file(struct file *fp, char *buf, size_t count, loff_t *offset)
{
	printk(KERN_INFO "[FUNFS] File reading is not supported\n");
	return -1;
}


static ssize_t funfs_write_file(struct file *fp, const char *buf, size_t count, loff_t *offset)
{
	printk(KERN_INFO "[FUNFS] File writing is not supported\n");
	return -1;
}

static struct file_operations funfs_file_operations = {
	.open = funfs_open,
	.read = funfs_read_file,
	.write = funfs_write_file,
};

static struct dentry *funfs_create_file(struct super_block *sb, 
		struct dentry *dir,  const char *name)
{
	struct dentry *dentry;
	struct inode *inode;

	printk(KERN_INFO "[FUNFS] Creating file %s\n", name);

	dentry = lookup_one_len(name, dir, strlen(name));

	if (!IS_ERR(dentry) && d_really_is_positive(dentry))
	{
		printk(KERN_INFO "[FUNFS] file %s already exists, skipping\n", name);
		dput(dentry);
		return NULL;
	}
	if (IS_ERR(dentry))
	{
		printk(KERN_ERR "[FUNFS] failed to allocate dentry for file %s\n", name);
		return NULL;
	}
	inode = funfs_make_inode(sb, S_IFREG | 0644);
	if (!inode)
	{
		printk(KERN_ERR "[FUNFS] failed to create inode for file %s\n", name);
		dput(dentry);
		return NULL;
	}
	inode->i_fop = &funfs_file_operations;
	d_instantiate(dentry, inode);
	fsnotify_create(d_inode(dentry->d_parent), dentry);
	return dentry;
}

static void funfs_create_files(struct super_block *sb, int n)
{
	
	unsigned int x;
	int i;
	char filename[20];

	for (i = 0; i < n; i++)
	{
		get_random_bytes(&x, sizeof(x));
		x = (x % MAX_FILE_NUM) + 1;
		snprintf(filename, 20, "%d", x);
		funfs_create_file(sb, sb->s_root, filename);
	}
}

static int funfs_unlink(struct inode *dir, struct dentry *dentry)
{
	const char *rmname, *curname;
	long curnum, rmnum, minnum = LONG_MAX;
	struct dentry *dentry_iter;
	struct dentry *parent = dentry->d_parent;
	int res = 0;

	rmname = dentry->d_name.name;
	kstrtol(rmname, 10, &rmnum);
	printk(KERN_INFO "[FUNFS] deleting file: %ld\n", rmnum);

	
	list_for_each_entry (dentry_iter, &parent->d_subdirs, d_child)
	{
		curname = dentry_iter->d_name.name;
		kstrtol(curname, 10, &curnum);
		if (curnum <= minnum)
		{
			minnum = curnum;
		}
	}

	printk(KERN_INFO "[FUNFS] minimum file number is %ld\n", minnum);

	if (rmnum <= minnum)
	{
		printk(KERN_INFO "[FUNFS] Correct file, unlinking\n");
		res = simple_unlink(dir, dentry);
	}
	else
	{
		printk(KERN_INFO "[FUNFS] Wrong file, creating 2 new\n");
		funfs_create_files(dir->i_sb, 2);
		res = -1;
	}
	return res;
}

static struct inode_operations funfs_dir_inode_operations = {
	.lookup = simple_lookup,
	.link = simple_link,
	.unlink = funfs_unlink,
};

static struct super_operations funfs_s_op = {
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode
};

int funfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;

	unsigned int n;

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = FUNFS_MAGIC;
	sb->s_op = &funfs_s_op;
	
	root = funfs_make_inode(sb, S_IFDIR | 0755);
	if (!root)
		return -ENOSPC;
	root->i_op = &funfs_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	root_dentry = d_make_root(root);
	if (!root_dentry)
	{
		iput(root);
		return -ENOMEM;
	}
	sb->s_root = root_dentry;
	
	
	get_random_bytes(&n, sizeof(n));
	n = (n % 11) + 5;
	
	inode_lock(d_inode(root_dentry));
	funfs_create_files(sb, n);
	inode_unlock(d_inode(root_dentry));

	return 0;
}

static struct dentry *funfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	struct dentry *root_dentry;

	root_dentry = mount_nodev(fs_type, flags, data, funfs_fill_super);

	if (IS_ERR(root_dentry))
		printk(KERN_ERR "[FUNFS] fs mounting failed\n");
	else
		printk(KERN_INFO "[FUNFS] fs mounting successful\n");
	
	return root_dentry;
}

static struct file_system_type funfs_fs_type = {
	.owner = THIS_MODULE,
	.name  = "funfs",
	.mount = funfs_mount,
	.kill_sb = kill_litter_super
};

static int __init init_funfs(void)
{
	int status;
	status = register_filesystem(&funfs_fs_type);
	if (status == 0)
		printk(KERN_INFO "[FUNFS] fs registration succesful\n");
	else
		printk(KERN_ERR "[FUNFS] fs registration failed, error code: %d\n", status);
	return status;
}

static void __exit exit_funfs(void)
{
	unregister_filesystem(&funfs_fs_type);
	printk(KERN_INFO "funfs module unloaded\n");
}

module_init(init_funfs);
module_exit(exit_funfs);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Igor Drozdov");
