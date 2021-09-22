#ifndef VSF_DEFS_H
#define VSF_DEFS_H

//modify by chz for our platform
//#define VSFTP_DEFAULT_CONFIG    "/etc/vsftpd.conf"

#define VSFTP_DIR				"/var/vsftp"
#define HOME_DIR 				"/var/vsftp/root"
#define VSFTP_ETC_DIR			"/var/vsftp/etc"
#define VSFTP_DEFAULT_PASSWD    "/var/vsftp/etc/passwd"
#define VSFTP_DEFAULT_CONFIG    "/var/vsftp/etc/vsftpd.conf"
#define VSFTP_VAR_DIR			"/var/vsftp/var"
#define VSFTP_SSL_DIR			"/var/vsftp/ssl"
#define VSFTP_CERTS_DIR			"/var/vsftp/ssl/certs"
#define VSFTP_LOG_DIR			"/var/vsftp/log"
//end modify

#define VSFTP_COMMAND_FD        0

#define VSFTP_PASSWORD_MAX      128
#define VSFTP_USERNAME_MAX      128
#define VSFTP_MAX_COMMAND_LINE  4096
#define VSFTP_DATA_BUFSIZE      65536
#define VSFTP_DIR_BUFSIZE       16384
#define VSFTP_PATH_MAX          4096
#define VSFTP_CONF_FILE_MAX     100000
#define VSFTP_LISTEN_BACKLOG    32
#define VSFTP_SECURE_UMASK      077
#define VSFTP_ROOT_UID          0
/* Must be at least the size of VSFTP_MAX_COMMAND_LINE, VSFTP_DIR_BUFSIZE and
   VSFTP_DATA_BUFSIZE*2 */
#define VSFTP_PRIVSOCK_MAXSTR   VSFTP_DATA_BUFSIZE * 2
#define VSFTP_AS_LIMIT          100UL * 1024 * 1024

#endif /* VSF_DEFS_H */

