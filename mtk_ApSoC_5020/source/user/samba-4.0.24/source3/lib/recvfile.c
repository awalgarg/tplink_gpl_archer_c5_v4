/*
 Unix SMB/Netbios implementation.
 Version 3.2.x
 recvfile implementations.
 Copyright (C) Jeremy Allison 2007.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/*
 * This file handles the OS dependent recvfile implementations.
 * The API is such that it returns -1 on error, else returns the
 * number of bytes written.
 */

#include "includes.h"
#include "system/filesys.h"

/* Do this on our own in TRANSFER_BUF_SIZE chunks.
 * It's safe to make direct syscalls to lseek/write here
 * as we're below the Samba vfs layer.
 *
 * Returns -1 on short reads from fromfd (read error)
 * and sets errno.
 *
 * Returns number of bytes written to 'tofd'
 * return != count then sets errno.
 * Returns count if complete success.
 */

#ifndef TRANSFER_BUF_SIZE
#define TRANSFER_BUF_SIZE (128*1024)
#endif

static ssize_t default_sys_recvfile(int fromfd,
			int tofd,
			off_t offset,
			size_t count)
{
	int saved_errno = 0;
	size_t total = 0;
	size_t bufsize = MIN(TRANSFER_BUF_SIZE,count);
	size_t total_written = 0;
	char *buffer = NULL;

	DEBUG(10,("default_sys_recvfile: from = %d, to = %d, "
		"offset=%.0f, count = %lu\n",
		fromfd, tofd, (double)offset,
		(unsigned long)count));

	if (count == 0) {
		return 0;
	}

	if (tofd != -1 && offset != (off_t)-1) {
		if (lseek(tofd, offset, SEEK_SET) == -1) {
			if (errno != ESPIPE) {
				return -1;
			}
		}
	}

	buffer = SMB_MALLOC_ARRAY(char, bufsize);
	if (buffer == NULL) {
		return -1;
	}

	while (total < count) {
		size_t num_written = 0;
		ssize_t read_ret;
		size_t toread = MIN(bufsize,count - total);

		/* Read from socket - ignore EINTR. */
		read_ret = sys_read(fromfd, buffer, toread);
		if (read_ret <= 0) {
			/* EOF or socket error. */
			free(buffer);
			return -1;
		}

		num_written = 0;

		/* Don't write any more after a write error. */
		while (tofd != -1 && (num_written < read_ret)) {
			ssize_t write_ret;

			/* Write to file - ignore EINTR. */
			write_ret = sys_write(tofd,
					buffer + num_written,
					read_ret - num_written);

			if (write_ret <= 0) {
				/* write error - stop writing. */
				tofd = -1;
                                if (total_written == 0) {
					/* Ensure we return
					   -1 if the first
					   write failed. */
                                        total_written = -1;
                                }
				saved_errno = errno;
				break;
			}

			num_written += (size_t)write_ret;
			total_written += (size_t)write_ret;
		}

		total += read_ret;
	}

	free(buffer);
	if (saved_errno) {
		/* Return the correct write error. */
		errno = saved_errno;
	}
	return (ssize_t)total_written;
}

#if defined(HAVE_LINUX_SPLICE)

/*
 * Try and use the Linux system call to do this.
 * Remember we only return -1 if the socket read
 * failed. Else we return the number of bytes
 * actually written. We always read count bytes
 * from the network in the case of return != -1.
 */

#define MAX_SPLICE_SIZE	(64 * 1024)
ssize_t sys_recvfile(int fromfd,
			int tofd,
			off_t offset,
			size_t count)
{
	static int pipefd[2] = { -1, -1 };
	static bool try_splice_call = false;
	size_t total_written = 0;
	loff_t splice_offset = offset;

	DEBUG(10,("sys_recvfile: from = %d, to = %d, "
		"offset=%.0f, count = %lu\n",
		fromfd, tofd, (double)offset,
		(unsigned long)count));

	if (count == 0) {
		return 0;
	}

	/*
	 * Older Linux kernels have splice for sendfile,
	 * but it fails for recvfile. Ensure we only try
	 * this once and always fall back to the userspace
	 * implementation if recvfile splice fails. JRA.
	 */

#if 0
	if (!try_splice_call) {
		return default_sys_recvfile(fromfd,
				tofd,
				offset,
				count);
	}

	if ((pipefd[0] == -1) && (pipe(pipefd) == -1)) {
		try_splice_call = false;
		return default_sys_recvfile(fromfd, tofd, offset, count);
	}
#endif

	while (count > 0) {
		int nread, to_write;

		if (count > MAX_SPLICE_SIZE)
			to_write = MAX_SPLICE_SIZE;
		else
			to_write = count;
#if 0
		nread = splice(fromfd, NULL, pipefd[1], NULL,
			       MIN(count, 16384), SPLICE_F_MOVE);
#else
		nread = splice(fromfd, NULL, tofd, &splice_offset,
				to_write, SPLICE_F_MOVE);
#endif
		if (nread == -1) {
			if (errno == EINTR) {
				continue;
			}
			if (total_written == 0 &&
			    (errno == EBADF || errno == EINVAL)) {
				try_splice_call = false;
				return default_sys_recvfile(fromfd, tofd,
							    offset, count);
			}
			break;
		}

#if 0
		to_write = nread;
		while (to_write > 0) {
			int thistime;
			thistime = splice(pipefd[0], NULL, tofd,
					  &splice_offset, to_write,
					  SPLICE_F_MOVE);
			if (thistime == -1) {
				goto done;
			}
			to_write -= thistime;
		}
#endif

		total_written += nread;
		count -= nread;
	}

 done:
	if (count) {
		int saved_errno = errno;
		if (drain_socket(fromfd, count) != count) {
			/* socket is dead. */
			return -1;
		}
		errno = saved_errno;
	}

	return total_written;
}
#else

/*****************************************************************
 No recvfile system call - use the default 128 chunk implementation.
*****************************************************************/

ssize_t sys_recvfile(int fromfd,
			int tofd,
			off_t offset,
			size_t count)
{
	return default_sys_recvfile(fromfd, tofd, offset, count);
}
#endif

/*****************************************************************
 Throw away "count" bytes from the client socket.
 Returns count or -1 on error.
 Must only operate on a blocking socket.
*****************************************************************/

ssize_t drain_socket(int sockfd, size_t count)
{
	size_t total = 0;
	size_t bufsize = MIN(TRANSFER_BUF_SIZE,count);
	char *buffer = NULL;
	int old_flags = 0;

	if (count == 0) {
		return 0;
	}

	buffer = SMB_MALLOC_ARRAY(char, bufsize);
	if (buffer == NULL) {
		return -1;
	}

	old_flags = fcntl(sockfd, F_GETFL, 0);
	if (set_blocking(sockfd, true) == -1) {
		free(buffer);
		return -1;
	}

	while (total < count) {
		ssize_t read_ret;
		size_t toread = MIN(bufsize,count - total);

		/* Read from socket - ignore EINTR. */
		read_ret = sys_read(sockfd, buffer, toread);
		if (read_ret <= 0) {
			/* EOF or socket error. */
			count = (size_t)-1;
			goto out;
		}
		total += read_ret;
	}

  out:

	free(buffer);
	if (fcntl(sockfd, F_SETFL, old_flags) == -1) {
		return -1;
	}
	return count;
}
