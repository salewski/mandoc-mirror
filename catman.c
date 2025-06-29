/*	$Id$ */
/*
 * Copyright (c) 2017 Michael Stapelberg <stapelberg@debian.org>
 * Copyright (c) 2017 Ingo Schwarze <schwarze@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHORS DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"

#if NEED_XPG4_2
#define _XPG4_2
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#if HAVE_ERR
#include <err.h>
#endif
#include <errno.h>
#include <fcntl.h>
#if HAVE_FTS
#include <fts.h>
#else
#include "compat_fts.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int	 process_manpage(int, int, const char *);
int	 process_tree(int, int);
void	 run_mandocd(int, const char *, const char *)
		__attribute__((__noreturn__));
ssize_t	 sock_fd_write(int, int, int, int);
void	 usage(void) __attribute__((__noreturn__));


void
run_mandocd(int sockfd, const char *outtype, const char* defos)
{
	char	 sockfdstr[10];
	int	 len;

	len = snprintf(sockfdstr, sizeof(sockfdstr), "%d", sockfd);
	if (len >= (int)sizeof(sockfdstr)) {
		errno = EOVERFLOW;
		len = -1;
	}
	if (len < 0)
		err(1, "snprintf");
	if (defos == NULL)
		execlp("mandocd", "mandocd", "-T", outtype,
		    sockfdstr, (char *)NULL);
	else
		execlp("mandocd", "mandocd", "-T", outtype,
		    "-I", defos, sockfdstr, (char *)NULL);
	err(1, "exec(mandocd)");
}

ssize_t
sock_fd_write(int fd, int fd0, int fd1, int fd2)
{
	const struct timespec timeout = { 0, 10000000 };  /* 0.01 s */
	struct msghdr	 msg;
	struct iovec	 iov;
	union {
		struct cmsghdr	 cmsghdr;
		char		 control[CMSG_SPACE(3 * sizeof(int))];
	} cmsgu;
	struct cmsghdr	*cmsg;
	int		*walk;
	ssize_t		 sz;
	unsigned char	 dummy[1] = {'\0'};

	iov.iov_base = dummy;
	iov.iov_len = sizeof(dummy);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = cmsgu.control;
	msg.msg_controllen = sizeof(cmsgu.control);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(3 * sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	walk = (int *)CMSG_DATA(cmsg);
	*(walk++) = fd0;
	*(walk++) = fd1;
	*(walk++) = fd2;

	/*
	 * It appears that on some systems, sendmsg(3)
	 * may return EAGAIN even in blocking mode.
	 * Seen for example on Oracle Solaris 11.2.
	 * The sleeping time was chosen by experimentation,
	 * to neither cause more than a handful of retries
	 * in normal operation nor unnecessary delays.
	 */
	while ((sz = sendmsg(fd, &msg, 0)) == -1) {
		if (errno != EAGAIN) {
			warn("FATAL: sendmsg");
			break;
		}
		nanosleep(&timeout, NULL);
	}
	return sz;
}

