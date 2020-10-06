//
//  fslinkfix.c
//  union_test
//
//  Created by Björn Eschrich on 14.09.20.
//  Copyright © 2020 Empire of Fun. All rights reserved.
//
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
/* "readdir" etc. are defined here. */
#include <dirent.h>
/* limits.h defines "PATH_MAX". */
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

int
handle_dir_entry(struct dirent * entry, int dephth_level, const char* dir_name, const char* path)
{
	char* full_link_path;
	char* new_link;
    char* e_type;
    struct stat sb;
    char* linkname;
    ssize_t r;
    int i, ret = 0;
	size_t size;

    if ((ret = lstat(path, &sb)) < 0)
    {
        fprintf(stderr, "ERROR: lstat failed: %s\n", path);
        return errno;
    }

    linkname = malloc(sb.st_size + 1);
    if (linkname == NULL)
    {
        fprintf(stderr, "ERROR: Insufficient memory\n");
        return ENOMEM;
    }

    errno = 0;

    if ((r = readlink(path, linkname, sb.st_size + 1)) < 0)
    {
        /* not a link, skip*/
        if (errno != EINVAL)
        {
            fprintf(stderr, "ERROR: readlink failed: %s\n", path);
            perror(" ");
			ret = errno;
        }
        free(linkname);
        return ret;
    }

    /* if a relative link, skip this entry */
    if (linkname[0] != '/')
    {
        free(linkname);
        return 0;
    }

    if (r > sb.st_size)
    {
        fprintf(stderr, "symlink increased in size between lstat() and readlink()\n");
        free(linkname);
        return EINVAL;
    }

    linkname[sb.st_size] = '\0';
    e_type = (entry->d_type & DT_DIR ? "D" : "F");

    //fprintf(stdout, "%s[%d]: '%s' -> '%s'\n", e_type, dephth_level, path, linkname);

	size = 3 * dephth_level + 1 + strlen(linkname);
    if ((new_link = malloc(size)) == NULL)
    {
        fprintf(stderr, "ERROR: Insufficient memory\n");
        free(linkname);
		return ENOMEM;
    }

	memset(new_link, 0, size);

	for (i = 0; i < dephth_level; i++)
	{
		strcat(new_link, "../");
	}

	strcat(new_link, (char*) &linkname[1]);

	/* check access to linked resource */
	size = strlen(dir_name) + strlen(entry->d_name) + strlen(new_link);
   	if (size > PATH_MAX)
	{
		fprintf(stderr, "ERROR: Path length exceed maximum of %d\n", PATH_MAX);
		free(linkname);
		free(new_link);
		return EINVAL;
	}

	if ((full_link_path = malloc(size)) == NULL)
	{
		fprintf(stderr, "ERROR: Insufficient memory\n");
        free(linkname);
        free(new_link);
        return ENOMEM;
	}

	memset(full_link_path, 0, size);
	snprintf(full_link_path, size, "%s/%s", dir_name, new_link);

	// fprintf(stdout, "%s[%d]: Check link %s to %s\n", "?", dephth_level, path, new_link);
	if (access(full_link_path, F_OK) == 0)
	{
    	/* remove old link */
    	if ((ret = unlink(path)))
    	{
        	fprintf(stderr, "ERROR: Can't remove old symlink %s\n", path);
        	ret = errno;
        	goto exit_cleanup;
    	}

    	/* create new link */
    	if ((ret = symlink(new_link, path)))
    	{
        	fprintf(stderr, "ERROR: Can't create symlink %s for %s\n", new_link, path);
        	ret = errno;
        	goto exit_cleanup;
    	}

		fprintf(stdout, "%s[%d]: %s linked to: %s\n", e_type, dephth_level, path, new_link);
		ret = 0;
	}
	else
	{
		// fprintf(stderr, "%s[%d]: Resource link %s inaccessible, skipped.\n", "-", dephth_level, full_link_path);
		ret = 0;
	}

exit_cleanup:
	free(full_link_path);
	free(new_link);
    free(linkname);
    return ret;
}

static void
list_dir (const char * dir_name, int dephth_level)
{
    int ret, p_len;
    char path[PATH_MAX];
    DIR * d;

	/* increment dephth */
	dephth_level++;

    /* Open the directory specified by "dir_name". */
    if (!(d = opendir (dir_name)))
    {
        fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
        return;
    }

    while (1)
    {
        struct dirent* entry;
        const char* d_name;

        /* "Readdir" gets subsequent entries from "d". */
        if (!(entry = readdir (d)))
        {
            /* There are no more entries in this directory,
             * so break out of the while loop. */
            break;
        }

        d_name = entry->d_name;

        if ((p_len = snprintf (path, PATH_MAX, "%s/%s", dir_name, d_name)) >= PATH_MAX)
        {
            fprintf (stderr, "Path length has got too long.\n");
            break;
        }

        /* handle path/file entry */
        if (strcmp (d_name, "..") != 0 &&
            strcmp (d_name, ".") != 0)
        {
            if ((ret = handle_dir_entry(entry, dephth_level, dir_name, path)))
			{
                if (ret == ENOMEM) /* stop! */
                    break;
			}
        }

        if (entry->d_type & DT_DIR)
        {
            /* Check that the directory is not "d" or d's parent. */
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0)
            {
                /* Recursively call "list_dir" with the new path. */
                list_dir (path, dephth_level);
            }
        }
    }

    /* After going through all the entries, close the directory. */
    if (closedir (d))
    {
        fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
    }
}

void
fix_links (const char * dir_name)
{
	fprintf(stdout, "ROOTFS directory: %s\n", dir_name);
    list_dir (dir_name, -1);
}

int main(int argc, char* argv[])
{
    if (argc < 1)
    {
        fprintf(stdout, "Usage: rootfs_fixlinks <rootfs dir>\n");
        return 1;
    }

    fix_links(argv[1]);
    return 0;
}