int
process_manpage(int srv_fd, int dstdir_fd, const char *path)
{
	int	 in_fd, out_fd;
	int	 irc;

	if ((in_fd = open(path, O_RDONLY)) == -1) {
		warn("open %s for reading", path);
		fflush(stderr);
		return 0;
	}

	if ((out_fd = openat(dstdir_fd, path,
	    O_WRONLY | O_NOFOLLOW | O_CREAT | O_TRUNC,
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		warn("openat %s for writing", path);
		fflush(stderr);
		close(in_fd);
		return 0;
	}

	irc = sock_fd_write(srv_fd, in_fd, out_fd, STDERR_FILENO);

	close(in_fd);
	close(out_fd);

	return irc;
}

int
process_tree(int srv_fd, int dstdir_fd)
{
	FTS		*ftsp;
	FTSENT		*entry;
	const char	*argv[2];
	const char	*path;
	int		 fatal;
	int		 gooddirs, baddirs, goodfiles, badfiles;

	argv[0] = ".";
	argv[1] = (char *)NULL;

	if ((ftsp = fts_open((char * const *)argv,
	    FTS_PHYSICAL | FTS_NOCHDIR, NULL)) == NULL) {
		warn("fts_open");
		return -1;
	}

	fatal = 0;
	gooddirs = baddirs = goodfiles = badfiles = 0;
	while (fatal == 0 && (entry = fts_read(ftsp)) != NULL) {
		path = entry->fts_path + 2;
		switch (entry->fts_info) {
		case FTS_F:
			switch (process_manpage(srv_fd, dstdir_fd, path)) {
			case -1:
				fatal = errno;
				break;
			case 0:
				badfiles++;
				break;
			default:
				goodfiles++;
				break;
			}
			break;
		case FTS_D:
			if (*path != '\0' &&
			    mkdirat(dstdir_fd, path, S_IRWXU | S_IRGRP |
			      S_IXGRP | S_IROTH | S_IXOTH) == -1 &&
			    errno != EEXIST) {
				warn("mkdirat %s", path);
				fflush(stderr);
				(void)fts_set(ftsp, entry, FTS_SKIP);
				baddirs++;
			} else
				gooddirs++;
			break;
		case FTS_DP:
			break;
		case FTS_DNR:
			warnx("directory %s unreadable: %s",
			    path, strerror(entry->fts_errno));
			fflush(stderr);
			baddirs++;
			break;
		case FTS_DC:
			warnx("directory %s causes cycle", path);
			fflush(stderr);
			baddirs++;
			break;
		case FTS_ERR:
		case FTS_NS:
			warnx("file %s: %s",
			    path, strerror(entry->fts_errno));
			fflush(stderr);
			badfiles++;
			break;
		default:
			warnx("file %s: not a regular file", path);
			fflush(stderr);
			badfiles++;
			break;
		}
	}
	if (fatal == 0 && (fatal = errno) != 0)
		warn("FATAL: fts_read");

	fts_close(ftsp);
	if (baddirs > 0)
		warnx("skipped %d %s due to errors", baddirs,
		    baddirs == 1 ? "directory" : "directories");
	if (badfiles > 0)
		warnx("skipped %d %s due to errors", badfiles,
		    badfiles == 1 ? "file" : "files");
	if (fatal != 0) {
		warnx("processing aborted due to fatal error, "
		    "results are probably incomplete");
	}
	return 0;
}

int
main(int argc, char **argv)
{
	const char	*defos, *outtype;
	int		 srv_fds[2];
	int		 dstdir_fd;
	int		 opt;
	pid_t		 pid;

	defos = NULL;
	outtype = "ascii";
	while ((opt = getopt(argc, argv, "I:T:")) != -1) {
		switch (opt) {
		case 'I':
			defos = optarg;
			break;
		case 'T':
			outtype = optarg;
			break;
		default:
			usage();
		}
	}

	if (argc > 0) {
		argc -= optind;
		argv += optind;
	}
	if (argc != 2) {
		switch (argc) {
		case 0:
			warnx("missing arguments: srcdir and dstdir");
			break;
		case 1:
			warnx("missing argument: dstdir");
			break;
		default:
			warnx("too many arguments: %s", argv[2]);
			break;
		}
		usage();
	}

	if (socketpair(AF_LOCAL, SOCK_STREAM, AF_UNSPEC, srv_fds) == -1)
		err(1, "socketpair");

	pid = fork();
	switch (pid) {
	case -1:
		err(1, "fork");
	case 0:
		close(srv_fds[0]);
		run_mandocd(srv_fds[1], outtype, defos);
	default:
		break;
	}
	close(srv_fds[1]);

	if ((dstdir_fd = open(argv[1], O_RDONLY | O_DIRECTORY)) == -1)
		err(1, "open destination %s", argv[1]);

	if (chdir(argv[0]) == -1)
		err(1, "chdir to source %s", argv[0]);

	return process_tree(srv_fds[0], dstdir_fd) == -1 ? 1 : 0;
}

void
usage(void)
{
	fprintf(stderr, "usage: %s [-I os=name] [-T output] "
	    "srcdir dstdir\n", BINM_CATMAN);
	exit(1);
}
